// ============================================================================
// persistence_fencing — the admission gates in normative order, with the pit
// regression suite from docs/architecture/pit-regression-map.md:
//   split-brain (P-01), unverified actor (P-02), grant forgery, reader boot
//   preserves writer (P-25 family / DR-010), stale base, content-bound H2
//   (P-05), handoff-missing never retried, unattended read-only (P-08),
//   invariants, quarantine (P-33), corruption fails closed (P-23),
//   resource abuse, retire/lifetime split.
// ============================================================================

#include <qiven/context/context.hpp>

#include "detail/check.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace qiven::context;

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
                                "jason-worker-glm5-3-flash", "glm-5.3-flash", "standard" };
}

AuthenticatedActor stranger()
{
    return AuthenticatedActor { "github:SomeoneElse", Role::Worker, "x", "some-model", "standard" };
}

ContextTransaction lessonTx(const ContentId& base)
{
    ContextTransaction transaction;
    transaction.base = base;
    transaction.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                 .title         = "lesson",
                                                 .payload       = "gate order is normative",
                                                 .provenanceRef = "v3-phase2-session" });
    return transaction;
}

ContextTransaction decisionTx(const ContentId& base, std::int64_t id, bool withEvidence,
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
        evidence.reviewer            = "github:JasonHuang3D";
        evidence.reviewRef           = "brother-review";
        evidence.reviewedDeltaDigest = DigestOperations(transaction.operations);
        transaction.h2               = evidence;
    }
    return transaction;
}

// pit.port_never_reenters_service (P-42): the identity port receives the
// governance snapshot as context. Before the 2026-09-19 fix, the draft
// verifier called CanonicalHead() itself and re-locked the admission mutex on
// the same thread — MSVC Debug throws resource_deadlock_would_occur, uncaught,
// terminating the process silently (exit 3).
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
} // namespace

