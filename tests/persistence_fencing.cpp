// ============================================================================
// persistence_fencing — the admission gates, in normative order:
//   0 unknown/stale/quarantined  1 durable base  2 typed handoff
//   3 unattended read-only  4 invariants  5 atomic apply + token advance
// ============================================================================

#include <qiven/context/context.hpp>

#include "detail/check.hpp"

#include <memory>

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

TransactionDelta lesson()
{
    TransactionDelta delta;
    delta.kind    = TransactionDelta::Kind::AddMemoryRecord;
    delta.title   = "lesson";
    delta.payload = "gate order is normative";
    return delta;
}
} // namespace

int main()
{
    const auto store = freshStore();
    const auto c1    = QivenContext::CreateCognition(bootFromHead()); // genesis, epoch 1

    TransactionDelta applied = lesson();
    applied.base             = c1->contentId;
    QCD_CHECK(QivenContext::WriteToCognition(c1.get(), applied, WorkMode::SupervisedForeground));

    // restore from head: epoch 2, carries the applied record
    const auto c2 = QivenContext::CreateCognition(bootFromHead());
    QCD_CHECK(c2->epoch == c1->epoch + 1);
    QCD_CHECK(QivenContext::currentEpoch() == c2->epoch);
    QCD_CHECK(c2->memory.size() == 1);

    // gate 0: stale runtime epoch — the world moved on, think again
    QCD_CHECK(!QivenContext::WriteToCognition(c1.get(), applied, WorkMode::SupervisedForeground));

    // gate 1: durable base mismatch — re-read before writing
    TransactionDelta staleBase = lesson();
    staleBase.base             = ContentId { "draft-deadbeefdeadbeef" };
    QCD_CHECK(!QivenContext::WriteToCognition(c2.get(), staleBase, WorkMode::SupervisedForeground));

    // gate 2: decision acceptance = merge-class semantics, H2 evidence mandatory
    TransactionDelta decision;
    decision.kind     = TransactionDelta::Kind::AppendDecision;
    decision.recordId = 37;
    decision.title    = "ADR-0036";
    decision.payload  = "typed human handoffs";
    decision.base     = c2->contentId;
    QCD_CHECK(!QivenContext::WriteToCognition(c2.get(), decision, WorkMode::SupervisedForeground));
    decision.handoff            = Handoff::H2_Review;
    decision.handoffEvidenceRef = "PR-record review";
    QCD_CHECK(QivenContext::WriteToCognition(c2.get(), decision, WorkMode::SupervisedForeground));
    QCD_CHECK(c2->decisions.size() == 1 && c2->decisions[0].id == 37);

    // gate 3: unattended default is read-only; an ordinary turn stays a no-op
    TransactionDelta mutate = lesson();
    mutate.base             = c2->contentId;
    QCD_CHECK(!QivenContext::WriteToCognition(c2.get(), mutate, WorkMode::Unattended));
    TransactionDelta ordinary;
    ordinary.kind = TransactionDelta::Kind::None;
    ordinary.base = c2->contentId;
    QCD_CHECK(QivenContext::WriteToCognition(c2.get(), ordinary, WorkMode::Unattended));

    // gate 4: invariants — decision ids are never reused
    TransactionDelta duplicate = decision;
    duplicate.base             = c2->contentId;
    QCD_CHECK(!QivenContext::WriteToCognition(c2.get(), duplicate, WorkMode::SupervisedForeground));

    // obligation lifecycle: open then close, history kept by status transition
    TransactionDelta upsert;
    upsert.kind     = TransactionDelta::Kind::UpsertObligation;
    upsert.recordId = 5;
    upsert.payload  = "design K5 against the K4 reference";
    upsert.base     = c2->contentId;
    QCD_CHECK(QivenContext::WriteToCognition(c2.get(), upsert, WorkMode::SupervisedForeground));
    TransactionDelta close;
    close.kind     = TransactionDelta::Kind::CloseObligation;
    close.recordId = 5;
    close.base     = c2->contentId;
    QCD_CHECK(QivenContext::WriteToCognition(c2.get(), close, WorkMode::SupervisedForeground));
    QCD_CHECK(c2->obligations[0].status == Obligation::Status::Done);

    // IsContinueable — authority, liveness and blockers (before any newer boot:
    // only the current epoch is continuable by definition)
    Human human { "Jason", {}, "github:JasonHuang3D" };
    auto llm        = std::make_shared<LLM>();
    llm->pCognition = c2;
    auto client     = std::make_shared<LLMClientTool>();
    auto device     = std::make_shared<Device>();
    QCD_CHECK(QivenContext::IsContinueable(&human, client.get(), device.get(), llm.get(),
                                           c2.get()));
    human.verifiedPrincipal = "not-the-principal";
    QCD_CHECK(!QivenContext::IsContinueable(&human, client.get(), device.get(), llm.get(),
                                            c2.get()));
    human.verifiedPrincipal = "github:JasonHuang3D";
    c2->obligations.push_back(
        Obligation { Obligation::Status::Blocked, Obligation::TriggerKind::Manual, 9, "host", "" });
    QCD_CHECK(!QivenContext::IsContinueable(&human, client.get(), device.get(), llm.get(),
                                            c2.get()));

    // K4 path: artifact restore quarantines — cognition without write authority
    const Bytes artifact = QivenContext::ReadFromCognition(c2.get(), Query {});
    CognitionSource artifactSource;
    artifactSource.kind               = CognitionSourceKind::HandoffArtifact;
    artifactSource.inlineBytes        = artifact;
    const auto restored               = QivenContext::CreateCognition(artifactSource);
    TransactionDelta quarantinedWrite = lesson();
    quarantinedWrite.base             = restored->contentId;
    QCD_CHECK(!QivenContext::WriteToCognition(restored.get(), quarantinedWrite,
                                              WorkMode::SupervisedForeground));
    QCD_CHECK(store->head() == c2->contentId); // the store was never touched

    // retire: the service drops ownership; pinned copies stay alive but fenced
    QCD_CHECK(QivenContext::RetireCognition(c1));
    QCD_CHECK(!QivenContext::RetireCognition(c1)); // already retired
    QCD_CHECK(c1->memory.size() == 1);             // pinned object alive (lifetime split)

    std::printf("[ OK ] persistence fencing gates\n");
}
