#pragma once

// ============================================================================
// persistence.hpp — PART 2: store contract, materialization, gated service
//
// v3 phase 2C (semantic closure, per the 2026-09-20 review):
//  - THREE ORTHOGONAL IDENTITIES (review §3): SnapshotDigest (integrity of the
//    canonical snapshot bytes), RevisionId (storage history identity — under a
//    GitStore this is the commit OID), GrantId (runtime authority capability).
//    They are distinct types and must never share a value space again.
//  - IMMUTABLE MATERIALIZATIONS (review §2): a write mints a SUCCESSOR
//    Materialization; nothing mutates a published one. Participants pinning a
//    handle see one consistent world for their whole Work cycle — the alias
//    data race between read_from_cognition and the publish path is gone.
//  - TYPED STORE RECEIPTS (review §6): compareAndSwap returns
//    Committed / CompareFailed / OutcomeUnknown — a CAS failure (definitely
//    not committed) and a lost acknowledgement (possibly committed) are
//    different worlds and can never be conflated again.
//  - UNFORGEABLE GRANTS (review §4): ExecutionGrant is a move-only minted
//    capability with a private constructor; assembling a value-equal forgery
//    is not expressible in the type system.
//  - Idempotency receipts (DR-011) remain IN-PROCESS in this draft; only a
//    restart-capable adapter may claim durable receipts (DR-011 wording).
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
[[nodiscard]] ContentId draft_content_id(const Bytes& stateBytes);

// canonical serialization of one snapshot — the export primitive (K4 shape);
// deterministic for the same snapshot and serialization version
[[nodiscard]] Bytes serialize_snapshot(const Snapshot& snapshot);

// integrity identity of canonical snapshot bytes (review §3)
[[nodiscard]] SnapshotDigest draft_snapshot_digest(const Bytes& stateBytes);

// --- store contract: the ENTIRE persistence requirement ----------------------

struct StoreReceipt // review §6: a CAS failure and a lost acknowledgement are
{                   // different worlds; the contract types them separately
    enum class Kind
    {
        Committed,      // revision appended; `revision` is valid
        CompareFailed,  // base != head: definitely NOT committed
        OutcomeUnknown, // possibly committed; acknowledgement lost
    };
    Kind kind { Kind::CompareFailed };
    RevisionId revision; // meaningful when Committed
};

class ICognitionStore
{
public:
    virtual ~ICognitionStore() = default;

    // compare-and-swap on the REVISION (storage identity): append the state
    // chained onto `base`. CompareFailed means definitely not committed;
    // OutcomeUnknown means possibly committed with the acknowledgement lost.
    [[nodiscard]] virtual StoreReceipt compareAndSwap(const RevisionId& base,
                                                      const Bytes& stateBytes) = 0;

    // read the state at a revision (git: checkout / cat-file)
    [[nodiscard]] virtual Bytes materialize(const RevisionId& revision) const = 0;

    [[nodiscard]] virtual bool verify(const RevisionId& revision) const = 0; // git: cat-file -e

    [[nodiscard]] virtual RevisionId head() const = 0;
};

// in-memory reference implementation (proves the contract; used by tests/demo)
class MemoryStore final : public ICognitionStore
{
public:
    [[nodiscard]] StoreReceipt compareAndSwap(const RevisionId& base, const Bytes& stateBytes) override;
    [[nodiscard]] Bytes materialize(const RevisionId& revision) const override;
    [[nodiscard]] bool verify(const RevisionId& revision) const override;
    [[nodiscard]] RevisionId head() const override;

private:
    std::vector<std::pair<RevisionId, Bytes>> revisions_; // linear history, genesis first
    RevisionId head_ {};
};

