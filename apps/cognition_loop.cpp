// ============================================================================
// cognition_loop — runnable demo of the whole architecture:
// boot -> Work cycles through the gated write -> restore -> continue check
// ============================================================================

#include <qiven/context/context.hpp>

#include <cstdio>
#include <memory>
#include <string>

using namespace qiven::context;

namespace
{
TransactionDelta addLesson(const std::string& statement)
{
    TransactionDelta delta;
    delta.kind    = TransactionDelta::Kind::AddMemoryRecord;
    delta.title   = "draft-lesson";
    delta.payload = statement;
    return delta;
}

TransactionDelta acceptDecision(std::int64_t id, bool withHandoff)
{
    TransactionDelta delta;
    delta.kind     = TransactionDelta::Kind::AppendDecision;
    delta.recordId = id;
    delta.title    = "ADR-draft";
    delta.payload  = "accept the executable cognition specification";
    if (withHandoff)
    {
        delta.handoff            = Handoff::H2_Review; // delegated review evidence (ADR-0036)
        delta.handoffEvidenceRef = "PR-record review";
    }
    return delta;
}
} // namespace

int main()
{
    auto store = std::make_shared<MemoryStore>();
    QivenContext::attachStore(store);

    // cold boot: empty store -> genesis cognition (governance + constitution)
    CognitionSource boot;
    boot.kind      = CognitionSourceKind::CanonicalRemote;
    const auto cog = QivenContext::CreateCognition(boot);
    std::printf("[ OK ] boot        epoch=%llu head=%s\n",
                static_cast<unsigned long long>(cog->epoch), cog->contentId.c_str());

    // participants: a pure pointer graph — every change below is a rebind (R3)
    auto llm            = std::make_shared<LLM>();
    llm->name           = "GLM-5.3-Flash";
    llm->pCognition     = cog;
    llm->deltaGenerator = [](const std::string&, const LLMCognition&) {
        return addLesson("qiven-context-draft v2 runs the full cognition loop");
    };

    auto device  = std::make_shared<Device>();
    device->name = "JasonPC";
    device->os   = "Windows11";
    device->env  = "git,gh,MSVC,etc";

    auto client          = std::make_shared<LLMClientTool>();
    client->name         = "zcode-desktop";
    client->pCurrentLLM  = llm;
    client->pTargeDevice = device;
    client->binding      = ParticipantBinding { Role::Worker, "glm-5.3-flash" };

    auto human               = std::make_shared<Human>();
    human->name              = "Jason";
    human->verifiedPrincipal = "github:JasonHuang3D"; // session-injected, verified (R4)

    std::string result;
    // cycle 1: supervised mutating write — passes every gate
    llm->Work("run the draft loop", result);
    std::printf("[ %s ] cycle1      %s\n", llm->parkedDeltas.empty() ? "OK" : "FAIL", result.c_str());

    // cycle 2: unattended mutating write — refused (ADR-0036: read-only default)
    llm->Work("background maintenance", result, WorkMode::Unattended);
    std::printf("[ OK ] cycle2      %s (unattended write refused)\n", result.c_str());

    // cycle 3: decision acceptance without the typed H2 evidence — refused
    llm->deltaGenerator = [](const std::string&, const LLMCognition&) {
        return acceptDecision(37, false);
    };
    llm->Work("accept without review", result);
    std::printf("[ OK ] cycle3      %s (H2 missing -> refused)\n", result.c_str());

    // cycle 4: with the H2 evidence — accepted; the durable token advances
    llm->deltaGenerator = [](const std::string&, const LLMCognition&) {
        return acceptDecision(37, true);
    };
    llm->Work("accept with review", result);
    std::printf("[ %s ] cycle4      %s\n", llm->parkedDeltas.size() == 2 ? "OK" : "FAIL",
                result.c_str());

    // restore: materialize NEW, rebind, THEN retire (R2 ordering — no dangling window)
    CognitionSource freshSource;
    freshSource.kind = CognitionSourceKind::CanonicalRemote; // empty id = store head
    const auto fresh = QivenContext::CreateCognition(freshSource);
    llm->pCognition  = fresh;
    QivenContext::RetireCognition(cog);
    std::printf("[ OK ] restore     epoch=%llu decisions=%zu memory=%zu\n",
                static_cast<unsigned long long>(fresh->epoch), fresh->decisions.size(),
                fresh->memory.size());

    // continuation check — the only whole-graph predicate
    if (!QivenContext::IsContinueable(human.get(), client.get(), device.get(), llm.get(),
                                      fresh.get()))
    {
        std::printf("[FAIL] continueable\n");
        return 1;
    }
    std::printf("[ OK ] continueable\n");

    // K4 path: artifact materialization is quarantined — restores cognition but
    // cannot write until a governed authority cutover
    const Bytes artifact = QivenContext::ReadFromCognition(fresh.get(), Query {});
    CognitionSource artifactSource;
    artifactSource.kind        = CognitionSourceKind::HandoffArtifact;
    artifactSource.inlineBytes = artifact;
    const auto restored        = QivenContext::CreateCognition(artifactSource);
    TransactionDelta mutation  = addLesson("mutate a quarantined restore");
    mutation.base              = restored->contentId;
    const bool allowed         = QivenContext::WriteToCognition(restored.get(), mutation,
                                                                WorkMode::SupervisedForeground);
    std::printf("[ %s ] k4-quarantine write %s\n", allowed ? "FAIL" : "OK",
                allowed ? "allowed" : "refused");

    std::printf("[ OK ] qiven-context-draft loop complete\n");
    return allowed ? 1 : 0;
}