int main()
{
    const auto store = freshStore();
    DeserializeError bootErr;
    const auto c1 = QivenContext::CreateCognition(bootFromHead(), &bootErr); // genesis, epoch 1
    if (!c1)
    {
        std::printf("boot failed: kind=%d offset=%zu detail=%s\n", static_cast<int>(bootErr.kind),
                    bootErr.offset, bootErr.detail.c_str());
    }
    QCD_CHECK(c1 != nullptr);
    QCD_CHECK(c1->quarantine == QuarantineState::Promoted);
    QCD_CHECK(!c1->state->policy.handoff.empty()); // authority rules live in cognition (R4)

    // golden vector: pins the v2 genesis serialization format
    std::printf("genesis content id: %s\n", c1->contentId.c_str());
    QCD_CHECK(c1->contentId == "draft-92f97647488b7c58"); // v3: conflicts + full policy table

    // pit.port_never_reenters_service (P-42): the port is invoked with the
    // governance snapshot supplied by the service — never re-enters
    const auto recorder = std::make_shared<RecordingVerifier>();
    QivenContext::attachIdentityVerifier(recorder);
    const auto portGrant = QivenContext::AcquireGrant(goodActor(), WorkMode::SupervisedForeground);
    QCD_CHECK(portGrant.has_value());
    QCD_CHECK(recorder->called);
    QCD_CHECK(recorder->sawGovernance);
    QCD_CHECK(!QivenContext::AcquireGrant(stranger(), WorkMode::SupervisedForeground).has_value());
    QivenContext::ReleaseGrant(*portGrant);
    QivenContext::attachIdentityVerifier(nullptr); // restore the default port

    // pit.unverified_actor_refused: identity is verified at the port, never asserted
    QCD_CHECK(!QivenContext::AcquireGrant(stranger(), WorkMode::SupervisedForeground).has_value());

    const auto actor = goodActor();
    const auto g1    = QivenContext::AcquireGrant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g1.has_value());

    // pit.split_brain_second_flow_refused: a second acquire is refused
    // fail-closed — even for the SAME actor; flows are never queued
    QCD_CHECK(!QivenContext::AcquireGrant(actor, WorkMode::SupervisedForeground).has_value());
    QCD_CHECK(!QivenContext::AcquireGrant(stranger(), WorkMode::SupervisedForeground).has_value());

    // write with the active grant → applied; the durable token advances
    const auto applied = QivenContext::WriteToCognition(c1, lessonTx(c1->contentId), *g1);
    QCD_CHECK(applied.outcome == Verdict::Outcome::Applied);
    const ContentId head1 = applied.successorId;
    QCD_CHECK(store->head() == head1);

    // pit.grant forgery: a fabricated grant is refused; stale grants never
    // regain authority
    const ExecutionGrant forged { g1->epoch, stranger().principal, WorkMode::SupervisedForeground };
    QCD_CHECK(QivenContext::WriteToCognition(c1, lessonTx(c1->contentId), forged).reason == RefusalReason::GrantRefused);

    // pit.reader_boot_preserves_writer (DR-010): re-materializing for reading
    // must NOT revoke the active writer — the v2 epoch check did exactly that
    const auto c2 = QivenContext::CreateCognition(bootFromHead());
    QCD_CHECK(c2 != nullptr);
    QCD_CHECK(c2->epoch == c1->epoch + 1);
    QCD_CHECK(c2->contentId == head1);
    QCD_CHECK(QivenContext::WriteToCognition(c1, lessonTx(c1->contentId), *g1).outcome == Verdict::Outcome::Applied);

    // stale base → StaleBase: durable divergence, re-read before writing
    const auto stale =
        QivenContext::WriteToCognition(c2, lessonTx(ContentId { "draft-0000000000000000" }), *g1);
    QCD_CHECK(stale.reason == RefusalReason::StaleBase);
    QCD_CHECK(QivenContext::RecoveryFor(*c1->state, RefusalReason::StaleBase) == RecoveryAction::RereadRethink);

    // --- DR-011: idempotency keys and durable receipts ------------------------

    ContextTransaction keyed = lessonTx(c1->contentId);
    keyed.idempotencyKey     = "txn-alpha-001";
    const auto keyedApply    = QivenContext::WriteToCognition(c1, keyed, *g1);
    QCD_CHECK(keyedApply.outcome == Verdict::Outcome::Applied);
    const ContentId keyedHead      = keyedApply.successorId;
    const std::size_t memoryBefore = c1->state->memory.size();

    // pit.same_key_one_outcome: the client re-sends the EXACT original request
    // (original base included) after an ambiguous outcome — the key resolves to
    // the original verdict without re-executing, even though the head moved
    const auto replayed = QivenContext::WriteToCognition(c1, keyed, *g1);
    QCD_CHECK(replayed.outcome == Verdict::Outcome::Applied);
    QCD_CHECK(replayed.successorId == keyedHead);        // original result
    QCD_CHECK(c1->state->memory.size() == memoryBefore); // nothing re-executed

    // pit.same_key_different_content_refused: same key, different request
    ContextTransaction conflicting    = keyed;
    conflicting.operations[0].payload = "a DIFFERENT request under the same key";
    QCD_CHECK(QivenContext::WriteToCognition(c1, conflicting, *g1).reason == RefusalReason::KeyConflict);

    // pit.timeout_yields_unknown_not_rollback: a lost store acknowledgement
    // after a possibly-successful commit is OutcomeUnknown — never a rollback
    // claim; dependent work stops; the same key resolves to the same receipt
    class FaultInjectionStore final : public ICognitionStore
    {
    public:
        explicit FaultInjectionStore(std::shared_ptr<ICognitionStore> inner) :
        inner_(std::move(inner))
        {
        }
        bool loseNextAck { false };
        [[nodiscard]] ContentId append(const Bytes& stateBytes, const ContentId& base) override
        {
            const ContentId id = inner_->append(stateBytes, base);
            if (loseNextAck)
            {
                loseNextAck = false;
                return {}; // committed above, acknowledgement lost
            }
            return id;
        }
        [[nodiscard]] Bytes materialize(const ContentId& id) const override
        {
            return inner_->materialize(id);
        }
        [[nodiscard]] Bytes readDelta(const ContentId& base, const ContentId& target) const override
        {
            return inner_->readDelta(base, target);
        }
        [[nodiscard]] bool verify(const ContentId& id) const override
        {
            return inner_->verify(id);
        }
        [[nodiscard]] ContentId head() const override
        {
            return inner_->head();
        }

    private:
        std::shared_ptr<ICognitionStore> inner_;
    };
    auto flaky = std::make_shared<FaultInjectionStore>(store);
    QivenContext::attachStore(flaky);
    flaky->loseNextAck      = true;
    ContextTransaction lost = lessonTx(c1->contentId);
    lost.idempotencyKey     = "txn-beta-002";
    const auto unknown      = QivenContext::WriteToCognition(c1, lost, *g1);
    QCD_CHECK(unknown.outcome == Verdict::Outcome::OutcomeUnknown);
    QCD_CHECK(QivenContext::RecoveryFor(*c1->state, RefusalReason::OutcomeUnresolved) == RecoveryAction::Block);
    const auto resolved = QivenContext::WriteToCognition(c1, lost, *g1); // verbatim re-send
    QCD_CHECK(resolved.outcome == Verdict::Outcome::OutcomeUnknown);     // receipt, not re-execution
    QCD_CHECK(store->head() != c1->contentId);                           // the commit HAD landed; no rollback claim
    QivenContext::attachStore(store);                                    // restore the honest store

    // empty transaction = ordinary turn: applied, nothing stored
    ContextTransaction ordinary;
    ordinary.base = c1->contentId;
    QCD_CHECK(QivenContext::WriteToCognition(c1, ordinary, *g1).outcome == Verdict::Outcome::Applied);
    QivenContext::ReleaseGrant(*g1);
    const ContentId head2 = store->head();

    // --- Work-cycle level: policy table + content-bound H2 evidence (P-05) ----

    // a fresh reader boot at the current head carries the Work section; c1's
    // writes above advanced the head past c2 (c2 stays stale ON PURPOSE — the
    // store-level CAS is a separate publish-time fence from the base gate)
    const auto c3 = QivenContext::CreateCognition(bootFromHead());
    QCD_CHECK(c3 != nullptr);
    QCD_CHECK(c3->contentId == head2);

    Human human { "Jason", {}, "github:JasonHuang3D" };
    auto llm              = std::make_shared<LLM>();
    llm->name             = "GLM-5.3-Flash";
    llm->pCognition       = c3;
    auto device           = std::make_shared<Device>();
    device->name          = "JasonPC";
    auto client           = std::make_shared<LLMClientTool>();
    client->name          = "zcode-desktop";
    client->pCurrentLLM   = llm;
    client->pTargetDevice = device;
    client->binding       = ParticipantBinding { Role::Worker, "glm-5.3-flash" };

    // pit.handoff_missing_never_retried: merge-class without H2 → refused; the
    // recovery table mandates HaltEscalate; the delta is NOT parked for retry
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

    // with content-bound H2 evidence → applied
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        return decisionTx(ContentId {}, 37, true, "typed human handoffs");
    };
    human.UseLLMToWork(client, "accept with review", result);
    QCD_CHECK(result.find("delta applied") != std::string::npos);
    QCD_CHECK(c3->contentId != head2);

    // pit.h2_evidence_rebind_refused: the payload changed AFTER the review —
    // the digest no longer binds (MEM-20260913T162546Z-E73124)
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = 38,
                                                     .title    = "ADR-0037",
                                                     .payload  = "executable specification" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer  = "github:JasonHuang3D";
        evidence.reviewRef = "brother-review";
        std::vector<Operation> reviewed; // the delta AS REVIEWED — different payload
        reviewed.push_back(Operation { .kind = Operation::Kind::AppendDecision, .recordId = 38, .title = "ADR-0037", .payload = "a DIFFERENT payload" });
        evidence.reviewedDeltaDigest = DigestOperations(reviewed);
        transaction.h2               = evidence;
        return transaction;
    };
    human.UseLLMToWork(client, "accept with stale review", result);
    QCD_CHECK(result.find("handoff-invalid") != std::string::npos);
    QCD_CHECK(llm->checkpoint.nextAction.find("halt") != std::string::npos);

    // pit invariant: duplicate decision id → InvariantFailed → DesignReview
    // (constitution 17: known hazard class, not a patch loop)
    llm->deltaGenerator = [](const std::string&, const Snapshot& snapshot) {
        const std::int64_t taken = snapshot.decisions.empty() ? 1 : snapshot.decisions.back().id;
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                     .recordId = taken,
                                                     .title    = "dup",
                                                     .payload  = "duplicate id" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:JasonHuang3D";
        evidence.reviewRef           = "brother-review";
        evidence.reviewedDeltaDigest = DigestOperations(transaction.operations);
        transaction.h2               = evidence;
        return transaction;
    };
    human.UseLLMToWork(client, "duplicate decision id", result);
    QCD_CHECK(result.find("invariant-failed") != std::string::npos);
    QCD_CHECK(llm->checkpoint.nextAction.find("design review") != std::string::npos);

    // pit.unattended_mutation_refused: unattended grants carry no write right
    const auto ug = QivenContext::AcquireGrant(actor, WorkMode::Unattended);
    QCD_CHECK(ug.has_value());
    QCD_CHECK(QivenContext::WriteToCognition(c3, lessonTx(c3->contentId), *ug).reason == RefusalReason::UnattendedMutation);
    QivenContext::ReleaseGrant(*ug);

    // --- Phase 2: conflicts, lifecycle, evidence (DR-006, pit P-14/P-36/P-37) --

    // pit.open_conflict_blocks_acceptance: an OPEN global conflict blocks a
    // merge-class acceptance even when its H2 evidence is valid — the K4
    // trial-1 lesson as a compiled gate
    const auto g2 = QivenContext::AcquireGrant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g2.has_value());
    ContextTransaction openConflict;
    openConflict.base = c3->contentId;
    openConflict.operations.push_back(Operation { .kind    = Operation::Kind::OpenConflict,
                                                  .title   = "state-wording",
                                                  .payload = "producer invocation wording disagrees" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, openConflict, *g2).outcome == Verdict::Outcome::Applied);
    ContextTransaction blockedDecision = decisionTx(c3->contentId, 50, true, "blocked by conflict");
    const auto blocked                 = QivenContext::WriteToCognition(c3, blockedDecision, *g2);
    QCD_CHECK(blocked.reason == RefusalReason::ConflictUnresolved);
    QCD_CHECK(QivenContext::RecoveryFor(*c3->state, RefusalReason::ConflictUnresolved) == RecoveryAction::FailClosed);
    // a scoped conflict would not block a differently-scoped operation; a
    // global one (empty scope) blocks everything — this one is global
    ContextTransaction unrelatedMemory;
    unrelatedMemory.base = c3->contentId;
    unrelatedMemory.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                     .scope         = "retrieval",
                                                     .title         = "unrelated",
                                                     .payload       = "global conflicts still block",
                                                     .provenanceRef = "v3-phase2-session" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, unrelatedMemory, *g2).reason == RefusalReason::ConflictUnresolved);

    // resolving with a rationale re-opens acceptance; resolution text required
    ContextTransaction resolveNoText;
    resolveNoText.base = c3->contentId;
    resolveNoText.operations.push_back(Operation { .kind  = Operation::Kind::ResolveConflict,
                                                   .title = "state-wording" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, resolveNoText, *g2).reason == RefusalReason::InvariantFailed);
    ContextTransaction resolve;
    resolve.base = c3->contentId;
    resolve.operations.push_back(Operation { .kind    = Operation::Kind::ResolveConflict,
                                             .title   = "state-wording",
                                             .payload = "human view with --verbose is canonical" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, resolve, *g2).outcome == Verdict::Outcome::Applied);
    // the proposer re-proposes at the new head: same reviewed delta (the H2
    // digest binds to the operations), fresh base
    const auto unblocked = decisionTx(c3->contentId, 50, true, "blocked by conflict");
    QCD_CHECK(QivenContext::WriteToCognition(c3, unblocked, *g2).outcome == Verdict::Outcome::Applied);

    // pit.history_append_only + reciprocal acyclic supersession
    ContextTransaction accept60 = decisionTx(c3->contentId, 60, true, "first decision");
    QCD_CHECK(QivenContext::WriteToCognition(c3, accept60, *g2).outcome == Verdict::Outcome::Applied);
    ContextTransaction accept61 = decisionTx(c3->contentId, 61, true, "successor decision");
    QCD_CHECK(QivenContext::WriteToCognition(c3, accept61, *g2).outcome == Verdict::Outcome::Applied);
    ContextTransaction supersede;
    supersede.base               = c3->contentId;
    supersede.handoffEvidenceRef = "PR-record review"; // supersede is merge-class
    supersede.operations.push_back(Operation { .kind              = Operation::Kind::SupersedeDecision,
                                               .recordId          = 60,
                                               .successorRecordId = 61,
                                               .payload           = "replaced by the successor" });
    supersede.h2 = H2Evidence { DigestOperations(supersede.operations),
                                "github:JasonHuang3D", "brother-review" };
    QCD_CHECK(QivenContext::WriteToCognition(c3, supersede, *g2).outcome == Verdict::Outcome::Applied);
    bool foundSuperseded = false;
    for (const auto& decision : c3->state->decisions)
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
    selfSupersede.base               = c3->contentId;
    selfSupersede.handoffEvidenceRef = "PR-record review";
    selfSupersede.operations.push_back(Operation { .kind              = Operation::Kind::SupersedeDecision,
                                                   .recordId          = 61,
                                                   .successorRecordId = 61,
                                                   .payload           = "self" });
    selfSupersede.h2 = H2Evidence { DigestOperations(selfSupersede.operations),
                                    "github:JasonHuang3D", "brother-review" };
    QCD_CHECK(QivenContext::WriteToCognition(c3, selfSupersede, *g2).reason == RefusalReason::InvariantFailed);

    // pit.unprovenanced_record_refused (P-37): lessons carry provenance
    ContextTransaction noProvenance;
    noProvenance.base = c3->contentId;
    noProvenance.operations.push_back(Operation { .kind    = Operation::Kind::AddMemory,
                                                  .title   = "orphan",
                                                  .payload = "no provenance" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, noProvenance, *g2).reason == RefusalReason::InvariantFailed);

    // AddEvidence: evidence records are reachable at all (v2 write-path gap)
    ContextTransaction addEvidence;
    addEvidence.base = c3->contentId;
    addEvidence.operations.push_back(Operation { .kind    = Operation::Kind::AddEvidence,
                                                 .title   = "draft-92f97647488b7c58",
                                                 .payload = "genesis golden vector pinned" });
    QCD_CHECK(QivenContext::WriteToCognition(c3, addEvidence, *g2).outcome == Verdict::Outcome::Applied);
    QCD_CHECK(!c3->state->evidence.empty());
    QivenContext::ReleaseGrant(*g2);

    // pit.restored_never_self_promotes: artifact restore quarantines —
    // cognition without write authority, store untouched
    const Bytes artifact = QivenContext::ReadFromCognition(c3, Query {});
    CognitionSource artifactSource;
    artifactSource.kind           = CognitionSourceKind::HandoffArtifact;
    artifactSource.inlineBytes    = artifact;
    artifactSource.expectedDigest = DraftContentId(artifact);
    const auto restored           = QivenContext::CreateCognition(artifactSource);
    QCD_CHECK(restored != nullptr);
    QCD_CHECK(restored->quarantine == QuarantineState::Isolated);
    QCD_CHECK(restored->contentId == c3->contentId); // same content, same identity
    const auto rg = QivenContext::AcquireGrant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(rg.has_value());
    QCD_CHECK(QivenContext::WriteToCognition(restored, lessonTx(restored->contentId), *rg).reason == RefusalReason::GovernanceDenied);
    QCD_CHECK(store->head() == c3->contentId); // the store was never touched
    QivenContext::ReleaseGrant(*rg);

    // pit.corrupt_artifact_fails_closed (P-23): a flipped byte anywhere is a
    // digest mismatch — corruption is a failure, never a warning
    Bytes corrupted = artifact;
    corrupted[corrupted.size() / 2] ^= std::byte { 0xFF };
    CognitionSource corruptedSource;
    corruptedSource.kind           = CognitionSourceKind::HandoffArtifact;
    corruptedSource.inlineBytes    = corrupted;
    corruptedSource.expectedDigest = DraftContentId(artifact);
    DeserializeError error;
    QCD_CHECK(QivenContext::CreateCognition(corruptedSource, &error) == nullptr);
    QCD_CHECK(error.kind == DeserializeError::Kind::DigestMismatch);

    // pit.resource_abuse_fails_closed (DR-009): attacker-controlled counts are
    // rejected before allocation, with a typed error — in Debug AND Release.
    // Layout: version, empty root principal, then a huge article count.
    Bytes abusive;
    abusive.push_back(std::byte { 3 }); // serialization version
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
    abusiveSource.expectedDigest = DraftContentId(abusive);
    QCD_CHECK(QivenContext::CreateCognition(abusiveSource, &error) == nullptr);
    QCD_CHECK(error.kind == DeserializeError::Kind::ResourceAbuse);

    // retire: lifetime split — the service drops ownership; pinned handles
    // stay alive but fenced (v1's conflation, split in v2, typed in v3)
    QCD_CHECK(QivenContext::RetireCognition(c1));
    QCD_CHECK(!QivenContext::RetireCognition(c1));       // already retired
    QCD_CHECK(c1->state->memory.size() == memoryBefore); // pinned alive; the
                                                         // unknown outcome does NOT advance the in-memory
                                                         // state — only the next boot materializes the head

    std::printf("[ OK ] persistence fencing gates + pit regression suite\n");
}