// git implementation — DOCUMENTED SKETCH, deliberately not implemented here.
// The store contract is proven by MemoryStore; this class records the mapping.
// With RevisionId as the CAS token the mapping is now implementable:
//
//   compareAndSwap(base, bytes) -> git commit with parent = base (RevisionId =
//                                  commit OID); CompareFailed = non-fast-forward
//   materialize(revision)       -> git checkout / cat-file at that OID
//   verify(revision)            -> git cat-file -e
//   head()                      -> rev-parse HEAD
//
// (The v3 phase 1 contract passed the SNAPSHOT DIGEST as the parent token,
// which no real storage can chain on — review §3. This sketch is why the two
// identities are now separate types.)
//
// Layered transport ceremonies, NOT part of the store contract:
//   push/pull               -> replica sync between devices
//   PR + review + merge     -> the H2 acceptance ceremony over the human-AI channel
//   GitHub Actions CI       -> a REMOTE DEVICE validating appended states; evidence
//                              flows back into cognition via ordinary gated writes
class GitStore final : public ICognitionStore
{
public:
    [[nodiscard]] StoreReceipt compareAndSwap(const RevisionId& base, const Bytes& stateBytes) override;
    [[nodiscard]] Bytes materialize(const RevisionId& revision) const override;
    [[nodiscard]] bool verify(const RevisionId& revision) const override;
    [[nodiscard]] RevisionId head() const override;
};

// --- materialization (DR-001 / DR-010 / review §2) ----------------------------

enum class QuarantineState
{
    Isolated,         // restored from an artifact; non-authoritative (K4)
    Verified,         // integrity and completeness validated; still non-authoritative
    AuthorityPending, // a governed cutover has been proposed
    Promoted,         // authoritative via the governed promotion operation only
};

// A Materialization is IMMUTABLE once minted (review §2): a write produces a
// SUCCESSOR materialization; nothing mutates a published one. A participant
// pinning a handle therefore sees one consistent world for its whole Work
// cycle, and readers never race the publish path.
struct Materialization
{
    std::shared_ptr<const Snapshot> state; // immutable value tree
    SnapshotDigest digest;                 // integrity identity of the state bytes
    RevisionId revision;                   // storage history identity (store CAS token)
    Epoch epoch { 0 };                     // runtime fencing token (writer lease lineage)
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

// --- work modes, actors, grants (DR-003 / review §4) ---------------------------

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

// UNFORGEABLE LEASE (review §4): private constructor, move-only, no default.
// A grant is minted only by QivenContext::acquire_grant and carries a minted
// GrantId the service validates against its active lease — assembling a
// value-equal forgery is not expressible in the type system.
class ExecutionGrant
{
public:
    ExecutionGrant(const ExecutionGrant&)            = delete;
    ExecutionGrant& operator=(const ExecutionGrant&) = delete;
    ExecutionGrant(ExecutionGrant&&)                 = default;
    ExecutionGrant& operator=(ExecutionGrant&&)      = default;
    ~ExecutionGrant()                                = default;

    [[nodiscard]] const GrantId& id() const
    {
        return id_;
    }
    [[nodiscard]] WorkMode mode() const
    {
        return mode_;
    }

private:
    friend class QivenContext;
    ExecutionGrant(GrantId id, Epoch epoch, PrincipalId principal, WorkMode mode);
    GrantId id_;
    Epoch epoch_ { 0 };
    PrincipalId principal_;
    WorkMode mode_ { WorkMode::SupervisedForeground };
};

// identity port: verification happens here, never by caller assertion. Ports
// are called WITH the governance snapshot the service supplies and NEVER
// re-enter QivenContext (pit P-42 — the 2026-09-19 incident: a verifier that
// called canonical_head() re-locked the admission mutex on the same thread and
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
        UpsertProfile,        // EvidenceWrite: profile records views resolve against
        OpenConflict,         // ConflictWrite
        ResolveConflict,      // ConflictWrite
        AmendViewSpec,        // EvidenceWrite-class: durable view adaptation
        SetNextBoundary,      // StateUpdate: bind nextBoundaryRef to an open obligation
    };
    Kind kind { Kind::AddMemory };
    std::int64_t recordId { 0 };          // decision/obligation/conflict id, per kind
    std::int64_t successorRecordId { 0 }; // supersede/transition successor
    std::uint8_t aux { 0 };               // TransitionObligation: Obligation::Status
    std::string scope;                    // conflict scope (empty = global); view agent:human
    std::string title;
    std::string payload;
    std::string provenanceRef; // required for AddMemory (constitution 9);
                               // AmendViewSpec: comma-separated profile refs
};

