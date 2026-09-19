// ============================================================================
// persistence_fencing — the admission gates in normative order, with the pit
// regression suite (P-01..P-48) and the v3 phase 2C semantic-closure proofs:
//   orthogonal identities (§3), typed store receipts (§6), immutable
//   materializations (§2), unforgeable grants (§4), authority-bound H2 (§5),
//   graph-edge validation and the runtime rebirth test (§9/§10, S7-R3).
// ============================================================================

#include <qiven/context/context.hpp>

#include "detail/check.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace qiven::context;

// pit.grant_unforgeable (review §4): a grant is a MINTED capability — private
// constructor, move-only. Assembling a value-equal forgery is not expressible.
static_assert(!std::is_default_constructible_v<ExecutionGrant>);
static_assert(!std::is_copy_constructible_v<ExecutionGrant>);
static_assert(!std::is_constructible_v<ExecutionGrant, GrantId, Epoch, PrincipalId, WorkMode>);

namespace
{
std::shared_ptr<MemoryStore> freshStore()
{
    auto store = std::make_shared<MemoryStore>();
    QivenContext::attachStore(store);
    return store;
}

CognitionSource bootFromHead()
{
    CognitionSource source;
    source.kind = CognitionSourceKind::CanonicalRemote;
    return source;
}

AuthenticatedActor goodActor()
{
    return AuthenticatedActor { "github:JasonHuang3D", Role::Worker,
                                "GLM-5.3-Flash", "GLM-5.3-Flash", "standard" };
}

AuthenticatedActor stranger()
{
    return AuthenticatedActor { "github:SomeoneElse", Role::Worker, "x", "some-model", "standard" };
}

ContextTransaction lessonTx(const RevisionId& base)
{
    ContextTransaction transaction;
    transaction.base = base;
    transaction.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                 .title         = "lesson",
                                                 .payload       = "gate order is normative",
                                                 .provenanceRef = "v3-phase2c-session" });
    return transaction;
}

// pit.port_never_reenters_service (P-42): the identity port receives the
// governance snapshot as context. Before the 2026-09-19 fix, the draft
// verifier called canonical_head() itself and re-locked the admission mutex on
// the same thread — MSVC Debug throws resource_deadlock_would_occur.
class RecordingVerifier final : public IIdentityVerifier
{
public:
    mutable bool called { false };
    mutable bool sawGovernance { false };

    [[nodiscard]] bool verify(const AuthenticatedActor& actor,
                              const Snapshot& governanceSource) const override
    {
        called        = true;
        sawGovernance = governanceSource.governance.rootPrincipal == "github:JasonHuang3D";
        return actor.principal == governanceSource.governance.rootPrincipal;
    }
};

ContextTransaction decisionTx(const RevisionId& base, std::int64_t id, bool withEvidence,
                              const char* payload)
{
    ContextTransaction transaction;
    transaction.base = base;
    transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                 .recordId = id,
                                                 .title    = "ADR-0036",
                                                 .payload  = payload });
    if (withEvidence)
    {
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:JasonHuang3D";  // root-tier H2 authority
        evidence.reviewerBinding     = "jason-brother-glm5-3"; // NOT the author binding
        evidence.reviewRef           = "PR-record review";
        evidence.reviewedDeltaDigest = digest_operations(transaction.operations);
        transaction.h2               = evidence;
    }
    return transaction;
}
} // namespace

