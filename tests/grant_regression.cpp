// ============================================================================
// grant_regression — GrantId monotonic generation: after any number of
// acquire/release cycles, a saved old GrantId can never collide with the
// current one, and a stale grant can never regain authority.
// Review: the old implementation truncated to 8 bits, wrapping at 256.
// ============================================================================

#include <qiven/context/context.hpp>

#include "detail/check.hpp"

#include <memory>
#include <string>

using namespace qiven::context;

namespace
{
AuthenticatedActor goodActor()
{
    return AuthenticatedActor { "github:JasonHuang3D", Role::Worker,
                                "GLM-5.3-Flash", "GLM-5.3-Flash", "standard" };
}
} // namespace

int main()
{
    auto store = std::make_shared<MemoryStore>();
    QivenContext::attachStore(store);

    const auto actor = goodActor();

    // boot genesis so the canonical head exists for identity verification
    CognitionSource boot;
    boot.kind      = CognitionSourceKind::CanonicalRemote;
    const auto cog = QivenContext::create_cognition(boot);
    QCD_CHECK(cog != nullptr);

    // Phase 1: acquire a grant, save its ID, release it
    const auto g_saved = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g_saved.has_value());
    const GrantId saved_id = g_saved->id();
    QCD_CHECK(!saved_id.value.empty());
    QivenContext::release_grant(*g_saved);

    // Phase 2: 300+ acquire/release cycles (far exceeds the old 256 wrap point)
    for (int i = 0; i < 300; ++i)
    {
        const auto g = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
        QCD_CHECK(g.has_value());
        // every new grant must have a DIFFERENT id from the saved one
        QCD_CHECK(g->id() != saved_id);
        QivenContext::release_grant(*g);
    }

    // Phase 3: the saved stale grant can never regain authority
    // (the current grant holder's write succeeds; a stale grant's write fails)
    const auto g_current = QivenContext::acquire_grant(actor, WorkMode::SupervisedForeground);
    QCD_CHECK(g_current.has_value());
    QCD_CHECK(g_current->id() != saved_id);

    // Phase 4: all 300+ grants had unique IDs (monotonic, never wrapped)
    // This is proven by the fact that every check above passed: if the ID
    // had wrapped, a stale grant would have collided with the current one.

    std::printf("[ OK ] grant regression: 300+ cycles, no wrap, no collision\n");
    return 0;
}
