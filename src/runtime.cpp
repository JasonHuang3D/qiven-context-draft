// ============================================================================
// runtime.cpp — the causal work cycle (numbered steps are normative)
//
// DR-002: the model never chooses its own recovery. A refusal's reason is
// looked up in the policy table carried by cognition, and the mandated next
// action is recorded in the checkpoint. Only StaleBase parks a delta for a
// re-think; handoff refusals halt and escalate — retrying is forbidden.
//
// DR-012: annotate_budget is called on EVERY completed turn — success,
// refusal, and no-op alike. The model never chooses its own recovery.
// ============================================================================

#include <qiven/context/runtime.hpp>

#include <chrono>
#include <utility>

namespace qiven::context
{
namespace
{
constexpr std::string_view refusalName(RefusalReason reason)
{
    switch (reason)
    {
    case RefusalReason::GrantRefused: return "grant-refused";
    case RefusalReason::UnverifiedActor: return "unverified-actor";
    case RefusalReason::StaleBase: return "stale-base";
    case RefusalReason::HandoffMissing: return "handoff-missing";
    case RefusalReason::HandoffInvalid: return "handoff-invalid";
    case RefusalReason::UnattendedMutation: return "unattended-mutation";
    case RefusalReason::InvariantFailed: return "invariant-failed";
    case RefusalReason::GovernanceDenied: return "governance-denied";
    case RefusalReason::StoreDiverged: return "store-diverged";
    case RefusalReason::OutcomeUnresolved: return "outcome-unresolved";
    case RefusalReason::KeyConflict: return "key-conflict";
    }
    return "unknown";
}
} // namespace

void LLM::work(const std::string& prompt, const AuthenticatedActor& actor, std::string& result,
               WorkMode mode)
{
    const auto turnStart = std::chrono::steady_clock::now(); // session economics (DR-012)
    work_impl(prompt, actor, result, mode, turnStart);
    annotate_budget(turnStart); // EVERY completed turn: success, refusal, no-op
}

void LLM::work_impl(const std::string& prompt, const AuthenticatedActor& actor,
                    std::string& result, WorkMode mode,
                    std::chrono::steady_clock::time_point turnStart)
{
    const auto handle = pCognition; // pin lifetime for this whole cycle;
                                    // authority is fenced by the grant (DR-010)
    if (!handle)
    {
        result = name + ": no cognition bound";
        return;
    }

    // 1. single-writer lease: a competing flow is refused fail-closed (P-01)
    const auto grant = QivenContext::acquire_grant(actor, mode);
    if (!grant.has_value())
    {
        checkpoint.lastTrigger = CheckpointTrigger::TurnBoundary;
        checkpoint.nextAction  = "fail closed: a competing flow holds the chain";
        result                 = name + ": grant refused";
        return;
    }
    checkpoint.servingDisclosure = actor.servingModel + "/" + actor.binding; // pit.undisclosed_substitution_flagged

    // 2. full snapshot for thinking — the typed Bundle with floors lands in
    //    Phase 2 (DR-007); token cost is a Bundle concern, never K5's
    const Query query = queryBuilder ? queryBuilder(prompt) : Query { prompt, 0 };
    const Data data   = QivenContext::read_from_cognition(handle, query);
    static_cast<void>(data); // 3-4. VerifyLiveFacts + Thinking + CallTools: opaque here

    // 5. build the write shape; no generator or empty transaction = ordinary turn
    ContextTransaction transaction;
    if (deltaGenerator)
    {
        transaction = deltaGenerator(prompt, *handle->state);
    }
    if (transaction.operations.empty())
    {
        QivenContext::release_grant(*grant);
        checkpoint.nextAction = "continue";
        result                = name + ": no material cognition this turn";
        return;
    }
    transaction.base = handle->revision; // durable fencing token: my thinking assumed this

    // 6. gated write — every gate inside the service; verdict carries the reason
    const Verdict verdict = QivenContext::write_to_cognition(handle, transaction, *grant);
    QivenContext::release_grant(*grant);

    if (verdict.outcome == Verdict::Outcome::Applied)
    {
        // review §2: the write minted a SUCCESSOR materialization; rebind to it
        // so the next cycle sees the advanced world (a runtime rebind, R3)
        pCognition = verdict.successor;
        checkpoint.acceptedRefs.push_back(verdict.successor->revision);
        checkpoint.lastTrigger = CheckpointTrigger::MaterialTransaction;
        checkpoint.nextAction  = "continue";
        result                 = name + ": delta applied; head=" + verdict.successor->revision.value;
        return;
    }

    // 7. refused: the recovery table in cognition mandates the next action
    const RecoveryAction action =
        QivenContext::recovery_for(*handle->state, verdict.reason); // pre-write table: policy rows are stable
    checkpoint.unacceptedCandidates.push_back(transaction.base);
    if (verdict.outcome == Verdict::Outcome::OutcomeUnknown)
    {
        // typed absence: the commit outcome is genuinely unknowable right now
        // pit.gaps_recorded_not_synthesized: typed absence, never synthesized
        checkpoint.evidenceGaps.push_back({ "commit outcome for key " + transaction.idempotencyKey,
                                            "store acknowledgement lost; the receipt records it as unresolved" });
    }
    switch (action)
    {
    case RecoveryAction::RereadRethink:
        // the ONLY park-and-rethink path: a stale base means the thinking was
        // stale — hold, re-read, re-think (never retry the write blind)
        parkedDeltas.push_back(transaction);
        checkpoint.knownGaps  = "delta parked: admission gate refused";
        checkpoint.nextAction = "re-read, re-think; never blind-retry";
        break;
    case RecoveryAction::HaltEscalate:
        checkpoint.nextAction = "halt; escalate for typed handoff; retry is forbidden";
        break;
    case RecoveryAction::DeferToSupervised:
        checkpoint.nextAction = "defer to a supervised session";
        break;
    case RecoveryAction::DesignReview:
        checkpoint.nextAction = "design review required (constitution 17)";
        break;
    case RecoveryAction::FailClosed:
        checkpoint.nextAction = "fail closed";
        break;
    case RecoveryAction::Block:
        checkpoint.nextAction = "outcome unknown; dependent mutations stopped";
        break;
    }
    result = name + ": refused (" + std::string(refusalName(verdict.reason)) + "); next: " + checkpoint.nextAction;
}

void LLM::annotate_budget(std::chrono::steady_clock::time_point turnStart) // pit.liveness_reports_observables
{
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - turnStart);
    if (elapsed >= turnBudget.hard)
    {
        checkpoint.nextAction += "; HARD budget exceeded: halt";
    }
    else if (elapsed >= turnBudget.soft)
    {
        checkpoint.nextAction += "; soft budget reached: finish the safe checkpoint";
    }
}
} // namespace qiven::context
