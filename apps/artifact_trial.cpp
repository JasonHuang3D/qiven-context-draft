// ============================================================================
// artifact_trial — the K4-shaped trial tooling for OBL-B4D6A8 Phase 4:
//
//   produce  : materialize a known cognition, export ONE self-describing
//              artifact (snapshot bytes + integrity digest + record manifest)
//   verify   : the CONSUMER side — read ONLY the artifact, verify integrity,
//              deserialize fail-closed, and print the structural facts a fresh
//              consumer can state without any repository access
//
// The cognitive trial (a fresh LLM answering the Project Continuity questions
// from the artifact alone) runs OUTSIDE this binary: acceptance topologies are
// real and cannot be simulated by the authoring session (pit P-13).
// ============================================================================

#include <qiven/context/context.hpp>

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace qiven::context;

namespace
{
constexpr char kTrialMarker[] = "trial: knowledge compaction is lossless only when the artifact is the cause";

ContextTransaction knownWrite(const RevisionId& base, std::int64_t decisionId)
{
    ContextTransaction transaction;
    transaction.base = base;
    transaction.operations.push_back(Operation { .kind     = Operation::Kind::AppendDecision,
                                                 .recordId = decisionId,
                                                 .title    = "ADR-trial",
                                                 .payload  = "the artifact-trial decision proves artifact-caused continuity" });
    transaction.handoffEvidenceRef = "trial-producer";
    transaction.operations.push_back(Operation { .kind          = Operation::Kind::AddMemory,
                                                 .aux           = 5, // NegativeKnowledge: a floor the consumer must be able to state
                                                 .title         = "rejected alternative",
                                                 .payload       = "event-sourcing was rejected: state-replication only",
                                                 .provenanceRef = "ADR-0033-alternatives" });
    transaction.h2 = H2Evidence { DigestOperations(transaction.operations),
                                  "github:JasonHuang3D", "jason-brother-glm5-3", "trial-producer" };
    return transaction;
}

bool writeFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        return false;
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(out);
}