int main()
{
    const auto store = freshStore();
    DeserializeError bootErr;
    const auto c1 = QivenContext::create_cognition(bootFromHead(), &bootErr); // genesis, epoch 1
    if (!c1)
    {
        std::printf("boot failed: kind=%d offset=%zu detail=%s\n", static_cast<int>(bootErr.kind),
                    bootErr.offset, bootErr.detail.c_str());
    }
    QCD_CHECK(c1 != nullptr);
    QCD_CHECK(c1->quarantine == QuarantineState::Promoted);
    QCD_CHECK(!c1->state->policy.handoff.empty()); // authority rules live in cognition (R4)

    // orthogonal identities (review §3): the materialization carries BOTH the
    // integrity digest and the storage revision, and they are different types
    std::printf("genesis digest: %s revision: %s\n", c1->digest.value.c_str(),
                c1->revision.value.c_str());
    QCD_CHECK(c1->digest == SnapshotDigest { "snap-2709358ddc94d185" }); // pit.golden_vector_pinned (v5 genesis format)
    QCD_CHECK(!IsEmpty(c1->revision));

    // pit.port_never_reenters_service (P-42): the identity port receives the
    // governance snapshot as context
    const auto recorder = std::make_shared<RecordingVerifier>();
    QivenContext::attach_identity_verifier(recorder);
    const auto portGrant = QivenContext::acquire_grant(goodActor(), WorkMode::SupervisedForeground);
    QCD_CHECK(portGrant.has_value());
    QCD_CHECK(recorder->called);
    QCD_CHECK(recorder->sawGovernance);
    QivenContext::release_grant(*portGrant);
    QivenContext::attach_identity_verifier(nullptr); // restore the default port

    // pit.unverified_actor_refused (P-02)
    QCD_CHECK(!QivenContext::acquire_grant(stranger(), WorkMode::SupervisedForeground).has_value());

    const auto actor = goodActor();
    const auto g1    = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g1.has_value());

    // pit.split_brain_second_flow_refused (P-01): fail-closed, never queued
    QCD_CHECK(!QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground).has_value());
    QCD_CHECK(!QivenContext::acquire_grant(stranger(), WorkMode::SupervisedForeground).has_value());

    // keyed write → applied; the SUCCESSOR MATERIALIZATION is minted (review §2)
    ContextTransaction keyed = lessonTx(c1->revision);
    keyed.idempotencyKey     = "txn-alpha-001";
    const auto keyedApply    = QivenContext::write_to_cognition(c1, keyed, *g1);
    QCD_CHECK(keyedApply.outcome == Verdict::Outcome::Applied);
    const RevisionId head1 = keyedApply.successor->revision;
    QCD_CHECK(store->head() == head1);
    QCD_CHECK(keyedApply.successor->digest == draft_snapshot_digest(serialize_snapshot(*keyedApply.successor->state)));
    QCD_CHECK(c1->revision != head1); // the pinned handle kept its OWN world

    // pit.same_key_one_outcome (DR-011): verbatim re-send resolves to the
    // original verdict without re-executing
    const auto replayed = QivenContext::write_to_cognition(c1, keyed, *g1);
    if (replayed.outcome != Verdict::Outcome::Applied)
    {
        const std::string replayEol(1, '\n');
        std::printf("replay outcome=%d reason=%d%s", static_cast<int>(replayed.outcome),
                    static_cast<int>(replayed.reason), replayEol.c_str());
    }
    QCD_CHECK(replayed.outcome == Verdict::Outcome::Applied);
    QCD_CHECK(replayed.successor->revision == head1);

    // pit.same_key_different_content_refused
    ContextTransaction conflicting    = keyed;
    conflicting.operations[0].payload = "a DIFFERENT request under the same key";
    QCD_CHECK(QivenContext::write_to_cognition(c1, conflicting, *g1).reason == RefusalReason::KeyConflict);

    // pit.timeout_yields_unknown_not_rollback (review §6): lost ack ≠ rollback
    class FaultInjectionStore final : public ICognitionStore
    {
    public:
        explicit FaultInjectionStore(std::shared_ptr<ICognitionStore> inner) :
        inner_(std::move(inner))
        {
        }
        bool loseNextAck { false };
        [[nodiscard]] StoreReceipt compareAndSwap(const RevisionId& base, const Bytes& stateBytes) override
        {
            const auto receipt = inner_->compareAndSwap(base, stateBytes);
            if (loseNextAck && receipt.kind == StoreReceipt::Kind::Committed)
            {
                loseNextAck = false;
                return StoreReceipt { StoreReceipt::Kind::OutcomeUnknown, {} }; // ack lost
            }
            return receipt;
        }
        [[nodiscard]] Bytes materialize(const RevisionId& revision) const override
        {
            return inner_->materialize(revision);
        }
        [[nodiscard]] bool verify(const RevisionId& revision) const override
        {
            return inner_->verify(revision);
        }
        [[nodiscard]] RevisionId head() const override
        {
            return inner_->head();
        }

    private:
        std::shared_ptr<ICognitionStore> inner_;
    };
    auto flaky = std::make_shared<FaultInjectionStore>(store);
    QivenContext::attachStore(flaky);
    flaky->loseNextAck      = true;
    ContextTransaction lost = lessonTx(keyedApply.successor->revision);
    lost.idempotencyKey     = "txn-beta-002";
    const auto unknown      = QivenContext::write_to_cognition(keyedApply.successor, lost, *g1);
    QCD_CHECK(unknown.outcome == Verdict::Outcome::OutcomeUnknown);
    QCD_CHECK(QivenContext::recovery_for(*keyedApply.successor->state,
                                         RefusalReason::OutcomeUnresolved) == RecoveryAction::Block);
    const auto resolved = QivenContext::write_to_cognition(keyedApply.successor, lost, *g1);
    QCD_CHECK(resolved.outcome == Verdict::Outcome::OutcomeUnknown); // receipt, not re-execution
    QCD_CHECK(store->head() != keyedApply.successor->revision);      // the commit HAD landed
    QivenContext::attachStore(store);                                // restore the honest store
    const std::size_t memoryAtUnknown = keyedApply.successor->state->memory.size();
    static_cast<void>(memoryAtUnknown);

    // a released lease never regains authority
    QivenContext::release_grant(*g1);
    QCD_CHECK(QivenContext::write_to_cognition(keyedApply.successor, lessonTx(head1), *g1).reason == RefusalReason::GrantRefused);
    const auto g1b = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g1b.has_value());

    // the BLOCKED lineage: after an OutcomeUnknown, dependent mutations on the
    // stale materialization stop (recovery = Block; the store CAS fails because
    // the head advanced) — dependent work stops without a rollback claim
    QCD_CHECK(QivenContext::write_to_cognition(
                  keyedApply.successor, lessonTx(keyedApply.successor->revision), *g1b)
                  .reason == RefusalReason::StoreDiverged);

    // pit.reader_boot_preserves_writer (DR-010): re-materialize the lineage and
    // a reader boot — neither revokes the active writer's lease
    const auto c2 = QivenContext::create_cognition(bootFromHead());
    QCD_CHECK(c2 != nullptr);
    QCD_CHECK(c2->epoch > c1->epoch); // epochs are monotonic per materialization
    QCD_CHECK(c2->revision == store->head());
    const auto writer = QivenContext::create_cognition(bootFromHead());
    QCD_CHECK(QivenContext::write_to_cognition(
                  writer, lessonTx(writer->revision), *g1b)
                  .outcome == Verdict::Outcome::Applied);

    // stale base → StaleBase: durable divergence, re-read before writing
    const auto stale = QivenContext::write_to_cognition(
        c2, lessonTx(RevisionId { "rev-doesnotexist" }), *g1b);
    QCD_CHECK(stale.reason == RefusalReason::StaleBase);
    QCD_CHECK(QivenContext::recovery_for(*keyedApply.successor->state, RefusalReason::StaleBase) == RecoveryAction::RereadRethink);

    // empty transaction = ordinary turn: applied, nothing stored
    const auto writer2 = QivenContext::create_cognition(bootFromHead()); // fresh at the head
    ContextTransaction ordinary;
    ordinary.base = writer2->revision;
    QCD_CHECK(QivenContext::write_to_cognition(writer2, ordinary, *g1b).outcome == Verdict::Outcome::Applied);
    QivenContext::release_grant(*g1b);

    // --- Work-cycle level: AUTHORITY-BOUND H2 (review §5) ----------------------

    Human human { "Jason", {}, "github:JasonHuang3D" };
    auto llm              = std::make_shared<LLM>();
    llm->name             = "GLM-5.3-Flash";
    llm->pCognition       = QivenContext::create_cognition(bootFromHead());
    auto device           = std::make_shared<Device>();
    device->name          = "JasonPC";
    auto client           = std::make_shared<LLMClientTool>();
    client->name          = "zcode-desktop";
    client->pCurrentLLM   = llm;
    client->pTargetDevice = device;
    client->binding       = ParticipantBinding { Role::Worker, "GLM-5.3-Flash" };

    // pit.handoff_missing_never_retried: merge-class without H2 → refused;
    // recovery mandates HaltEscalate; the delta is NOT parked for retry
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 37,
                                                     .title    = "ADR-0036",
                                                     .payload  = "typed human handoffs" });
        return transaction;
    };
    std::string result;
    human.UseLLMToWork(client, "accept without review", result);
    QCD_CHECK(result.find("handoff-missing") != std::string::npos);
    QCD_CHECK(llm->checkpoint.nextAction.find("halt") != std::string::npos);
    QCD_CHECK(llm->parkedDeltas.empty()); // retrying is forbidden; nothing parked

    // content- AND authority-bound H2 → applied (reviewer binding ≠ author binding)
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        return decisionTx(RevisionId {}, 37, true, "typed human handoffs");
    };
    human.UseLLMToWork(client, "accept with review", result);
    QCD_CHECK(result.find("delta applied") != std::string::npos);

    // pit.h2_evidence_rebind_refused: payload changed AFTER the review
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 38,
                                                     .title    = "ADR-0037",
                                                     .payload  = "executable specification" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer        = "github:JasonHuang3D";
        evidence.reviewerBinding = "jason-brother-glm5-3";
        evidence.reviewRef       = "PR-record review";
        std::vector<Operation> reviewed;
        reviewed.push_back(Operation { .kind = Operation::Kind::AppendDecision, .recordId = 38, .title = "ADR-0037", .payload = "a DIFFERENT payload" });
        evidence.reviewedDeltaDigest = digest_operations(reviewed);
        transaction.h2               = evidence;
        return transaction;
    };
    human.UseLLMToWork(client, "accept with stale review", result);
    QCD_CHECK(result.find("handoff-invalid") != std::string::npos);

    // review §5: the H2 gate is AUTHORITY-bound — a reviewer without the root
    // principal cannot certify a merge-class delta
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 39,
                                                     .title    = "ADR-0038",
                                                     .payload  = "unauthorized review" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:SomeoneElse";
        evidence.reviewerBinding     = "someone-else";
        evidence.reviewRef           = "PR-record review";
        evidence.reviewedDeltaDigest = digest_operations(transaction.operations);
        transaction.h2               = evidence;
        return transaction;
    };
    human.UseLLMToWork(client, "accept with unauthorized review", result);
    QCD_CHECK(result.find("handoff-invalid") != std::string::npos);

    // pit.self_review_refused (P-09): reviewer binding == author binding
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 40,
                                                     .title    = "ADR-0039",
                                                     .payload  = "self review" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:JasonHuang3D";
        evidence.reviewerBinding     = "GLM-5.3-Flash"; // the AUTHOR's binding
        evidence.reviewRef           = "PR-record review";
        evidence.reviewedDeltaDigest = digest_operations(transaction.operations);
        transaction.h2               = evidence;
        return transaction;
    };
    human.UseLLMToWork(client, "self review", result);
    QCD_CHECK(result.find("handoff-invalid") != std::string::npos);

    // review §5: an empty reviewRef is a HandoffMissing, not a pass
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 41,
                                                     .title    = "ADR-0040",
                                                     .payload  = "no review ref" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:JasonHuang3D";
        evidence.reviewerBinding     = "jason-brother-glm5-3";
        evidence.reviewedDeltaDigest = digest_operations(transaction.operations);
        transaction.h2               = evidence; // reviewRef EMPTY
        return transaction;
    };
    human.UseLLMToWork(client, "accept without review ref", result);
    QCD_CHECK(result.find("handoff-missing") != std::string::npos);

    // pit invariant: duplicate decision id → InvariantFailed → DesignReview
    llm->deltaGenerator = [](const std::string&, const Snapshot& snapshot) {
        const std::int64_t taken = snapshot.decisions.empty() ? 1 : snapshot.decisions.back().id;
        return decisionTx(RevisionId {}, taken, true, "duplicate id");
    };
    human.UseLLMToWork(client, "duplicate decision id", result);
    QCD_CHECK(result.find("invariant-failed") != std::string::npos);
    QCD_CHECK(llm->checkpoint.nextAction.find("design review") != std::string::npos);

    // pit.unattended_mutation_refused (P-08)
    const auto ug = QivenContext::acquire_grant(actor, WorkMode::Unattended);
    QCD_CHECK(ug.has_value());
    const auto current = QivenContext::create_cognition(bootFromHead());
    QCD_CHECK(QivenContext::write_to_cognition(current, lessonTx(current->revision), *ug).reason == RefusalReason::UnattendedMutation);
    QivenContext::release_grant(*ug);

    // --- Phase 2: conflicts, lifecycle, evidence (DR-006, P-14/P-36/P-37) -----
    // With immutable materializations, every direct write needs a CURRENT
    // handle: the fresh() helper re-materializes at the head, and reads always
    // go through a fresh handle too (a pinned handle keeps its pinned world).

    const auto g2 = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g2.has_value());
    const auto fresh = [&] { return QivenContext::create_cognition(bootFromHead()); };

    // pit.open_conflict_blocks_acceptance: an OPEN global conflict blocks a
    // merge-class acceptance even when its H2 evidence is valid
    {
        const auto w = fresh();
        ContextTransaction openConflict;
        openConflict.base = w->revision;
        openConflict.operations.push_back(Operation { .kind    = Operation::Kind::OpenConflict,
                                                      .scope   = "",
                                                      .title   = "state-wording",
                                                      .payload = "producer invocation wording disagrees" });
        QCD_CHECK(QivenContext::write_to_cognition(w, openConflict, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction blockedDecision =
            decisionTx(w->revision, 50, true, "blocked by conflict");
        QCD_CHECK(QivenContext::write_to_cognition(w, blockedDecision, *g2).reason == RefusalReason::ConflictUnresolved);
        QCD_CHECK(QivenContext::recovery_for(*w->state, RefusalReason::ConflictUnresolved) == RecoveryAction::FailClosed);
        ContextTransaction unrelatedMemory;
        unrelatedMemory.base = w->revision;
        unrelatedMemory.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                         .scope         = "retrieval",
                                                         .title         = "unrelated",
                                                         .payload       = "global conflicts still block",
                                                         .provenanceRef = "v3-phase2c-session" });
        QCD_CHECK(QivenContext::write_to_cognition(w, unrelatedMemory, *g2).reason == RefusalReason::ConflictUnresolved);
    }
    {
        const auto w = fresh();
        ContextTransaction resolveNoText;
        resolveNoText.base = w->revision;
        resolveNoText.operations.push_back(Operation { .kind  = Operation::Kind::ResolveConflict,
                                                       .scope = "",
                                                       .title = "state-wording" });
        QCD_CHECK(QivenContext::write_to_cognition(w, resolveNoText, *g2).reason == RefusalReason::InvariantFailed);
        ContextTransaction resolve;
        resolve.base = w->revision;
        resolve.operations.push_back(Operation { .kind    = Operation::Kind::ResolveConflict,
                                                 .scope   = "",
                                                 .title   = "state-wording",
                                                 .payload = "human view with --verbose is canonical" });
        QCD_CHECK(QivenContext::write_to_cognition(w, resolve, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        // the proposer re-proposes at the new revision: same reviewed delta
        const auto w                 = fresh();
        ContextTransaction unblocked = decisionTx(w->revision, 50, true, "blocked by conflict");
        QCD_CHECK(QivenContext::write_to_cognition(w, unblocked, *g2).outcome == Verdict::Outcome::Applied);
    }

    // pit.history_append_only + reciprocal acyclic supersession
    {
        const auto w                = fresh();
        ContextTransaction accept60 = decisionTx(w->revision, 60, true, "first decision");
        QCD_CHECK(QivenContext::write_to_cognition(w, accept60, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w                = fresh();
        ContextTransaction accept61 = decisionTx(w->revision, 61, true, "successor decision");
        QCD_CHECK(QivenContext::write_to_cognition(w, accept61, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction supersede;
        supersede.base               = w->revision;
        supersede.handoffEvidenceRef = "PR-record review"; // supersede is merge-class
        supersede.operations.push_back(Operation { .kind              = Operation::Kind::SupersedeDecision,
                                                   .recordId          = 60,
                                                   .successorRecordId = 61,
                                                   .payload           = "replaced by the successor" });
        supersede.h2 = H2Evidence { digest_operations(supersede.operations),
                                    "github:JasonHuang3D", "jason-brother-glm5-3", "PR-record review" };
        QCD_CHECK(QivenContext::write_to_cognition(w, supersede, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w         = fresh();
        bool foundSuperseded = false;
        for (const auto& decision : w->state->decisions)
        {
            if (decision.id == 60)
            {
                foundSuperseded = decision.status == Lifecycle::Superseded && decision.supersededBy.size() == 1 && decision.supersededBy[0] == 61;
            }
            if (decision.id == 61)
            {
                foundSuperseded = foundSuperseded && decision.supersedes.size() == 1 && decision.supersedes[0] == 60;
            }
        }
        QCD_CHECK(foundSuperseded); // history kept, links reciprocal
        ContextTransaction selfSupersede;
        selfSupersede.base               = w->revision;
        selfSupersede.handoffEvidenceRef = "PR-record review";
        selfSupersede.operations.push_back(Operation { .kind              = Operation::Kind::SupersedeDecision,
                                                       .recordId          = 61,
                                                       .successorRecordId = 61,
                                                       .payload           = "self" });
        selfSupersede.h2 = H2Evidence { digest_operations(selfSupersede.operations),
                                        "github:JasonHuang3D", "jason-brother-glm5-3", "PR-record review" };
        QCD_CHECK(QivenContext::write_to_cognition(w, selfSupersede, *g2).reason == RefusalReason::InvariantFailed);
    }
    {
        const auto w = fresh();
        ContextTransaction noProvenance;
        noProvenance.base = w->revision;
        noProvenance.operations.push_back(Operation { .kind    = Operation::Kind::AddMemory,
                                                      .title   = "orphan",
                                                      .payload = "no provenance" });
        QCD_CHECK(QivenContext::write_to_cognition(w, noProvenance, *g2).reason == RefusalReason::InvariantFailed);
    }
    {
        // negative knowledge is a SEMANTIC floor: rejected alternatives surface
        // in every bundle regardless of budget (constitution #7, review §8)
        const auto w = fresh();
        ContextTransaction negative;
        negative.base = w->revision;
        negative.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                  .aux           = 5, // MemoryRecord::Kind::NegativeKnowledge
                                                  .title         = "rejected alternative",
                                                  .payload       = "event-sourcing was rejected: state-replication only",
                                                  .provenanceRef = "ADR-0033-alternatives" });
        QCD_CHECK(QivenContext::write_to_cognition(w, negative, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction addEvidence;
        addEvidence.base = w->revision;
        addEvidence.operations.push_back(Operation { .kind    = Operation::Kind::AddEvidence,
                                                     .scope   = "",
                                                     .title   = "snap-2709358ddc94d185",
                                                     .payload = "genesis golden vector pinned" });
        QCD_CHECK(QivenContext::write_to_cognition(w, addEvidence, *g2).outcome == Verdict::Outcome::Applied);
    }
    QCD_CHECK(!fresh()->state->evidence.empty());

    // --- Phase 2b: state coherence, views, bundles (DR-007/DR-008, P-15/P-40) --

    // pit.state_references_resolve (P-15): the next boundary binds to OPEN work
    {
        const auto w = fresh();
        ContextTransaction upsertBoundary;
        upsertBoundary.base = w->revision;
        upsertBoundary.operations.push_back(Operation { .kind     = Operation::Kind::UpsertObligation,
                                                        .recordId = 9,
                                                        .payload  = "the next boundary obligation" });
        QCD_CHECK(QivenContext::write_to_cognition(w, upsertBoundary, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction setBoundary;
        setBoundary.base = w->revision;
        setBoundary.operations.push_back(
            Operation { .kind = Operation::Kind::SetNextBoundary, .recordId = 9 });
        QCD_CHECK(QivenContext::write_to_cognition(w, setBoundary, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction dangling;
        dangling.base = w->revision;
        dangling.operations.push_back(
            Operation { .kind = Operation::Kind::CloseObligation, .recordId = 9 });
        QCD_CHECK(QivenContext::write_to_cognition(w, dangling, *g2).reason == RefusalReason::InvariantFailed); // dangling boundary reference
    }

    // pit.view_refs_resolve / pit.view_never_invented (DR-008, P-40/P-41):
    // profile records are first-class and views resolve against them
    {
        const auto w = fresh();
        ContextTransaction upsertProfile;
        upsertProfile.base = w->revision;
        upsertProfile.operations.push_back(Operation { .kind    = Operation::Kind::UpsertProfile,
                                                       .scope   = "environment",
                                                       .title   = "views/environments/jasonpc",
                                                       .payload = "JasonPC durable environment profile" });
        upsertProfile.operations.push_back(Operation { .kind    = Operation::Kind::UpsertProfile,
                                                       .scope   = "workflow",
                                                       .title   = "views/workflows/local-supervised-agent",
                                                       .payload = "supervised local agent workflow profile" });
        QCD_CHECK(QivenContext::write_to_cognition(w, upsertProfile, *g2).outcome == Verdict::Outcome::Applied);
    }
    {
        const auto w = fresh();
        ContextTransaction badView;
        badView.base = w->revision;
        badView.operations.push_back(Operation { .kind          = Operation::Kind::AmendViewSpec,
                                                 .scope         = "ZCode:Jason",
                                                 .title         = "zcode-jason",
                                                 .payload       = "supervised local agent adaptation",
                                                 .provenanceRef = "views/environments/jasonpc,views/does/not-exist" });
        QCD_CHECK(QivenContext::write_to_cognition(w, badView, *g2).reason == RefusalReason::InvariantFailed); // dangling profile ref (review §7)
    }
    {
        const auto w = fresh();
        ContextTransaction goodView;
        goodView.base = w->revision;
        goodView.operations.push_back(Operation { .kind          = Operation::Kind::AmendViewSpec,
                                                  .scope         = "ZCode:Jason",
                                                  .title         = "zcode-jason",
                                                  .payload       = "supervised local agent adaptation",
                                                  .provenanceRef = "views/environments/jasonpc,views/workflows/local-supervised-agent" });
        QCD_CHECK(QivenContext::write_to_cognition(w, goodView, *g2).outcome == Verdict::Outcome::Applied);
    }
    ResolveDiagnostic diagnostic;
    {
        const auto w            = fresh();
        const auto resolvedView = QivenContext::ResolveView(w, "zcode-jason", &diagnostic);
        QCD_CHECK(resolvedView.has_value() && diagnostic.kind == ResolveDiagnostic::Kind::None);
        QCD_CHECK(resolvedView->human == "Jason");
        QCD_CHECK(!QivenContext::ResolveView(w, "chatgpt-jason", &diagnostic).has_value());
        QCD_CHECK(diagnostic.kind == ResolveDiagnostic::Kind::NotFound); // never invented
    }

    // pit.bundle_floor_survives_budget (review §8): SEMANTIC floors survive —
    // real content, not counts; the budget shrinks candidates only
    {
        const auto w           = fresh();
        const auto starved     = QivenContext::BuildBundle(w, Query { "task", 1 });
        bool floorConstitution = false;
        bool floorGovernance   = false;
        bool floorBoundary     = false;
        for (const auto& floor : starved.mandatoryInputs)
        {
            if (floor.find("constitution: History is not overwritten") != std::string::npos)
            {
                floorConstitution = true; // the ACTUAL article, not a count
            }
            if (floor.find("governance: github:JasonHuang3D") != std::string::npos)
            {
                floorGovernance = true;
            }
            if (floor.find("next boundary: obligation 9") != std::string::npos)
            {
                floorBoundary = true; // the boundary obligation's STATEMENT is in the floor
            }
        }
        QCD_CHECK(floorConstitution && floorGovernance && floorBoundary);
        QCD_CHECK(!starved.protectedConstraints.empty());
        // (shrunk-to-budget asserted after `rich` below)
        QCD_CHECK(!starved.omissions.empty()); // the shrink is EXPLAINED
        const auto rich = QivenContext::BuildBundle(w, Query { "task", 0 });
        QCD_CHECK(starved.candidates.size() < rich.candidates.size()); // shrunk
        QCD_CHECK(rich.candidates.size() > starved.candidates.size());
        QCD_CHECK(rich.mandatoryInputs.size() == starved.mandatoryInputs.size());
        bool floorNegativeKnowledge = false;
        for (const auto& floor : rich.mandatoryInputs)
        {
            if (floor.find("negative knowledge: event-sourcing was rejected") != std::string::npos)
            {
                floorNegativeKnowledge = true;
            }
        }
        QCD_CHECK(floorNegativeKnowledge);

        // pit.candidate_not_truth: candidates are typed evidence; a bundle never
        // grants authorization; the machine render is deterministic
        const auto rendered = QivenContext::RenderBundle(starved, OutputView::Machine);
        QCD_CHECK(rendered.find("authorization: not_granted") != std::string::npos);
        QCD_CHECK(rendered == QivenContext::RenderBundle(starved, OutputView::Machine));
        for (const auto& candidate : starved.candidates)
        {
            QCD_CHECK(candidate.kind == "decision" || candidate.kind == "memory");
        }
        QCD_CHECK(!QivenContext::RenderBundle(starved, OutputView::Human).empty());
    }
    QivenContext::release_grant(*g2);

    // pit.restored_never_self_promotes: artifact restore quarantines —
    // cognition without write authority, store untouched
    const auto c5        = fresh();
    const Bytes artifact = QivenContext::read_from_cognition(c5, Query {});
    CognitionSource artifactSource;
    artifactSource.kind           = CognitionSourceKind::HandoffArtifact;
    artifactSource.inlineBytes    = artifact;
    artifactSource.expectedDigest = draft_content_id(artifact);
    const auto restored           = QivenContext::create_cognition(artifactSource);
    QCD_CHECK(restored != nullptr);
    QCD_CHECK(restored->quarantine == QuarantineState::Isolated);
    QCD_CHECK(restored->digest == c5->digest); // same content, same integrity identity
    QCD_CHECK(IsEmpty(restored->revision));    // a quarantined artifact has no storage identity
    const auto rg = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(rg.has_value());
    ContextTransaction quarantinedWrite = lessonTx(restored->revision);
    QCD_CHECK(QivenContext::write_to_cognition(restored, quarantinedWrite, *rg).reason == RefusalReason::GovernanceDenied);
    QCD_CHECK(store->head() == c5->revision); // the store was never touched
    QivenContext::release_grant(*rg);

    // pit.corrupt_artifact_fails_closed (P-23)
    DeserializeError error;
    Bytes corrupted = artifact;
    corrupted[corrupted.size() / 2] ^= std::byte { 0xFF };
    CognitionSource corruptedSource;
    corruptedSource.kind           = CognitionSourceKind::HandoffArtifact;
    corruptedSource.inlineBytes    = corrupted;
    corruptedSource.expectedDigest = draft_content_id(artifact);
    QCD_CHECK(QivenContext::create_cognition(corruptedSource, &error) == nullptr);
    QCD_CHECK(error.kind == DeserializeError::Kind::DigestMismatch);

    // pit.resource_abuse_fails_closed (DR-009): bounded reserves, typed error
    Bytes abusive;
    abusive.push_back(std::byte { 5 }); // serialization version
    for (unsigned i = 0; i < 4; ++i)
    {
        abusive.push_back(std::byte { 0 }); // empty root principal string
    }
    const auto huge = static_cast<std::uint32_t>(0xFFFFFFFFU);
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        abusive.push_back(std::byte { static_cast<unsigned char>((huge >> shift) & 0xFFU) });
    }
    CognitionSource abusiveSource;
    abusiveSource.kind           = CognitionSourceKind::HandoffArtifact;
    abusiveSource.inlineBytes    = abusive;
    abusiveSource.expectedDigest = draft_content_id(abusive);
    QCD_CHECK(QivenContext::create_cognition(abusiveSource, &error) == nullptr);
    QCD_CHECK(error.kind == DeserializeError::Kind::ResourceAbuse);

    // retire: lifetime split — the service drops ownership; pinned handles
    // stay alive but fenced (v1's conflation, split in v2, typed in v3)
    const auto retired = fresh();
    QCD_CHECK(QivenContext::retire_cognition(retired));
    QCD_CHECK(!QivenContext::retire_cognition(retired)); // already retired
    QCD_CHECK(!retired->state->memory.empty());          // pinned object alive

    // --- Phase 3: session economics, process types, quarantine machine --------

    // pit.checkpoint_tracks_transactions (P-16): the checkpoint names WHY it
    // was written and records both sides of a turn
    QCD_CHECK(!llm->checkpoint.acceptedRefs.empty());
    QCD_CHECK(llm->checkpoint.lastTrigger == CheckpointTrigger::MaterialTransaction);
    QCD_CHECK(!llm->checkpoint.unacceptedCandidates.empty());
    QCD_CHECK(!llm->checkpoint.servingDisclosure.empty());
    QCD_CHECK(llm->turnBudget.soft == std::chrono::seconds(26 * 60)); // the observed boundary

    // the quarantine state machine is FAIL-CLOSED (review §10, S10-R2):
    // verify_restored moves Isolated -> Verified (root principal only);
    // promote_authority refuses everything but AuthorityPending — checksum
    // success, import or uptime never promote; AuthorityPending is only
    // reachable via a canonical cutover ADR (out of draft scope by design)
    const Bytes artifact2 = QivenContext::read_from_cognition(fresh(), Query {});
    CognitionSource artifactSource2;
    artifactSource2.kind           = CognitionSourceKind::HandoffArtifact;
    artifactSource2.inlineBytes    = artifact2;
    artifactSource2.expectedDigest = draft_content_id(artifact2);
    const auto isolated            = QivenContext::create_cognition(artifactSource2);
    QCD_CHECK(isolated != nullptr && isolated->quarantine == QuarantineState::Isolated);
    const AuthenticatedActor rootActor { "github:JasonHuang3D", Role::Owner, "owner-session",
                                         "root-principal", "n/a" };
    QCD_CHECK(QivenContext::promote_authority(isolated, rootActor) == nullptr); // not AuthorityPending
    const auto verified = QivenContext::verify_restored(isolated, rootActor);
    QCD_CHECK(verified != nullptr && verified->quarantine == QuarantineState::Verified);
    QCD_CHECK(verified->digest == isolated->digest);                            // same content, new quarantine state
    QCD_CHECK(QivenContext::promote_authority(verified, rootActor) == nullptr); // still fail-closed
    QCD_CHECK(QivenContext::verify_restored(isolated, stranger()) == nullptr);  // stranger refused

    // --- review §9/§10 + S7-R3: the RUNTIME REBIRTH test -----------------------
    // Generation A ran above: Human/LLM/Client/Device were created, used, and
    // will now be DESTROYED. Only durable cognition (the store) survives.
    {
        const auto headCognition          = QivenContext::create_cognition(bootFromHead());
        const std::size_t decisionsBefore = headCognition->state->decisions.size();
        static_cast<void>(decisionsBefore);

        // Generation A: a full participant graph, used, then destroyed
        auto humanA               = std::make_shared<Human>();
        humanA->name              = "Jason";
        humanA->verifiedPrincipal = "github:JasonHuang3D";
        auto llmA                 = std::make_shared<LLM>();
        llmA->name                = "GLM-5.3-Flash";
        llmA->pCognition          = headCognition;
        auto deviceA              = std::make_shared<Device>();
        auto clientA              = std::make_shared<LLMClientTool>();
        clientA->pCurrentLLM      = llmA;
        clientA->pTargetDevice    = deviceA;
        clientA->binding          = ParticipantBinding { Role::Worker, "GLM-5.3-Flash" };

        // graph-edge validation (review §9): a MISMATCHED graph is not
        // continueable even when every node is individually healthy
        auto llmB        = std::make_shared<LLM>();
        llmB->name       = "GLM-5.3-Flash";
        llmB->pCognition = headCognition;
        auto deviceB     = std::make_shared<Device>();
        QCD_CHECK(!QivenContext::is_continueable(
            humanA.get(), clientA.get(), deviceB.get(), llmA.get(), headCognition));
        QCD_CHECK(!QivenContext::is_continueable(
            humanA.get(), clientA.get(), deviceA.get(), llmB.get(), headCognition));
        QCD_CHECK(QivenContext::is_continueable(
            humanA.get(), clientA.get(), deviceA.get(), llmA.get(), headCognition));

        // disclosure consistency (review §10): the actor names the ACTUAL
        // bound model; a rebind cannot drift the disclosure
        std::string rebirthResult;
        llmA->deltaGenerator = [](const std::string&, const Snapshot&) {
            ContextTransaction transaction;
            transaction.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                         .title         = "rebirth",
                                                         .payload       = "generation A wrote this",
                                                         .provenanceRef = "runtime-rebirth-test" });
            return transaction;
        };
        humanA->UseLLMToWork(clientA, "generation A write", rebirthResult);
        QCD_CHECK(rebirthResult.find("delta applied") != std::string::npos);
    } // every Generation-A participant is DESTROYED here; the store survives

    // Generation B: entirely fresh participants, same durable cognition
    const auto headB          = QivenContext::create_cognition(bootFromHead());
    auto humanB               = std::make_shared<Human>();
    humanB->name              = "Jason";
    humanB->verifiedPrincipal = "github:JasonHuang3D";
    auto llmB2                = std::make_shared<LLM>();
    llmB2->name               = "FutureModel-B"; // participant CLASS substitution
    llmB2->pCognition         = headB;
    auto deviceB2             = std::make_shared<Device>();
    deviceB2->name            = "OtherDevice";
    deviceB2->os              = "Linux";
    auto clientB2             = std::make_shared<LLMClientTool>();
    clientB2->pCurrentLLM     = llmB2;
    clientB2->pTargetDevice   = deviceB2;
    clientB2->binding         = ParticipantBinding { Role::Worker, "FutureModel-B" };

    // continuity: everything Generation A wrote is present, unchanged
    bool foundRebirth = false;
    for (const auto& record : headB->state->memory)
    {
        if (record.title == "rebirth" && record.statement == "generation A wrote this")
        {
            foundRebirth = record.provenance.sources.size() == 1;
        }
    }
    QCD_CHECK(foundRebirth);
    QCD_CHECK(!headB->state->decisions.empty());
    QCD_CHECK(!headB->state->conflicts.empty()); // resolved conflicts survive too

    // and Generation B can continue the work: cognition outlived ALL
    // participants — S7-R3 is now an executable proof, not an assertion
    std::string continueResult;
    llmB2->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                     .title         = "rebirth-continue",
                                                     .payload       = "generation B continues",
                                                     .provenanceRef = "runtime-rebirth-test" });
        return transaction;
    };
    humanB->UseLLMToWork(clientB2, "generation B continue", continueResult);
    QCD_CHECK(continueResult.find("delta applied") != std::string::npos);

    std::printf("[ OK ] persistence fencing gates + pit regression suite + rebirth\n");
}
