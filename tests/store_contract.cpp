// ============================================================================
// store_contract — ICognitionStore is the ENTIRE persistence requirement.
// Any storage with incremental append/read suffices; git is one implementation.
// Typed receipts: a CompareFailed and a lost acknowledgement are different
// worlds (review §6); revisions are the CAS token (review §3).
// ============================================================================

#include <qiven/context/context.hpp>

#include "detail/check.hpp"

#include <stdexcept>

using namespace qiven::context;

int main()
{
    MemoryStore store;

    // incremental write: genesis accepts an empty base on an empty store
    const Bytes state1 { std::byte { 'A' } };
    const auto receipt1 = store.compareAndSwap(RevisionId {}, state1);
    QCD_CHECK(receipt1.kind == StoreReceipt::Kind::Committed);
    const RevisionId id1 = receipt1.revision;
    QCD_CHECK(!IsEmpty(id1));
    QCD_CHECK(store.verify(id1));
    QCD_CHECK(store.materialize(id1) == state1);
    QCD_CHECK(store.head() == id1);

    // typed CAS rejection: a wrong base is DEFINITELY not committed (world A)
    const Bytes state2 { std::byte { 'B' } };
    const auto rejected = store.compareAndSwap(RevisionId { "rev-ffffffffffffffff" }, state2);
    QCD_CHECK(rejected.kind == StoreReceipt::Kind::CompareFailed);
    QCD_CHECK(store.head() == id1); // nothing landed

    // a commit chained onto the head succeeds
    const auto receipt2 = store.compareAndSwap(id1, state2);
    QCD_CHECK(receipt2.kind == StoreReceipt::Kind::Committed);
    const RevisionId id2 = receipt2.revision;
    QCD_CHECK(id2 != id1);
    QCD_CHECK(store.materialize(id2) == state2);
    QCD_CHECK(store.head() == id2);

    // revisions chain HISTORY, not content: the same bytes under a different
    // parent mint a different revision (this is what lets a GitStore exist)
    QCD_CHECK(receipt1.revision != receipt2.revision);

    // the revision contract folds incremental reads into materialize:
    // the receiver materializes the target; a real transport encodes the diff
    QCD_CHECK(!store.verify(RevisionId { "rev-0000000000000000" }));

    // idempotent re-commit of the current head
    const auto again = store.compareAndSwap(id2, state2);
    QCD_CHECK(again.kind == StoreReceipt::Kind::Committed);
    QCD_CHECK(again.revision == id2);

    // GitStore is a documented sketch, not an implementation
    GitStore git;
    bool threw = false;
    try
    {
        static_cast<void>(git.compareAndSwap(RevisionId {}, state1));
    }
    catch (const std::logic_error&)
    {
        threw = true;
    }
    QCD_CHECK(threw);

    std::printf("[ OK ] store contract\n");
}
