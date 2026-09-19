#pragma once

// ============================================================================
// persistence.hpp — PART 2: store contract, materialization, gated service
//
// v3 phase 1 (DR-001/002/003/004/009/010):
//  - Snapshot is immutable; Materialization carries runtime identity and
//    authority. Participants hold shared_ptr<const Materialization> — the
//    LLM never holds a mutable reference to cognition, as a type shape.
//  - Writes take an ExecutionGrant minted by the in-process authority: one
//    active grant chain-wide (a second acquire is refused fail-closed — the
//    split-brain pit), base CAS (durable fencing), the policy table, and
//    content-bound H2 evidence. Every refusal is a typed Verdict whose reason
//    maps to a recovery rule stored IN cognition.
//  - Untrusted deserialization fails closed with typed errors — never asserts
//    (which compile away in Release).
//  - epoch fences the writer lease (DR-010); re-materializing for reading
//    never revokes a writer.
// ============================================================================

#include <qiven/context/cognition.hpp>

#include <atomic>
#include <cstddef>
#include <map>
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

// canonical serialization of one snapshot — the export primitive (K4 shape);
// deterministic for the same snapshot and serialization version
[[nodiscard]] Bytes SerializeSnapshot(const Snapshot& snapshot);

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
    ContentId head_ {};
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

// --- materialization (DR-001 / DR-010) ---------------------------------------

enum class QuarantineState
{
    Isolated,         // restored from an artifact; non-authoritative (K4)
    Verified,         // integrity and completeness validated; still non-authoritative
    AuthorityPending, // a governed cutover has been proposed
    Promoted,         // authoritative via the governed promotion operation only
};

struct Materialization
{
    std::shared_ptr<const Snapshot> state; // immutable value tree
    ContentId contentId;                   // durable fencing token
    Epoch epoch { 0 };                     // runtime fencing token (writer lease)
    QuarantineState quarantine { QuarantineState::Isolated };
};
using CognitionHandle = std::shared_ptr<const Materialization>;

// --- typed failure channels (DR-009): untrusted input never asserts ----------

struct DeserializeError
{
    enum class Kind
    {
        Truncated,      // input ends inside a field, or trailing bytes remain
        BadVersion,     // serialization version mismatch
        BadEnum,        // encoded value outside the declared enum range
        ResourceAbuse,  // declared counts would allocate unbounded state
        DigestMismatch, // artifact bytes do not match the declared content id (K4)
    };
    Kind kind { Kind::Truncated };
    std::size_t offset { 0 };
    std::string detail;
};

struct DeserializeResult
{
    bool ok { false };
    Snapshot snapshot;
    DeserializeError error;
};

// --- work modes, actors, grants (DR-003) --------------------------------------

enum class WorkMode
{
    SupervisedForeground, // owner-launched, owner-visible session
    Unattended,           // scheduled/idle: read-only default (ADR-0036)
};

struct AuthenticatedActor
{
    PrincipalId principal; // verified by the identity port, never self-asserted
    Role role { Role::Worker };
    std::string binding;      // which model instance fulfills the role
    std::string servingModel; // disclosure duty (ADR-0035 rule 4)
    std::string reasoning;    // serving reasoning effort
};

[[nodiscard]] inline bool operator==(const AuthenticatedActor& a, const AuthenticatedActor& b)
{
    return a.principal == b.principal && a.role == b.role && a.binding == b.binding && a.servingModel == b.servingModel && a.reasoning == b.reasoning;
}

struct ExecutionGrant
{
    Epoch epoch { 0 };
    PrincipalId principal;
    WorkMode mode { WorkMode::SupervisedForeground };
};

[[nodiscard]] inline bool operator==(const ExecutionGrant& a, const ExecutionGrant& b)
{
    return a.epoch == b.epoch && a.principal == b.principal && a.mode == b.mode;
}

// identity port: verification happens here, never by caller assertion. Ports
// are called WITH the governance snapshot the service supplies and NEVER
// re-enter QivenContext (pit P-42 — the 2026-09-19 incident: a verifier that
// called CanonicalHead() re-locked the admission mutex on the same thread and
// threw resource_deadlock_would_occur; uncaught, it terminated silently).
// Production verifies against the live identity provider and re-checks at
// commit time (ADR-0033 section 4).
class IIdentityVerifier
{
public:
    virtual ~IIdentityVerifier()                                              = default;
    [[nodiscard]] virtual bool verify(const AuthenticatedActor& actor,
                                      const Snapshot& governanceSource) const = 0;
};

