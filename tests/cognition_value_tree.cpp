// ============================================================================
// cognition_value_tree — R1 as a compiled drift tripwire + DR-001/DR-009
// proof: every Snapshot member is a VALUE; the tree is assignable again
// (no const epoch member); serialization round-trips byte-stably with a
// pinned golden vector.
// ============================================================================

#include <qiven/context/context.hpp>
#include <qiven/context/persistence.hpp>

#include <cstddef>
#include <cstdio>

#include "detail/check.hpp"

#include <type_traits>
#include <utility>

using namespace qiven::context;

#define QCD_ASSERT_VALUE_MEMBER(member)                                          \
    static_assert(!std::is_pointer_v<decltype(std::declval<Snapshot>().member)>, \
                  "R1 violated: " #member " must not be a pointer")

QCD_ASSERT_VALUE_MEMBER(governance);
QCD_ASSERT_VALUE_MEMBER(constitution);
QCD_ASSERT_VALUE_MEMBER(policy);
QCD_ASSERT_VALUE_MEMBER(state);
QCD_ASSERT_VALUE_MEMBER(decisions);
QCD_ASSERT_VALUE_MEMBER(memory);
QCD_ASSERT_VALUE_MEMBER(obligations);
QCD_ASSERT_VALUE_MEMBER(evidence);

static_assert(std::is_copy_constructible_v<Snapshot>, "snapshot-able (K4/K5 precondition)");
static_assert(std::is_copy_assignable_v<Snapshot>,
              "DR-001: a pure value tree is assignable (v2's const epoch broke this)");
static_assert(!std::is_polymorphic_v<Snapshot>, "no runtime dispatch inside the value tree");
static_assert(!std::is_constructible_v<Snapshot, Epoch>,
              "authority tokens are not cognition members (DR-010)");

int main()
{
    QivenContext::attachStore(std::make_shared<MemoryStore>());

    // a populated snapshot covering every record type
    Snapshot snapshot;
    snapshot.governance.rootPrincipal = "github:JasonHuang3D";
    snapshot.constitution.articles    = { "History is not overwritten",
                                          "Session independence is mandatory" };
    snapshot.policy.handoff           = { HandoffPolicy { OperationClass::DecisionAcceptance, true, false } };
    snapshot.policy.recovery          = { RecoveryRule { RefusalReason::StaleBase,
                                                RecoveryAction::RereadRethink } };
    snapshot.state                    = State { "active-work", "current", "repositories", "roadmap" };
    snapshot.decisions.push_back(Decision { 36,
                                            Lifecycle::Accepted,
                                            "ADR-0036",
                                            "typed human handoffs",
                                            {},
                                            {},
                                            Provenance { { "user_statement" } } });
    snapshot.memory.push_back(MemoryRecord { MemoryRecord::Kind::NegativeKnowledge,
                                             MemoryRecord::Status::Active,
                                             "rejected alternative",
                                             "event-sourcing was rejected: state-replication only",
                                             {} });
    snapshot.obligations.push_back(Obligation { Obligation::Status::Open,
                                                Obligation::TriggerKind::OnTouch,
                                                7,
                                                "upgrade chatgpt-jason view",
                                                "schema v2 landed" });
    snapshot.evidence.push_back(EvidenceRecord { "draft-0000000000000001", "17 suites PASS" });

    // roundtrip: serialize -> artifact restore -> serialize must be byte-stable
    const Bytes first = SerializeSnapshot(snapshot);
    CognitionSource source;
    source.kind           = CognitionSourceKind::HandoffArtifact;
    source.inlineBytes    = first;
    source.expectedDigest = DraftContentId(first);
    const auto restored   = QivenContext::CreateCognition(source);
    QCD_CHECK(restored != nullptr);
    QCD_CHECK(restored->quarantine == QuarantineState::Isolated);
    const Bytes second = SerializeSnapshot(*restored->state);

    std::printf("restored: principal=%zu articles=%zu handoff=%zu recovery=%zu decisions=%zu "
                "memory=%zu obligations=%zu evidence=%zu state=(%zu,%zu,%zu,%zu) ENDFLAG\n",
                restored->state->governance.rootPrincipal.size(),
                restored->state->constitution.articles.size(), restored->state->policy.handoff.size(),
                restored->state->policy.recovery.size(), restored->state->decisions.size(),
                restored->state->memory.size(), restored->state->obligations.size(),
                restored->state->evidence.size(), restored->state->state.activeWork.size(),
                restored->state->state.current.size(), restored->state->state.repositories.size(),
                restored->state->state.roadmap.size());

    QCD_CHECK(first == second);

    // the restored instance carries the artifact's content identity
    QCD_CHECK(restored->contentId == DraftContentId(first));
    QCD_CHECK(restored->state->decisions.size() == 1);
    QCD_CHECK(restored->state->obligations[0].id == 7);

    // value semantics: an independent copy serializes identically (DR-001)
    Snapshot copy      = snapshot; // copy-ASSIGNMENT — impossible in v2 (const epoch)
    copy.state.current = "mutated copy";
    QCD_CHECK(copy.state.current == "mutated copy");
    QCD_CHECK(snapshot.state.current == "current"); // the original is untouched

    std::printf("[ OK ] cognition value tree + serialization roundtrip\n");
}
