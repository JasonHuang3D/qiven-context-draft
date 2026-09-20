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

bool hasRequirement(const std::vector<PreparedRequirement>& requirements,
                    RequirementKind kind)
{
    for (const auto& prepared : requirements)
    {
        if (prepared.requirement.kind == kind)
        {
            return true;
        }
    }
    return false;
}

PreparedRequirement& findPrepared(PreparationPacket& packet, RequirementKind kind)
{
    for (auto& prepared : packet.requirements)
    {
        if (prepared.requirement.kind == kind)
        {
            return prepared;
        }
    }
    QCD_CHECK(false); // test bug: kind not derived
    return packet.requirements.front();
}

} // namespace

int main()
{
    QivenContext::attachStore(std::make_shared<MemoryStore>());

    // --- the compiled scar backlog is policy data (P-51/52/53 as rules) ----
    {
        const InvocationPolicy policy = default_invocation_policy();
        QCD_CHECK(policy.rules.size() == 11);
        Snapshot snapshot; // V4S World A: policy ABSENT is a typed failure at
        ActionIntent any;  // preparation time, never an empty derivation claim
        QCD_CHECK(build_preparation_packet(snapshot, any).failure ==
                  PreparationFailure::InvocationPolicyMissing);
    }

    // --- §29: the naming scenario ------------------------------------------
    {
        Snapshot snapshot = sample_snapshot();
        MemoryRecord decoy; // pit.similar_prose_does_not_satisfy_naming_policy
        decoy.kind      = MemoryRecord::Kind::Fact;
        decoy.title     = "conventions chat"; // NOT the exact key
        decoy.statement = "someone mentioned the naming policy in passing";
        snapshot.memory.push_back(decoy);
        ActionIntent intent; // a fresh generation creating an identifier
        intent.kind                    = ActionKind::CreateCppSymbol;
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        for (const auto& entry : packet.mandatoryContext)
        {
            QCD_CHECK(entry.find("conventions chat") == std::string::npos);
        }
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
        QCD_CHECK(carriesPolicy);               // the convention IS in the preparation
        QCD_CHECK(packet.ready_for_judgment()); // naming recall SATISFIED
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
        QCD_CHECK(after.ready_for_judgment());
    }

    // --- V4S-03 / 27.7: a random record containing the phrase "canonical
    // record" must NOT satisfy VerifyCanonical - canonical verification is
    // not generic memory lookup; it stays Pending for an explicit resolver
    {
        Snapshot snapshot = sample_snapshot();
        MemoryRecord phraseDecoy;
        phraseDecoy.kind      = MemoryRecord::Kind::Fact;
        phraseDecoy.title     = "hallway note";
        phraseDecoy.statement = "the canonical record might be somewhere";
        snapshot.memory.push_back(phraseDecoy);
        ActionIntent intent;
        intent.kind                    = ActionKind::MakeCanonicalClaim;
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::VerifyCanonical));
        QCD_CHECK(packet.failure == PreparationFailure::None); // not a recall failure
        bool verifyPending = false;
        for (const auto& prepared : packet.requirements)
        {
            if (prepared.requirement.kind == RequirementKind::VerifyCanonical)
            {
                verifyPending = prepared.status == RequirementStatus::Pending;
            }
        }
        QCD_CHECK(verifyPending);                // PENDING, never accidentally Satisfied
        QCD_CHECK(!packet.ready_for_judgment()); // the action may not proceed
    }

    // --- V4S-02 / pit.publish_not_execution_ready_when_requirements_only_listed
    // and publish_execution_ready_only_after_all_blocking_requirements_satisfied
    // (plan sec 27.5): LISTED is never SATISFIED.
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent; // merge-class publication
        intent.kind              = ActionKind::Publish;
        PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::RunMechanicalCheck));
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::RequestReview));
        QCD_CHECK(packet.ready_for_judgment());   // no BeforeJudgment blocking rows
        QCD_CHECK(!packet.ready_for_execution()); // demands merely LISTED

        mark_satisfied(findPrepared(packet, RequirementKind::RunMechanicalCheck),
                       "gate receipt (reference state)");
        QCD_CHECK(!packet.ready_for_execution()); // review still unsatisfied

        PreparationPacket second = build_preparation_packet(snapshot, intent);
        mark_satisfied(findPrepared(second, RequirementKind::RequestReview),
                       "H2 record (reference state)");
        QCD_CHECK(!second.ready_for_execution()); // check still unsatisfied

        mark_satisfied(findPrepared(packet, RequirementKind::RequestReview),
                       "H2 record (reference state)");
        QCD_CHECK(packet.ready_for_execution()); // BOTH satisfied
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
        QCD_CHECK(first.requirements.front().requirement.kind ==
                  RequirementKind::InspectKnownPit);
        QCD_CHECK(first.intent.priorFailure.has_value() &&
                  first.intent.priorFailure->operation == "qiven gate");
        QCD_CHECK(!first.ready_for_judgment()); // no pit recalled yet: Failed

        // V4S-03 / 27.8: the fingerprint path CONNECTS to the packet
        Snapshot scarred = sample_snapshot();
        MemoryRecord scar;
        scar.kind      = MemoryRecord::Kind::Lesson;
        scar.title     = "qiven gate odyssey";
        scar.statement = "one-assert-per-cycle cost a full gate each round";
        scarred.memory.push_back(scar);
        const PreparationPacket recalled = build_preparation_packet(scarred, intent);
        QCD_CHECK(!recalled.knownPits.empty());   // the pit IS recalled
        QCD_CHECK(recalled.ready_for_judgment()); // InspectKnownPit Satisfied
    }

    // --- V4.5 / A8 scenario (seed 41-2 in miniature): a Foundation duplicate
    // is intercepted - the search is mandatory and the existing
    // implementation is surfaced for judgment, never auto-decided ------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent;
        intent.kind                    = ActionKind::IntroducePrimitive;
        intent.concepts                = { "hash" };
        const PreparationPacket packet = build_preparation_packet(snapshot, intent);
        QCD_CHECK(hasRequirement(packet.requirements, RequirementKind::SearchLowerLayer));
        QCD_CHECK(packet.requirements.front().status == RequirementStatus::Pending);
        QCD_CHECK(!packet.ready_for_judgment()); // search is mandatory pre-judgment

        ExistingImplementationReport unsearched; // the mechanism never ran
        QCD_CHECK(!primitive_judgment_precondition_met(intent, unsearched));

        ExistingImplementationReport searched; // it ran and FOUND the duplicate
        LowerLayerHit hit;
        hit.repo          = "qiven-foundation";
        hit.symbol        = "qiven::fnv1a64";
        hit.summary       = "canonical hashing";
        searched.searched = true;
        searched.hits.push_back(hit);
        QCD_CHECK(primitive_judgment_precondition_met(intent, searched));
        QCD_CHECK(searched.hits.front().symbol == "qiven::fnv1a64"); // surfaced
    }

    // --- V4S-04 / 27.9: the claim axis is not decorative --------------------
    {
        Snapshot snapshot = sample_snapshot();
        // isolate the scoped rule: replace the default policy so ONLY the
        // claim-scoped row exists for this action (the default also carries
        // an unscoped MakeCanonicalClaim row that would match any class)
        snapshot.invocation         = InvocationPolicy {};
        snapshot.invocation.present = true;
        InvocationRule claimScoped;
        claimScoped.action      = ActionKind::MakeCanonicalClaim;
        claimScoped.claimClass  = ClaimClass::CanonicalFact;
        claimScoped.requirement = RequirementKind::VerifyCanonical;
        claimScoped.subject     = "canonical record";
        snapshot.invocation.rules.push_back(claimScoped);

        ActionIntent asCanonical = []() {
            ActionIntent intent;
            intent.kind       = ActionKind::MakeCanonicalClaim;
            intent.claimClass = ClaimClass::CanonicalFact;
            return intent;
        }();
        ActionIntent asLocalRecall = asCanonical;
        asLocalRecall.claimClass   = ClaimClass::LocalRecall; // ONLY the claim differs

        const auto canonicalRows = derive_requirements(snapshot, asCanonical);
        const auto localRows     = derive_requirements(snapshot, asLocalRecall);
        QCD_CHECK(!canonicalRows.empty()); // scoped rule applied
        QCD_CHECK(localRows.empty());      // ...and not to the other class
    }

    // --- V4S-04 / 27.10: discretionary needs ride along, never waive --------
    {
        const Snapshot snapshot = sample_snapshot();
        ActionIntent intent;
        intent.kind = ActionKind::CreateCppSymbol;
        CognitiveNeed wantMore;
        wantMore.kind    = CognitiveNeedKind::SearchEvidence;
        wantMore.subject = "naming examples";
        const PreparationPacket withNeed =
            build_preparation_packet(snapshot, intent, { wantMore });
        const PreparationPacket withoutNeed = build_preparation_packet(snapshot, intent);
        QCD_CHECK(withNeed.discretionaryNeeds.size() == 1); // preserved
        QCD_CHECK(withoutNeed.discretionaryNeeds.empty());
        QCD_CHECK(withNeed.requirements.size() == withoutNeed.requirements.size());
        // a need cannot satisfy or remove the mandatory naming recall:
        QCD_CHECK(!withNeed.ready_for_judgment() ||
                  withNeed.requirements.front().status == RequirementStatus::Satisfied);
        bool namingStillMandatory = false;
        for (const auto& prepared : withNeed.requirements)
        {
            if (prepared.requirement.kind == RequirementKind::MandatoryRecall)
            {
                namingStillMandatory = true;
            }
        }
        QCD_CHECK(namingStillMandatory);
    }

    std::printf("%s", "[ OK ] control plane V4S-02: naming, transport, fail-closed, "
                      "readiness split, lower-layer interception\n");
    return 0;
}
