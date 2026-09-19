// ============================================================================
// store_contract — ICognitionStore is the ENTIRE persistence requirement.
// Any storage with incremental append/read suffices; git is one implementation.
// ============================================================================

#include <qiven/context/persistence.hpp>

#include "detail/check.hpp"

#include <stdexcept>

using namespace qiven::context;

int main()
{
    MemoryStore store;

    // incremental write: genesis accepts an empty base on an empty store
    const Bytes state1 { std::byte { 'A' } };
    const ContentId id1 = store.append(state1, {});
    QCD_CHECK(!id1.empty());
    QCD_CHECK(id1 == DraftContentId(state1)); // ContentId = digest of the bytes
    QCD_CHECK(store.verify(id1));
    QCD_CHECK(store.materialize(id1) == state1);
    QCD_CHECK(store.head() == id1);

    // divergence: an append must chain onto the current head
    const Bytes state2 { std::byte { 'B' } };
    QCD_CHECK(store.append(state2, ContentId { "draft-ffffffffffffffff" }).empty());

    const ContentId id2 = store.append(state2, id1);
    QCD_CHECK(!id2.empty() && id2 != id1);
    QCD_CHECK(store.materialize(id2) == state2);
    QCD_CHECK(store.head() == id2);

    // incremental read: the receiver must be able to materialize the target
    QCD_CHECK(store.readDelta(id1, id2) == state2);
    QCD_CHECK(!store.verify(ContentId { "draft-0000000000000000" }));

    // idempotent re-append of the current head
    QCD_CHECK(store.append(state2, id2) == id2);

    // content addressing: identical bytes, identical identity
    QCD_CHECK(DraftContentId(state2) == DraftContentId(state2));
    QCD_CHECK(DraftContentId(state1) != DraftContentId(state2));

    // GitStore is a documented sketch, not an implementation
    GitStore git;
    bool threw = false;
    try
    {
        static_cast<void>(git.append(state1, {}));
    }
    catch (const std::logic_error&)
    {
        threw = true;
    }
    QCD_CHECK(threw);

    std::printf("[ OK ] store contract\n");
}