// --- transactions and verdicts (DR-002 / DR-004 / DR-005) ---------------------

struct Operation
{
    enum class Kind
    {
        AppendDecision,       // DecisionAcceptance (merge-class)
        SupersedeDecision,    // DecisionAcceptance: lifecycle transition
        AddMemory,            // MemoryWrite
        UpsertObligation,     // ObligationWrite
        CloseObligation,      // ObligationWrite
        TransitionObligation, // ObligationWrite: Done/Cancelled/Superseded
        UpdateState,          // StateUpdate
        AddEvidence,          // EvidenceWrite
        OpenConflict,         // ConflictWrite
        ResolveConflict,      // ConflictWrite
    };
    Kind kind { Kind::AddMemory };
    std::int64_t recordId { 0 };          // decision/obligation/conflict id, per kind
    std::int64_t successorRecordId { 0 }; // supersede/transition successor
    std::uint8_t aux { 0 };               // TransitionObligation: Obligation::Status
    std::string scope;                    // conflict scope (empty = global)
    std::string title;
    std::string payload;
    std::string provenanceRef; // required for AddMemory (constitution 9)
};

struct H2Evidence // content-bound review evidence (DR-004, pit P-05)
{
    ContentId reviewedDeltaDigest; // over the serialized ordered operations
    PrincipalId reviewer;          // the delegated reviewer's principal
    std::string reviewRef;         // transport pointer: PR record / audit id
};

struct ContextTransaction
{
    ContentId base;                    // durable fencing token: the state my thinking assumed
    std::vector<Operation> operations; // atomic; validated as a whole, applied as a whole
    std::string handoffEvidenceRef;    // transport pointer (PR record / audit id)
    std::optional<H2Evidence> h2;      // present iff the policy table requires it
    std::string idempotencyKey;        // when non-empty: same key + same request resolves
                                       // to the original durable outcome and never
                                       // re-executes; same key + different request is
                                       // refused (DR-011, ADR-0033 section 4)
};

// content identity of the ordered operations — what an H2 review binds to
[[nodiscard]] ContentId DigestOperations(const std::vector<Operation>& operations);

struct Verdict // the write gate's typed outcome (DR-002); a bool erases the
{              // recovery rule — the reason IS cognition
    enum class Outcome
    {
        Applied,
        Refused,
        OutcomeUnknown, // produced by Phase 2 receipts; reserved here
    };
    Outcome outcome { Outcome::Refused };
    RefusalReason reason { RefusalReason::StoreDiverged };
    ContentId successorId; // meaningful when Applied
};

// --- read side (Bundle lands in Phase 2; K5 stays transport-only) ------------

enum class CognitionSourceKind
{
    CanonicalRemote,  // cold boot: git fetch (transport impl #1)
    HandoffArtifact,  // K4: artifact-only until sealed (impl #2)
    CompressedStream, // K5: lossless transport (impl #3)
};

struct CognitionSource
{
    CognitionSourceKind kind { CognitionSourceKind::CanonicalRemote };
    ContentId contentId;      // addressed by content — NOT necessarily a commit;
                              // empty = store head (genesis when store is empty)
    Bytes inlineBytes;        // HandoffArtifact: the artifact payload itself
    ContentId expectedDigest; // artifact integrity: when non-empty, the payload
                              // must hash to this id BEFORE deserialization
                              // (corruption fails closed — pit P-23)
};

struct Query
{
    std::string taskScope;      // mandatory inputs always included (BOOTSTRAP set)
    std::size_t tokenBudget {}; // Bundle domain (phase 2); K5 is transport-only
};

using Data = Bytes; // full materialized snapshot for thinking (ADR-0033)

// --- the service (single-writer gate + registry) ------------------------------

class QivenContext
{
public:
    // bind the process-wide persistence plane (the swappable storage impl)
    static void attachStore(std::shared_ptr<ICognitionStore> store);

    // bind the identity port; defaults to the root-principal verifier below
    static void attachIdentityVerifier(std::shared_ptr<IIdentityVerifier> verifier);