struct H2Evidence                  // content-bound AND authority-bound review evidence
{                                  // (DR-004 + review §5: reviewer identity, binding and authority)
    ContentId reviewedDeltaDigest; // over the serialized ordered operations
    PrincipalId reviewer;          // the reviewer's principal (verified at the gate)
    std::string reviewerBinding;   // the reviewer's model-instance binding
    std::string reviewRef;         // transport pointer: PR record / audit id
};

struct ContextTransaction
{
    RevisionId base;                   // durable fencing token: the revision my thinking assumed
    std::vector<Operation> operations; // atomic; validated as a whole, applied as a whole
    std::string handoffEvidenceRef;    // transport pointer (PR record / audit id)
    std::optional<H2Evidence> h2;      // present iff the policy table requires it
    std::string idempotencyKey;        // when non-empty: same key + same request resolves
                                       // to the original durable outcome and never
                                       // re-executes; same key + different request is
                                       // refused (DR-011, ADR-0033 section 4)
};

// content identity of the ordered operations — what an H2 review binds to
[[nodiscard]] ContentId digest_operations(const std::vector<Operation>& operations);

struct ReviewRecord // the process-layer artifact an H2Evidence comes FROM;
                    // pit.merge_surrogate_refused: publication paths are
                    // process-level (named-branch push / gh pr create only)
{                   // (process-model §2): the digest binds it to the exact
                    // delta; the binding enables the self-certification ban
    ContentId reviewedDeltaDigest;
    PrincipalId reviewer;
    std::string reviewerBinding;
    std::string reviewRef;
    bool sameSessionLimitation { false }; // disclosed when reviewer ~= author instance
    std::vector<std::string> findings;    // blocking / non-blocking, with evidence
};

enum class ContinuityTrialKind // ADR-0034 + pit.cold_boot_not_artifact_trial:
{                              // delivery profiles are distinct properties; a remote cold boot
    RemoteColdBoot,            // NEVER substitutes for an artifact trial, and
    CanonicalArtifactHandoff,  // acceptance trials are REAL topologies — they
    HumanSuccession,           // cannot be simulated by the authoring session
};

struct Verdict // the write gate's typed outcome (DR-002); a bool erases the
{              // recovery rule — the reason IS cognition
    enum class Outcome
    {
        Applied,
        Refused,
        OutcomeUnknown, // possibly committed; receipt recorded, state not advanced
    };
    Outcome outcome { Outcome::Refused };
    RefusalReason reason { RefusalReason::StoreDiverged };
    CognitionHandle successor; // the SUCCESSOR MATERIALIZATION when Applied
                               // (review §2: successors are minted, not mutated in)
};

// --- read side: view resolution, bundles, output views (DR-007 / DR-008) ------

enum class CognitionSourceKind
{
    CanonicalRemote,  // cold boot: git fetch (transport impl #1)
    HandoffArtifact,  // K4: artifact-only until sealed (impl #2)
    CompressedStream, // K5: lossless transport (impl #3); pit.k5_transport_only
};

struct CognitionSource
{
    CognitionSourceKind kind { CognitionSourceKind::CanonicalRemote };
    RevisionId revision;      // addressed by REVISION (storage identity); empty = head
    Bytes inlineBytes;        // HandoffArtifact: the artifact payload itself
    ContentId expectedDigest; // artifact integrity: when non-empty, the payload
                              // must hash to this id BEFORE deserialization
                              // (corruption fails closed — pit P-23)
};

struct Query
{
    std::string taskScope;      // mandatory inputs always included (BOOTSTRAP set)
    std::size_t tokenBudget {}; // approximate-token budget for CANDIDATES only
};

using Data = Bytes; // full materialized snapshot for thinking (ADR-0033)

