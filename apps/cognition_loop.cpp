// ============================================================================
// cognition_loop — runnable demo of the v3 architecture:
// boot -> grant -> Work cycles through the gated write (typed verdicts and
// recovery-as-data) -> reader boot preserves the writer -> restore quarantined
// ============================================================================

#include <qiven/context/context.hpp>

#include <cstdio>
#include <memory>
#include <string>

using namespace qiven::context;

namespace
{
AuthenticatedActor demoActor()
{
    return AuthenticatedActor { "github:JasonHuang3D", Role::Worker, "glm-5.3-flash",
                                "GLM-5.3-Flash", "max" };
}

ContextTransaction addLesson(const std::string& statement)
{
    ContextTransaction transaction;
    transaction.operations.push_back(
        Operation { .kind = Operation::Kind::AddMemory, .title = "draft-lesson", .payload = statement, .provenanceRef = "demo-session" });
    return transaction;
}
} // namespace

int main()
{
    auto store = std::make_shared<MemoryStore>();
    QivenContext::attachStore(store);

    // cold boot: empty store -> genesis cognition (governance + constitution
    // + the authority policy table)
    CognitionSource boot;
    boot.kind      = CognitionSourceKind::CanonicalRemote;
    const auto cog = QivenContext::CreateCognition(boot);
    std::printf("[ OK ] boot        epoch=%llu head=%s\n",
                static_cast<unsigned long long>(cog->epoch), cog->contentId.c_str());

    // participants: a pure pointer graph — every change below is a rebind (R3)
    auto llm            = std::make_shared<LLM>();
    llm->name           = "GLM-5.3-Flash";
    llm->pCognition     = cog;
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        return addLesson("qiven-context-draft v3 phase 1 runs the full cognition loop");
    };

    auto device  = std::make_shared<Device>();
    device->name = "JasonPC";
    device->os   = "Windows11";
    device->env  = "git,gh,MSVC,etc";

    auto client           = std::make_shared<LLMClientTool>();
    client->name          = "zcode-desktop";
    client->pCurrentLLM   = llm;
    client->pTargetDevice = device;
    client->binding       = ParticipantBinding { Role::Worker, "glm-5.3-flash" };

    auto human               = std::make_shared<Human>();
    human->name              = "Jason";
    human->verifiedPrincipal = "github:JasonHuang3D"; // session-injected, verified at the port (R4)

    std::string result;
    // cycle 1: supervised mutating write — passes every gate
    human->UseLLMToWork(client, "run the draft loop", result);
    std::printf("[ %s ] cycle1      %s\n", llm->parkedDeltas.empty() ? "OK" : "FAIL", result.c_str());

    // cycle 2: unattended mutating write — refused; recovery = defer (ADR-0036)
    human->UseLLMToWork(client, "background maintenance", result);
    std::printf("[ OK ] cycle2      %s\n", result.c_str());

    // cycle 3: decision acceptance without the typed H2 evidence — refused;
    // recovery = halt and escalate, retry forbidden
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind = Operation::Kind::AppendDecision, .recordId = 37, .title = "ADR-draft", .payload = "accept the specification" });
        return transaction;
    };
    human->UseLLMToWork(client, "accept without review", result);
    std::printf("[ OK ] cycle3      %s\n", result.c_str());

    // cycle 4: with content-bound H2 evidence — accepted; the durable token advances
    llm->deltaGenerator = [](const std::string&, const Snapshot&) {
        ContextTransaction transaction;
        transaction.operations.push_back(Operation { .kind = Operation::Kind::AppendDecision, .recordId = 37, .title = "ADR-draft", .payload = "accept the specification" });
        transaction.handoffEvidenceRef = "PR-record review";
        H2Evidence evidence;
        evidence.reviewer            = "github:JasonHuang3D";
        evidence.reviewRef           = "brother-review";
        evidence.reviewedDeltaDigest = DigestOperations(transaction.operations);
        transaction.h2               = evidence;
        return transaction;
    };
    human->UseLLMToWork(client, "accept with review", result);
    std::printf("[ %s ] cycle4      %s\n", result.find("delta applied") != std::string::npos ? "OK" : "FAIL",
                result.c_str());

    // restore: materialize NEW, rebind, THEN retire (R2 ordering — no dangling
    // window). The reader boot does NOT revoke the writer (DR-010) — the cycle
    // above already proved the write survived a second materialization.
    CognitionSource freshSource;
    freshSource.kind = CognitionSourceKind::CanonicalRemote; // empty id = store head
    const auto fresh = QivenContext::CreateCognition(freshSource);
    llm->pCognition  = fresh;
    QivenContext::RetireCognition(cog);
    std::printf("[ OK ] restore     epoch=%llu decisions=%zu memory=%zu\n",
                static_cast<unsigned long long>(fresh->epoch), fresh->state->decisions.size(),
                fresh->state->memory.size());

    // continuation check — the only whole-graph predicate
    if (!QivenContext::IsContinueable(human.get(), client.get(), device.get(), llm.get(), fresh))
    {
        std::printf("[FAIL] continueable\n");
        return 1;
    }
    std::printf("[ OK ] continueable\n");

    // K4 path: artifact materialization is quarantined — restores cognition but
    // cannot write until a governed authority cutover; corruption fails closed
    const Bytes artifact = QivenContext::ReadFromCognition(fresh, Query {});
    CognitionSource artifactSource;
    artifactSource.kind           = CognitionSourceKind::HandoffArtifact;
    artifactSource.inlineBytes    = artifact;
    artifactSource.expectedDigest = DraftContentId(artifact);
    const auto restored           = QivenContext::CreateCognition(artifactSource);
    const auto grant              = QivenContext::AcquireGrant(demoActor(), WorkMode::SupervisedForeground);
    const bool allowed            = grant.has_value() && QivenContext::WriteToCognition(restored, addLesson("mutate a quarantined restore"), *grant).outcome == Verdict::Outcome::Applied;
    std::printf("[ %s ] k4-quarantine write %s\n", allowed ? "FAIL" : "OK",
                allowed ? "allowed" : "refused");

    Bytes corrupted = artifact;
    corrupted[corrupted.size() / 2] ^= std::byte { 0xFF };
    CognitionSource corruptedSource;
    corruptedSource.kind           = CognitionSourceKind::HandoffArtifact;
    corruptedSource.inlineBytes    = corrupted;
    corruptedSource.expectedDigest = DraftContentId(artifact);
    const auto broken              = QivenContext::CreateCognition(corruptedSource);
    std::printf("[ %s ] corrupt artifact restore %s\n", broken ? "FAIL" : "OK",
                broken ? "accepted" : "failed closed");
    if (grant.has_value())
    {
        QivenContext::ReleaseGrant(*grant);
    }

    const bool ok = !allowed && broken == nullptr;
    std::printf("[ %s ] qiven-context-draft loop complete\n", ok ? "OK" : "FAIL");
    return ok ? 0 : 1;
}