    // materialize a fresh cognition. read-only w.r.t. project truth (R6).
    // CanonicalRemote -> cold boot (Promoted: the remote IS canonical);
    // HandoffArtifact -> quarantined Isolated (K4: restore is not authority).
    // Returns nullptr on a corrupt source with *err filled (DR-009).
    static CognitionHandle CreateCognition(const CognitionSource& source,
                                           DeserializeError* err = nullptr);

    // full snapshot for thinking; the typed Bundle with floors lands in Phase 2
    static Data ReadFromCognition(const CognitionHandle& handle, const Query& query);

    // single-writer lease: exactly one active grant chain-wide. A second
    // acquire while one is held is refused fail-closed (pit P-01) — competing
    // flows are quarantined, never queued. Identity is verified at acquire
    // AND re-checked at write (ADR-0033 section 4).
    [[nodiscard]] static std::optional<ExecutionGrant> AcquireGrant(const AuthenticatedActor& actor,
                                                                    WorkMode mode);
    static void ReleaseGrant(const ExecutionGrant& grant);

    // the gated write. Gate order is normative and tested:
    //   0. handle known + non-quarantined          -> GovernanceDenied
    //   1. grant is the active lease               -> GrantRefused
    //   2. actor re-verified                       -> UnverifiedActor
    //   3. idempotency key resolution              -> replay original verdict /
    //                                                 KeyConflict
    //   4. unattended + mutating                   -> UnattendedMutation
    //   5. base CAS                                -> StaleBase
    //   6. policy table + content-bound H2         -> HandoffMissing / HandoffInvalid
    //                                                 / GovernanceDenied
    //   7. invariants (whole transaction)          -> InvariantFailed
    //   8. apply atomically; store CAS             -> StoreDiverged
    // An empty operations list is an ordinary turn: Applied, nothing stored.
    // A non-empty key records a durable receipt; a lost store acknowledgement
    // yields Verdict{OutcomeUnknown, OutcomeUnresolved} — never a rollback
    // claim — and the same key resolves to the receipt until it is replaced.
    static Verdict WriteToCognition(const CognitionHandle& handle,
                                    const ContextTransaction& transaction,
                                    const ExecutionGrant& grant);

    // the recovery rule for a refusal, read from cognition (DR-002); missing
    // rows fall back to FailClosed
    [[nodiscard]] static RecoveryAction RecoveryFor(const Snapshot& snapshot, RefusalReason reason);

    // drop the service's owning reference; pinned handles stay alive but fenced
    // (lifetime = shared_ptr; authority = the active grant — the v1 conflation,
    // split in v2, made a type shape in v3)
    static bool RetireCognition(const CognitionHandle& handle);

    // the canonical head snapshot, for ports that verify against live
    // governance; empty principal on an unreadable store
    [[nodiscard]] static std::shared_ptr<const Snapshot> CanonicalHead();

    // the ONLY whole-graph predicate; evaluated by the loop, never by a participant
    static bool IsContinueable(const Human* h, const LLMClientTool* c, const Device* d,
                               const LLM* l, const CognitionHandle& handle);

    [[nodiscard]] static Epoch currentEpoch(); // lock-free sampling for pre-checks

private:
    struct Registry
    {
        std::shared_ptr<Materialization> live; // service-owned mutable view
    };
    // canonical head snapshot; caller MUST hold g_admission (ports receive it
    // as context — P-42: ports never re-enter the service while it is locked)
    [[nodiscard]] static std::shared_ptr<const Snapshot> CanonicalHeadLocked();
    static std::mutex g_admission;     // serializes grant state + writes
    static std::atomic<Epoch> g_epoch; // monotonic; fetch_add on each CreateCognition
    static std::shared_ptr<ICognitionStore> g_store;
    static std::shared_ptr<IIdentityVerifier> g_identity;
    static std::vector<Registry> g_live; // live materializations, epoch-indexed
    static std::optional<ExecutionGrant> g_activeGrant;
    static AuthenticatedActor g_grantActor;
    struct Receipt
    {
        std::string requestDigest; // digest of the complete request (base, ops, evidence)
        Verdict outcome;           // the durable outcome the key resolves to
    };
    static std::map<std::string, Receipt> g_receipts; // key -> receipt (draft: in-process)
};
} // namespace qiven::context
