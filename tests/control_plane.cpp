// ============================================================================
// control_plane — v4.3 compiled proof (roadmap v4.3; seed §28/§29/§41):
//   - the naming scenario (§29): a CreateCppSymbol intent derives a packet
//     whose mandatory context carries the naming policy - from the SNAPSHOT
//     only, no participant memory
//   - the §41 miniature: a serialized->restored generation derives the
//     IDENTICAL preparation (activation survives transport)
//   - fail-closed: a blocking recall that finds nothing lands in unresolved
//     and the packet is not ready (the action may not proceed)
//   - action-class demands (verify-live / run-check / review) are listed,
//     not auto-satisfied — satisfaction is execution-time, and the packet
//     grants no authority (§40)
// ============================================================================

#include <qiven/context/context.hpp>
#include <qiven/context/persistence.hpp>
#include <qiven/context/runtime.hpp>

#include <cstdio>
#include <string>
#include <vector>

#include "detail/check.hpp"

using namespace qiven::context;

namespace
{

Snapshot sample_snapshot()
{
    Snapshot snapshot;
    snapshot.governance.rootPrincipal = "github:JasonHuang3D";
    snapshot.state.objective          = "v4.3 control plane";
    snapshot.state.candidateRef       = "not-created";
    snapshot.invocation               = default_invocation_policy();
    MemoryRecord naming; // the §29 naming policy, as canonical cognition
    naming.kind      = MemoryRecord::Kind::Protocol;
    naming.title     = "naming policy";
    naming.statement = "snake_case identifiers per qiven-devkit conventions";
    snapshot.memory.push_back(naming);
    return snapshot;
}

CognitionHandle materialize(const Snapshot& snapshot)
{
    const Bytes bytes = serialize_snapshot(snapshot);
    CognitionSource source;
    source.kind            = CognitionSourceKind::HandoffArtifact;
    source.inlineBytes     = bytes;
    source.expectedDigest  = draft_content_id(bytes);
    CognitionHandle handle = QivenContext::create_cognition(source);
    QCD_CHECK(handle != nullptr);
    return handle;
}

bool hasRequirement(const std::vector<CognitiveRequirement>& requirements,
                    RequirementKind kind)
{
    for (const auto& requirement : requirements)
    {
        if (requirement.kind == kind)
        {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    QivenContext::attachStore(std::make_shared<MemoryStore>());

    // --- the compiled scar backlog is policy data (P-51/52/53 as rules) ----
    {
        const InvocationPolicy policy = default_invocation_policy();
        QCD_CHECK(policy.rules.size() == 11);
        Snapshot snapshot; // default-constructed policy is EMPTY: no rules,
        ActionIntent any;  // no activation claims - fail-closed by absence
        QCD_CHECK(derive_requirements(snapshot, any).empty());
    }

    // --- §29: the naming scenario ------------------------------------------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent; // a fresh generation creating an identifier
        intent.kind                    = ActionKind::CreateCppSymbol;
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::MandatoryRecall));
        QCD_CHECK(!packet.mandatoryContext.empty());
        bool carriesPolicy = false;
        for (const auto& entry : packet.mandatoryContext)
        {
            if (entry.find("naming policy") != std::string::npos)
            {
                carriesPolicy = true;
            }
        }
        QCD_CHECK(carriesPolicy); // the convention IS in the preparation
        QCD_CHECK(packet.ready());
    }

    // --- §41 miniature: activation survives transport -----------------------
    {
        const Snapshot snapshot          = sample_snapshot();
        const CognitionHandle restored   = materialize(snapshot);
        const Snapshot& restoredSnapshot = *restored->state;
        QCD_CHECK(restoredSnapshot.invocation.rules.size() ==
                  snapshot.invocation.rules.size());
        ActionIntent intent;
        intent.kind                    = ActionKind::CreateCppSymbol;
        const PreparationPacket before = build_preparation_packet(snapshot, intent);
        const PreparationPacket after =
            build_preparation_packet(restoredSnapshot, intent);
        QCD_CHECK(before.requirements.size() == after.requirements.size());
        QCD_CHECK(before.mandatoryContext == after.mandatoryContext); // identical
        QCD_CHECK(after.ready());
    }

    // --- fail-closed: a blocking recall that finds nothing ------------------
    {
        Snapshot snapshot = sample_snapshot();
        snapshot.memory.clear(); // the canonical record does not exist here
        snapshot.profiles.clear();
        ActionIntent intent;
        intent.kind                    = ActionKind::MakeCanonicalClaim;
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::VerifyCanonical));
        QCD_CHECK(!packet.unresolved.empty()); // blocking recall unresolved
        QCD_CHECK(!packet.ready());            // the action may not proceed
    }

    // --- action-class demands are listed, never auto-satisfied --------------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent; // merge-class publication
        intent.kind                    = ActionKind::Publish;
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::RunMechanicalCheck));
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::RequestReview));
        QCD_CHECK(packet.unresolved.empty()); // demands are execution-time;
        QCD_CHECK(packet.ready());            // the packet itself grants nothing
    }

    // --- purity: same snapshot + intent => identical packet ------------------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent;
        intent.kind = ActionKind::RetryFailure;
        FailureFingerprint fingerprint;
        fingerprint.operation          = "qiven gate";
        fingerprint.stableMessage      = "exit 1";
        intent.priorFailure            = fingerprint;
        const PreparationPacket first  = build_preparation_packet(snapshot, intent);
        const PreparationPacket second = build_preparation_packet(snapshot, intent);
        QCD_CHECK(first.requirements.size() == second.requirements.size());
        QCD_CHECK(first.requirements.front().kind == RequirementKind::InspectKnownPit);
        QCD_CHECK(first.intent.priorFailure.has_value() &&
                  first.intent.priorFailure->operation == "qiven gate");
    }

    // --- v4.5 / A8 scenario (seed 41-2 in miniature): a Foundation duplicate
    // is intercepted - the search is mandatory and the existing
    // implementation is surfaced for judgment, never auto-decided ------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent;
        intent.kind                    = ActionKind::IntroducePrimitive;
        intent.concepts                = { "hash" };
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::SearchLowerLayer));
        QCD_CHECK(packet.unresolved.empty()); // the demand is execution-time

        ExistingImplementationReport unsearched; // the mechanism never ran
        QCD_CHECK(!primitive_judgment_authorized(intent, unsearched));

        ExistingImplementationReport searched; // it ran and FOUND the duplicate
        LowerLayerHit hit;
        hit.repo          = "qiven-foundation";
        hit.symbol        = "qiven::fnv1a64";
        hit.summary       = "canonical hashing";
        searched.searched = true;
        searched.hits.push_back(hit);
        QCD_CHECK(primitive_judgment_authorized(intent, searched));
        QCD_CHECK(searched.hits.front().symbol == "qiven::fnv1a64"); // surfaced
    }

    std::printf("[ OK ] control plane: naming scenario, transport-stable activation, "
                "fail-closed recall, listed action-class demands, lower-layer interception\n");
    return 0;
}
