// ============================================================================
// cognition_value_tree — R1 as a compiled drift tripwire:
// every LLMCognition member must be a VALUE, never a pointer to runtime.
// ============================================================================

#include <qiven/context/cognition.hpp>
#include <qiven/context/persistence.hpp>

#include <cstddef>
#include <cstdio>

#include "detail/check.hpp"

#include <type_traits>
#include <utility>

using namespace qiven::context;

#define QCD_ASSERT_VALUE_MEMBER(member)                                              \
    static_assert(!std::is_pointer_v<decltype(std::declval<LLMCognition>().member)>, \
                  "R1 violated: " #member " must not be a pointer")

QCD_ASSERT_VALUE_MEMBER(epoch);
QCD_ASSERT_VALUE_MEMBER(contentId);
QCD_ASSERT_VALUE_MEMBER(governance);
QCD_ASSERT_VALUE_MEMBER(constitution);
QCD_ASSERT_VALUE_MEMBER(collaborations);
QCD_ASSERT_VALUE_MEMBER(state);
QCD_ASSERT_VALUE_MEMBER(decisions);
QCD_ASSERT_VALUE_MEMBER(memory);
QCD_ASSERT_VALUE_MEMBER(obligations);
QCD_ASSERT_VALUE_MEMBER(evidence);

static_assert(std::is_copy_constructible_v<LLMCognition>, "snapshot-able (K4/K5 precondition)");
static_assert(!std::is_polymorphic_v<LLMCognition>, "no runtime dispatch inside the value tree");

int main()
{
    QivenContext::attachStore(std::make_shared<MemoryStore>());

    // a populated cognition covering every record type
    LLMCognition cognition { 1,
                             ContentId {},
                             Governance { "github:JasonHuang3D" },
                             Constitution { { "History is not overwritten",
                                              "Session independence is mandatory" } },
                             { CollaborationRule { CollaborationRule::Domain::GitWorkflow,
                                                   "exact-head validation" } },
                             State { "active-work", "current", "repositories", "roadmap" },
                             { Decision { 36,
                                          Lifecycle::Accepted,
                                          "ADR-0036",
                                          "typed human handoffs",
                                          {},
                                          {},
                                          Provenance { { "user_statement" } } } },
                             { MemoryRecord { MemoryRecord::Kind::NegativeKnowledge,
                                              MemoryRecord::Status::Active,
                                              "rejected alternative",
                                              "event-sourcing was rejected: state-replication only",
                                              {} } },
                             { Obligation { Obligation::Status::Open,
                                            Obligation::TriggerKind::OnTouch,
                                            7,
                                            "upgrade chatgpt-jason view",
                                            "schema v2 landed" } },
                             { EvidenceRecord { "draft-0000000000000001", "17 suites PASS" } } };

    // roundtrip: serialize -> artifact restore -> serialize must be byte-stable
    const Bytes first = QivenContext::ReadFromCognition(&cognition, Query {});
    CognitionSource source;
    source.kind         = CognitionSourceKind::HandoffArtifact;
    source.inlineBytes  = first;
    const auto restored = QivenContext::CreateCognition(source);
    const Bytes second  = QivenContext::ReadFromCognition(restored.get(), Query {});

    std::printf("restored: principal=%zu articles=%zu collab=%zu decisions=%zu "
                "memory=%zu obligations=%zu evidence=%zu state=(%zu,%zu,%zu,%zu) ENDFLAG\n",
                restored->governance.rootPrincipal.size(),
                restored->constitution.articles.size(), restored->collaborations.size(),
                restored->decisions.size(), restored->memory.size(),
                restored->obligations.size(), restored->evidence.size(),
                restored->state.activeWork.size(), restored->state.current.size(),
                restored->state.repositories.size(), restored->state.roadmap.size());
    std::printf("sizes: first=%zu second=%zu\n", first.size(), second.size());

    if (first != second)
    {
        const std::size_t limit = first.size() < second.size() ? first.size() : second.size();
        for (std::size_t i = 0; i < limit; ++i)
        {
            if (first[i] != second[i])
            {
                std::printf("first diff at %zu: %d vs %d\n", i,
                            std::to_integer<int>(first[i]), std::to_integer<int>(second[i]));
                break;
            }
        }
    }
    QCD_CHECK(first == second);

    // the restored instance carries the artifact's content identity
    QCD_CHECK(restored->contentId == DraftContentId(first));
    QCD_CHECK(restored->decisions.size() == 1);
    QCD_CHECK(restored->obligations[0].id == 7);

    // value semantics: an independent copy serializes identically
    const auto copy = std::make_shared<LLMCognition>(cognition);
    QCD_CHECK(QivenContext::ReadFromCognition(copy.get(), Query {}) == first);

    std::printf("[ OK ] cognition value tree + serialization roundtrip\n");
}