std::string readFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf("usage: artifact_trial produce <artifact-file> | verify <artifact-file>\n");
        return 2;
    }
    const std::string mode = argv[1];
    const std::string path = argv[2];

    if (mode == "produce")
    {
        auto store = std::make_shared<MemoryStore>();
        QivenContext::attachStore(store);
        CognitionSource boot;
        boot.kind     = CognitionSourceKind::CanonicalRemote;
        const auto c1 = QivenContext::CreateCognition(boot);

        const auto actor = AuthenticatedActor { "github:JasonHuang3D", Role::Worker,
                                                "GLM-5.3-Flash", "GLM-5.3-Flash", "standard" };
        const auto grant = QivenContext::AcquireGrant(actor, WorkMode::SupervisedForeground);
        if (!grant.has_value())
        {
            std::printf("[FAIL] producer could not acquire the lease\n");
            return 1;
        }
        const auto verdict =
            QivenContext::WriteToCognition(c1, knownWrite(c1->revision, 1), *grant);
        QivenContext::ReleaseGrant(*grant);
        if (verdict.outcome != Verdict::Outcome::Applied)
        {
            std::printf("[FAIL] producer write refused (%d)\n", static_cast<int>(verdict.reason));
            return 1;
        }
        CognitionSource headSource;
        headSource.kind     = CognitionSourceKind::CanonicalRemote;
        headSource.revision = verdict.successor->revision;
        const auto head     = QivenContext::CreateCognition(headSource);
        if (!head)
        {
            const std::string eol(1, char(10));
            std::printf("[FAIL] producer could not re-materialize the head%s", eol.c_str());
            return 1;
        }

        // the ARTIFACT: self-describing, content-identified, no credentials
        const Bytes bytes         = SerializeSnapshot(*head->state);
        const ContentId contentId = DraftContentId(bytes); // restore identity
        std::ostringstream artifact;
        artifact << "qiven-artifact v1\n";
        artifact << "snapshot-digest: " << head->digest.value << "\n"; // integrity identity
        artifact << "content-id: " << contentId << "\n";               // restore identity (each in its role)
        artifact << "decisions: " << head->state->decisions.size() << "\n";
        artifact << "memory: " << head->state->memory.size() << "\n";
        artifact << "obligations: " << head->state->obligations.size() << "\n";
        artifact << "conflicts: " << head->state->conflicts.size() << "\n";
        artifact << "views: " << head->state->views.size() << "\n";
        artifact << "profiles: " << head->state->profiles.size() << "\n";
        artifact << "policy-handoff: " << head->state->policy.handoff.size() << "\n";
        artifact << "policy-recovery: " << head->state->policy.recovery.size() << "\n";
        artifact << "marker: " << kTrialMarker << "\n";
        artifact << "bytes-begin\n";
        for (const std::byte b : bytes)
        {
            artifact << static_cast<unsigned>(std::to_integer<unsigned char>(b)) << ' ';
        }
        artifact << "\nbytes-end\n";

        if (!writeFile(path, artifact.str()))
        {
            std::printf("[FAIL] could not write the artifact\n");
            return 1;
        }
        std::printf("[ OK ] artifact produced: %s (snapshot-digest %s)\n", path.c_str(),
                    head->digest.value.c_str());
        return 0;
    }

    if (mode == "verify")
    {
        const std::string artifact = readFile(path);
        if (artifact.empty())
        {
            std::printf("[FAIL] artifact unreadable or empty\n");
            return 1;
        }
        // the consumer parses the manifest and the byte section, then verifies
        // BOTH identities independently before trusting anything (review §3):
        // content-id drives the restore check; snapshot-digest drives integrity
        const auto digestPos = artifact.find("snapshot-digest: ");
        const auto idPos     = artifact.find("content-id: ");
        const auto dataPos   = artifact.find("bytes-begin\n");
        const auto endPos    = artifact.find("\nbytes-end\n");
        if (digestPos == std::string::npos || idPos == std::string::npos || dataPos == std::string::npos || endPos == std::string::npos)
        {
            std::printf("[FAIL] artifact manifest malformed\n");
            return 1;
        }
        const std::string declaredDigest =
            artifact.substr(digestPos + 17, artifact.find('\n', digestPos) - digestPos - 17);
        const std::string declaredContentId =
            artifact.substr(idPos + 12, artifact.find('\n', idPos) - idPos - 12);
        std::istringstream byteStream(artifact.substr(dataPos + 12, endPos - dataPos - 12));
        Bytes bytes;
        unsigned value = 0;
        while (byteStream >> value)
        {
            bytes.push_back(std::byte { static_cast<unsigned char>(value) });
        }
        if (DraftContentId(bytes) != declaredContentId)
        {
            std::printf("[FAIL] content-id mismatch: declared %s\n", declaredContentId.c_str());
            return 1;
        }
        const SnapshotDigest actual = DraftSnapshotDigest(bytes);
        if (actual.value != declaredDigest)
        {
            std::printf("[FAIL] digest mismatch: declared %s actual %s\n", declaredDigest.c_str(),
                        actual.value.c_str());
            return 1;
        }

        // fail-closed deserialization of the untrusted artifact bytes (DR-009)
        DeserializeError error;
        CognitionSource source;
        source.kind           = CognitionSourceKind::HandoffArtifact;
        source.inlineBytes    = bytes;
        source.expectedDigest = declaredContentId;
        const auto restored   = QivenContext::CreateCognition(source, &error);
        if (restored == nullptr)
        {
            std::printf("[FAIL] restore failed closed: kind=%d %s\n",
                        static_cast<int>(error.kind), error.detail.c_str());
            return 1;
        }

        // the structural facts a fresh consumer can state from the ARTIFACT ALONE
        std::printf("[ OK ] artifact verified: digest=%s\n", restored->digest.value.c_str());
        std::printf("[ OK ] governance: %s\n",
                    restored->state->governance.rootPrincipal.c_str());
        std::printf("[ OK ] constitution articles: %zu\n",
                    restored->state->constitution.articles.size());
        std::printf("[ OK ] decisions: %zu (first: %s)\n", restored->state->decisions.size(),
                    restored->state->decisions.empty() ? "<none>"
                                                       : restored->state->decisions[0].title.c_str());
        std::printf("[ OK ] memory: %zu\n", restored->state->memory.size());
        std::printf("[ OK ] policy rows: handoff=%zu recovery=%zu\n",
                    restored->state->policy.handoff.size(), restored->state->policy.recovery.size());
        bool foundTrialDecision     = false;
        bool foundNegativeKnowledge = false;
        for (const auto& decision : restored->state->decisions)
        {
            if (decision.title == "ADR-trial")
            {
                foundTrialDecision = true;
            }
        }
        for (const auto& record : restored->state->memory)
        {
            if (record.kind == MemoryRecord::Kind::NegativeKnowledge)
            {
                foundNegativeKnowledge = true;
            }
        }
        if (!foundTrialDecision || !foundNegativeKnowledge)
        {
            const std::string eol(1, char(10));
            std::printf("[FAIL] trial cognition missing from the restored artifact%s", eol.c_str());
            return 1;
        }
        std::printf("[ OK ] trial decision and negative knowledge recovered from the artifact\n");
        std::printf("[ OK ] %s\n", kTrialMarker);
        return 0;
    }

    std::printf("unknown mode: %s\n", mode.c_str());
    return 2;
}
