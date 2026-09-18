#pragma once

// ============================================================================
// persistence.hpp — PART 2: the store contract, QivenContext service
//
// The repository stores the serialized LLMCognition itself. The ENTIRE
// persistence requirement is ICognitionStore — any data storage with
// incremental append/read suffices. Git is one implementation; a K4 artifact
// and a K5 stream are others. State-replication, not event-sourcing: recovery
// reads the canonical snapshot, never replays diffs or commit graphs.
//
// Concurrency (v2): lock the ADMISSION, never the WORK.
//   locked    : CreateCognition / WriteToCognition / RetireCognition (one mutex)
//   lock-free : ReadFromCognition (immutable snapshot), IsContinueable (atomic load)
// ============================================================================

#include <qiven/context/cognition.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace qiven::context
{
// runtime participants (full types in runtime.hpp); only pointers cross this layer
struct Human;
struct LLM;
struct LLMClientTool;
struct Device;

using Bytes = std::vector<std::byte>; // draft serialization payload (versioned TLV)

// draft content hashing (FNV-1a hex); production mints SHA-256 ContentIds
[[nodiscard]] ContentId DraftContentId(const Bytes& stateBytes);

// --- store contract: the ENTIRE persistence requirement ---------------------

class ICognitionStore
{
public:
    virtual ~ICognitionStore() = default;

    // incremental write: full new state, chained onto `base` (git: commit)
    // returns the content id of the appended state; empty ContentId on reject
    [[nodiscard]] virtual ContentId append(const Bytes& stateBytes, const ContentId& base) = 0;

    // read the state at a content id (git: checkout / cat-file)
    [[nodiscard]] virtual Bytes materialize(const ContentId& id) const = 0;

    // incremental read between two content ids (git: fetch/pack — transport
    // optimization; the contract only requires `target` to be materializable)
    [[nodiscard]] virtual Bytes readDelta(const ContentId& base, const ContentId& target) const = 0;

    [[nodiscard]] virtual bool verify(const ContentId& id) const = 0; // git: cat-file -e

    [[nodiscard]] virtual ContentId head() const = 0;
};

// in-memory reference implementation (proves the contract; used by tests/demo)
class MemoryStore final : public ICognitionStore
{
public:
    [[nodiscard]] ContentId append(const Bytes& stateBytes, const ContentId& base) override;
    [[nodiscard]] Bytes materialize(const ContentId& id) const override;
    [[nodiscard]] Bytes readDelta(const ContentId& base, const ContentId& target) const override;
    [[nodiscard]] bool verify(const ContentId& id) const override;
    [[nodiscard]] ContentId head() const override;

private:
    std::vector<std::pair<ContentId, Bytes>> states_; // linear history, genesis first
    ContentId head_{};
};

// git implementation — DOCUMENTED SKETCH, deliberately not implemented here.
// The store contract is proven by MemoryStore; this class records the mapping.
//
//   append(state, base)     -> git commit (parent = head; ContentId = commit SHA)
//   materialize(id)         -> git checkout / cat-file at that SHA
//   readDelta(base, target) -> git fetch/pack between two SHAs
//   verify(id)              -> git cat-file -e
//
// Layered transport ceremonies, NOT part of the store contract:
//   push/pull               -> replica sync between devices
//   PR + review + merge     -> the H2 acceptance ceremony over the human-AI channel
//   GitHub Actions CI       -> a REMOTE DEVICE validating appended states; evidence
//                              flows back into cognition via ordinary gated writes
class GitStore final : public ICognitionStore
{
public:
    [[nodiscard]] ContentId append(const Bytes& stateBytes, const ContentId& base) override;
    [[nodiscard]] Bytes materialize(const ContentId& id) const override;
    [[nodiscard]] Bytes readDelta(const ContentId& base, const ContentId& target) const override;
    [[nodiscard]] bool verify(const ContentId& id) const override;
    [[nodiscard]] ContentId head() const override;
};

// --- read/write shapes ------------------------------------------------------

enum class CognitionSourceKind
{
    CanonicalRemote,                   // cold boot: git fetch (transport impl #1)
    HandoffArtifact,                   // K4: artifact-only until sealed (impl #2)
    CompressedStream,                  // K5: lossless transport (impl #3)
};

struct CognitionSource
{
    CognitionSourceKind kind{CognitionSourceKind::CanonicalRemote};
    ContentId contentId;               // addressed by content — NOT necessarily a commit;
                                       // empty = store head (genesis when store is empty)
    Bytes inlineBytes;                 // HandoffArtifact: the artifact payload itself
};

struct Query
{
    std::string taskScope;             // mandatory inputs always included (BOOTSTRAP set)
    std::size_t tokenBudget{};         // K5 domain: effective input tokens, lossless
};

using Data = Bytes;                    // full materialized snapshot for thinking (ADR-0033)

struct TransactionDelta                // write shape: only what changed
{
    enum class Kind
    {
        None,                          // ordinary conversational turn: no context commit
        AppendDecision,                // requires H2 (acceptance = merge-class semantics)
        AddMemoryRecord,
        UpsertObligation,
        CloseObligation,
        UpdateState,
    };
    Kind kind{Kind::None};
    ContentId base;                    // durable fencing token: contentId my thinking assumed
    std::int64_t recordId{};           // decision id / obligation id, per kind
    std::string title;
    std::string payload;
    std::optional<Handoff> handoff;    // typed handoff evidence, when the class requires one
    std::string handoffEvidenceRef;    // PR record / gate evidence / audit id
};

enum class WorkMode
{
    SupervisedForeground,              // owner-launched, owner-visible session
    Unattended,                        // scheduled/idle: read-only default (ADR-0036)
};

// --- the service (v0 statics preserved: a service, not an object) ------------

class QivenContext
{
public:
    // bind the process-wide persistence plane (the swappable storage impl)
    static void attachStore(std::shared_ptr<ICognitionStore> store);

    // materialize a fresh cognition. read-only w.r.t. project truth (R6).
    // CanonicalRemote -> cold boot | HandoffArtifact -> K4 quarantine semantics
    static std::shared_ptr<LLMCognition> CreateCognition(const CognitionSource& src);

    // full snapshot for thinking; token budget = K5's optimization target
    static Data ReadFromCognition(const LLMCognition* p, const Query& q);

    // the gated write. admission order is normative (ADR-0036 + fencing):
    //   0. unknown / stale runtime epoch / quarantined -> false
    //   1. base contentId mismatch -> false // durable divergence: re-read
    //   2. handoff unsatisfied   -> false   // H1-H4 by op class; verbal waiver N/A
    //   3. unattended + mutating -> false   // unattended = read-only default
    //   4. validation fails      -> false   // record invariants
    //   5. apply atomically; contentId advances; history by status, never erase
    static bool WriteToCognition(LLMCognition* p, const TransactionDelta& d, WorkMode mode);

    // drop the service's owning reference; pinned Work() cycles finish safely
    // (lifetime = shared_ptr; authority = epoch — v1's conflation, split in v2)
    static bool RetireCognition(const std::shared_ptr<LLMCognition>& p);

    // the ONLY whole-graph predicate; evaluated by the loop, never by a participant
    static bool IsContinueable(const Human* h, const LLMClientTool* c, const Device* d,
                               const LLM* l, const LLMCognition* p);

    [[nodiscard]] static Epoch currentEpoch(); // lock-free sampling for pre-checks

private:
    struct Registry                    // per-epoch runtime metadata, kept OUT of the
    {                                  // value tree (R1): quarantine flags, liveness
        std::shared_ptr<LLMCognition> cognition;
        bool quarantined{false};       // K4: restored artifacts stay non-authoritative
    };

    static std::mutex g_admission;     // the single-writer gate (ADR-0026, in-process)
    static std::atomic<Epoch> g_epoch; // monotonic; fetch_add on each CreateCognition
    static std::shared_ptr<ICognitionStore> g_store;
    static std::vector<Registry> g_live;   // epoch-indexed live cognitions
};
} // namespace qiven::context