// --- the read plane: view resolution and the ContextBundle (DR-007 / DR-008) ----

enum class OutputView
{
    Human,   // staged, operator-legible rendering
    Machine, // deterministic flat rendering for automation; both views of ONE result
};

struct ResolveDiagnostic
{
    enum class Kind
    {
        None,        // resolved
        NotFound,    // unknown view id: fall back to the identity-independent context,
                     // never invent a participant combination (pit P-41)
        InvalidRefs, // empty/duplicated refs or refs that do not resolve in-snapshot
                     // fail compilation (pit P-40)
    };
    Kind kind { Kind::None };
};

struct BundleCandidate
{
    std::string kind; // "decision" / "memory" / "obligation" — epistemic type preserved
    std::string id;
    std::string summary;
};

struct ObligationEvaluation
{
    std::int64_t id;
    enum class Evaluation
    {
        Met,
        Unknown,
        Unmet,
    };
    Evaluation evaluation { Evaluation::Unknown };
};

struct BundleOmission
{
    std::string what;
    std::string reason;
};

struct ContextBundle
{
    RevisionId snapshot;                           // one revision; never mixed
    std::vector<std::string> mandatoryInputs;      // SEMANTIC floors: real content
    std::vector<std::string> protectedConstraints; // verbatim; never elided
    std::vector<BundleCandidate> candidates;       // typed candidates, never truth
    std::vector<ObligationEvaluation> obligations; // three-valued (ADR-0033 §6)
    std::vector<BundleOmission> omissions;         // what was left out and why
};

// --- the service (single-writer gate + registry) ------------------------------

class QivenContext
{
public:
    // bind the process-wide persistence plane (the swappable storage impl)
    static void attachStore(std::shared_ptr<ICognitionStore> store);

    // bind the identity port; defaults to the root-principal verifier below
    static void attach_identity_verifier(std::shared_ptr<IIdentityVerifier> verifier);

    // materialize a fresh cognition. read-only w.r.t. project truth (R6).
    // CanonicalRemote -> cold boot (Promoted: the remote IS canonical);
    // HandoffArtifact -> quarantined Isolated (K4: restore is not authority).
    // Returns nullptr on a corrupt source with *err filled (DR-009).
    static CognitionHandle create_cognition(const CognitionSource& source,
                                            DeserializeError* err = nullptr);

    // full snapshot for thinking (immutable — race-free by construction)
    static Data read_from_cognition(const CognitionHandle& handle, const Query& query);

    // single-writer lease: exactly one active grant chain-wide. A second
    // acquire while one is held is refused fail-closed (pit P-01) — competing
    // flows are quarantined, never queued. Identity is verified at acquire
    // AND re-checked at write (ADR-0033 section 4). The minted GrantId is the
    // only capability the gate accepts (review §4).
    [[nodiscard]] static std::optional<ExecutionGrant> acquire_grant(const AuthenticatedActor& actor,
                                                                     WorkMode mode);
    static void release_grant(const ExecutionGrant& grant);

    // the gated write. Gate order is normative and tested:
    //   0. handle known + non-quarantined          -> GovernanceDenied
    //   1. grant id is the active lease            -> GrantRefused
    //   2. actor re-verified                       -> UnverifiedActor
    //   3. idempotency key resolution              -> replay original verdict /
    //                                                 KeyConflict
    //   4. unattended + mutating                   -> UnattendedMutation
    //   5. base revision CAS                       -> StaleBase
    //   6. policy table + authority-bound H2       -> HandoffMissing / HandoffInvalid
    //                                                 / GovernanceDenied
    //   7. open-conflict scope intersection        -> ConflictUnresolved
    //   8. invariants (whole transaction)          -> InvariantFailed
    //   9. post-apply state coherence              -> InvariantFailed
    //  10. store compareAndSwap                    -> StoreDiverged (CompareFailed)
    //                                                 / OutcomeUnknown (lost ack)
    // An empty operations list is an ordinary turn: Applied, nothing stored.
    // On Applied the SUCCESSOR MATERIALIZATION is returned and the registry
    // advances to it — the caller's old handle keeps its pinned world.
    static Verdict write_to_cognition(const CognitionHandle& handle,
                                      const ContextTransaction& transaction,
                                      const ExecutionGrant& grant);

