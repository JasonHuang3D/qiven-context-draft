// ============================================================================
// participant_model — DR-017/018/019 as compiled proof:
//   I-PM3  a session designation never changes effective authorities
//          (authority-delta-∅, enforced structurally: the registry lookup
//          never reads the designation)
//   I-PM6  runtime participant instances (device / remote / human / client
//          tool) never reach the read plane — R5 blindness, behaviorally
//   fail-closed: an unregistered or unprovenanced role has NO authorities
//   DR-018: workflow layer metadata survives the serialize -> restore roundtrip
//   DR-017: the role registry itself round-trips (exports carry it)
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
    snapshot.constitution.articles    = { "History is not overwritten" };
    snapshot.state.objective          = "participant hardening";
    snapshot.state.candidateRef       = "not-created";
    snapshot.roles                    = default_role_specs();
    snapshot.profiles.push_back(ProfileRecord { "views/workflows/supervised-agent",
                                                "workflow",
                                                "capability-class procedure",
                                                WorkflowLayer::CapabilityProcedure });
    snapshot.views.push_back(ViewSpec { "zcode-jason",
                                        "ZCode",
                                        "Jason",
                                        "supervised local execution",
                                        { "views/workflows/supervised-agent" } });
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

} // namespace

int main()
{
    QivenContext::attachStore(std::make_shared<MemoryStore>());

    // --- I-PM3: authority delta of a designation is empty, ALWAYS ----------
    {
        Snapshot snapshot = sample_snapshot();
        AuthenticatedActor plain { "github:JasonHuang3D", Role::Brother,
                                   "jason-brother-glm5-3", "GLM-5.3", "high", "" };
        AuthenticatedActor designated = plain;
        designated.designation        = "jason-extended-cognition";

        SessionDesignation designation;
        designation.id              = "jason-extended-cognition";
        designation.base            = Role::Brother;
        designation.grantRef        = "2026-09-20 owner conversation grant";
        designation.processBypasses = { "brother/worker partition (draft repo only)" };
        QCD_CHECK(designation_valid(designation));

        const auto plainAuths = effective_authorities(snapshot, plain);
        const auto desigAuths = effective_authorities(snapshot, designated);
        QCD_CHECK(plainAuths == desigAuths); // the delta is EMPTY
        QCD_CHECK(!plainAuths.empty());
    }

    // --- a designation without an owner grant is NEVER valid ---------------
    {
        SessionDesignation selfAssigned;
        selfAssigned.id   = "invented-designation";
        selfAssigned.base = Role::Owner;
        QCD_CHECK(!designation_valid(selfAssigned)); // no grantRef => invalid
    }

    // --- fail-closed: unregistered role has no authorities -----------------
    {
        Snapshot snapshot; // empty registry
        AuthenticatedActor actor { "github:JasonHuang3D", Role::Brother,
                                   "b", "m", "high", "" };
        QCD_CHECK(effective_authorities(snapshot, actor).empty());
    }

    // --- fail-closed: unprovenanced spec is a proposal, not a registry -----
    {
        Snapshot snapshot = sample_snapshot();
        RoleSpec proposal;
        proposal.id          = Role::Worker;
        proposal.authorities = { OperationClass::DecisionAcceptance }; // attempted widening
        proposal.duties      = { "no provenance" };
        // provenance intentionally empty
        snapshot.roles.push_back(proposal);
        AuthenticatedActor actor { "github:JasonHuang3D", Role::Worker,
                                   "w", "m", "standard", "" };
        // the FIRST registered spec wins and stays valid; the unprovenanced
        // one must not have been trusted as an entry even before that —
        // role_spec_valid is the admission predicate:
        QCD_CHECK(!role_spec_valid(proposal));
    }

    // --- canonical floors (DR-017): classes, not instances ------------------
    {
        Snapshot registryOnly;                     // named object: find_role_spec returns a pointer
        registryOnly.roles = default_role_specs(); // INTO it — no temporaries
        QCD_CHECK(registryOnly.roles.size() == 3);
        const RoleSpec* owner = find_role_spec(registryOnly, Role::Owner);
        QCD_CHECK(owner != nullptr);
        const RoleSpec* brother = find_role_spec(registryOnly, Role::Brother);
        QCD_CHECK(brother != nullptr);
        const RoleSpec* worker = find_role_spec(registryOnly, Role::Worker);
        QCD_CHECK(worker != nullptr);
        QCD_CHECK(owner->floor.identityPortVerified);
        QCD_CHECK(brother->floor.reviewGradeReasoning);
        QCD_CHECK(worker->floor.execution == CapabilityClass::LocalSupervised);
    }

    // --- DR-017/018: registry + layer metadata round-trip -------------------
    {
        const Snapshot snapshot        = sample_snapshot();
        const Bytes first              = serialize_snapshot(snapshot);
        const CognitionHandle restored = materialize(snapshot);
        const Bytes second             = serialize_snapshot(*restored->state);
        QCD_CHECK(first == second); // byte-stable incl. the new members

        const RoleSpec* worker = find_role_spec(*restored->state, Role::Worker);
        QCD_CHECK(worker != nullptr);
        QCD_CHECK(worker->floor.execution == CapabilityClass::LocalSupervised);
        QCD_CHECK(worker->duties.size() == 1);
        QCD_CHECK(!worker->provenance.empty());

        QCD_CHECK(restored->state->profiles.size() == 1);
        QCD_CHECK(restored->state->profiles.front().layer == WorkflowLayer::CapabilityProcedure);
    }

    // --- I-PM6: runtime participant instances never reach the read plane ----
    {
        Device device;
        device.name = "MARKER-DEVICE";
        RemoteDevice remote;
        remote.name = "MARKER-REMOTE-DEVICE";
        Human human;
        human.name = "MARKER-HUMAN";
        LLMClientTool tool;
        tool.name = "MARKER-CLIENT-TOOL";

        // the bundle is built from cognition ONLY; the runtime instances above
        // exist in the same scope and must not leak into any bundle field
        const CognitionHandle handle = materialize(sample_snapshot());
        Query query;
        query.taskScope            = "participant audit";
        const ContextBundle bundle = QivenContext::BuildBundle(handle, query);

        std::string bundleText;
        for (const auto& input : bundle.mandatoryInputs)
        {
            bundleText += input;
        }
        for (const auto& constraint : bundle.protectedConstraints)
        {
            bundleText += constraint;
        }
        for (const auto& candidate : bundle.candidates)
        {
            bundleText += candidate.summary;
        }
        QCD_CHECK(bundleText.find("MARKER-DEVICE") == std::string::npos);
        QCD_CHECK(bundleText.find("MARKER-REMOTE-DEVICE") == std::string::npos);
        QCD_CHECK(bundleText.find("MARKER-HUMAN") == std::string::npos);
        QCD_CHECK(bundleText.find("MARKER-CLIENT-TOOL") == std::string::npos);
    }

    std::printf("[ OK ] participant model: DR-017/018/019, I-PM3, I-PM6, fail-closed registry\n");
    return 0;
}
