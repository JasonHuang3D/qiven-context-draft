// ============================================================================
// runtime.cpp — the causal Work cycle (numbered steps are normative)
//
// DR-002: the model never chooses its own recovery. A refusal's reason is
// looked up in the policy table carried by cognition, and the mandated next
// action is recorded in the checkpoint. Only StaleBase parks a delta for a
// re-think; handoff refusals halt and escalate — retrying is forbidden.
// ============================================================================

#include <qiven/context/runtime.hpp>

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

void LLM::Work(const std::string& prompt, const AuthenticatedActor& actor, std::string& result,
               WorkMode mode)
{
    const auto handle = pCognition; // pin lifetime for this whole cycle;
                                    // authority is fenced by the grant (DR-010)
    if (!handle)
    {
        result = name + ": no cognition bound";
        return;
    }

    // 1. single-writer lease: a competing flow is refused fail-closed (P-01)
    const auto grant = QivenContext::AcquireGrant(actor, mode);
    if (!grant.has_value())
    {
        checkpoint.nextAction = "fail closed: a competing flow holds the chain";
        result                = name + ": grant refused";
        return;
    }

    // 2. full snapshot for thinking — the typed Bundle with floors lands in
    //    Phase 2 (DR-007); token cost is a Bundle concern, never K5's
    const Query query = queryBuilder ? queryBuilder(prompt) : Query { prompt, 0 };
    const Data data   = QivenContext::ReadFromCognition(handle, query);
    static_cast<void>(data); // 3-4. VerifyLiveFacts + Thinking + CallTools: opaque here

    // 5. build the write shape; no generator or empty transaction = ordinary turn
    ContextTransaction transaction;
    if (deltaGenerator)
    {
        transaction = deltaGenerator(prompt, *handle->state);
    }
    if (transaction.operations.empty())
    {
        QivenContext::ReleaseGrant(*grant);
        checkpoint.nextAction = "continue";
        result                = name + ": no material cognition this turn";
        return;
    }
    transaction.base = handle->contentId; // durable fencing token: my thinking assumed this

    // 6. gated write — every gate inside the service; verdict carries the reason
    const Verdict verdict = QivenContext::WriteToCognition(handle, transaction, *grant);
    QivenContext::ReleaseGrant(*grant);

    if (verdict.outcome == Verdict::Outcome::Applied)
    {
        checkpoint.acceptedRefs.push_back(verdict.successorId); // post-write: the advanced id
        checkpoint.nextAction = "continue";
        result                = name + ": delta applied; head=" + verdict.successorId;
        return;
    }

    // 7. refused: the recovery table in cognition mandates the next action
    const RecoveryAction action =
        QivenContext::RecoveryFor(*handle->state, verdict.reason); // pre-write table: policy rows are stable
    checkpoint.unacceptedCandidates.push_back(transaction.base);
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
} // namespace qiven::context