    // the recovery rule for a refusal, read from cognition (DR-002); missing
    // rows fall back to FailClosed
    [[nodiscard]] static RecoveryAction recovery_for(const Snapshot& snapshot, RefusalReason reason);

    // view resolution: a pure read-time transformation. Unknown ids fall back
    // to the identity-independent context (nullopt + NotFound) — a participant
    // combination is never invented (S1-R2); invalid refs fail compilation.
    [[nodiscard]] static std::optional<ViewSpec> ResolveView(const CognitionHandle& handle,
                                                             const std::string& viewId,
                                                             ResolveDiagnostic* diag = nullptr);

    // the bundle: floors and protected constraints survive any budget; the
    // budget shrinks candidates only, with omissions explained
    [[nodiscard]] static ContextBundle BuildBundle(const CognitionHandle& handle,
                                                   const Query& query);

    // two renderings of one result; neither audience scrapes the other
    [[nodiscard]] static std::string RenderBundle(const ContextBundle& bundle, OutputView view);

    // drop the service's owning reference; pinned handles stay alive but fenced
    // (lifetime = shared_ptr; authority = the active grant — the v1 conflation,
    // split in v2, made a type shape in v3)
    static bool retire_cognition(const CognitionHandle& handle);

    // quarantine state machine (fail-closed): Isolated -> Verified requires
    // the root principal (integrity was already checked at restore);
    // AuthorityPending -> Promoted is the governed cutover act itself —
    // checksum success, import or uptime NEVER promote (review §10, S10-R2).
    // Nothing in this draft sets AuthorityPending: that requires a canonical
    // cutover ADR, which is out of draft scope by design.
    [[nodiscard]] static CognitionHandle verify_restored(const CognitionHandle& handle,
                                                         const AuthenticatedActor& actor);
    [[nodiscard]] static CognitionHandle promote_authority(const CognitionHandle& handle,
                                                           const AuthenticatedActor& actor);

    // the canonical head snapshot, for ports that verify against live
    // governance; empty principal on an unreadable store
    [[nodiscard]] static std::shared_ptr<const Snapshot> canonical_head();

    // the ONLY whole-graph predicate; validated for EDGE consistency too —
    // the client must actually reference the llm and device passed in, the
    // llm must be bound to this handle, and the binding disclosure must match
    // the LLM identity (review §9/§10)
    static bool is_continueable(const Human* h, const LLMClientTool* c, const Device* d,
                                const LLM* l, const CognitionHandle& handle);

    [[nodiscard]] static Epoch current_epoch(); // lock-free sampling for pre-checks

private:
    struct Registry
    {
        CognitionHandle materialization; // IMMUTABLE; replaced on commit, never mutated
    };
    // canonical head snapshot; caller MUST hold g_admission (ports receive it
    // as context — P-42: ports never re-enter the service while it is locked)
    [[nodiscard]] static std::shared_ptr<const Snapshot> canonical_head_locked();
    static std::mutex g_admission;     // serializes grant state + writes
    static std::atomic<Epoch> g_epoch; // monotonic; fetch_add on each create_cognition
    static std::shared_ptr<ICognitionStore> g_store;
    static std::shared_ptr<IIdentityVerifier> g_identity;
    static std::vector<Registry> g_live; // live materializations, epoch-indexed
    static GrantId g_activeGrantId;      // the minted id of the active lease
    static AuthenticatedActor g_grantActor;
    struct Receipt
    {
        std::string requestDigest; // digest of the complete request (base, ops, evidence)
        Verdict outcome;           // the durable outcome the key resolves to
    };
    static std::map<std::string, Receipt> g_receipts; // IN-PROCESS idempotency receipts
                                                      // (DR-011: durable receipts require
                                                      // a restart-capable adapter)
};
} // namespace qiven::context
