#pragma once

// ============================================================================
// cognition.hpp — the Snapshot value tree + authority policy data (PART 1)
//
// R1: Snapshot is a PURE VALUE TREE. Zero pointers, zero references to
//     runtime participants — snapshot-able, restorable, compressible (K4/K5),
//     and the drift detector: a surface that cannot be a member below is not
//     context.
//
// v3 phase 1 (DR-001/002/005): the tree no longer carries epoch/contentId —
// runtime identity and authority live in Materialization (persistence.hpp).
// The PolicyTable makes R4 real: authority rules are cognition DATA the gate
// interprets, and every refusal carries the recovery rule the loop must
// follow (recovery-as-data, not comments).
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace qiven::context
{
using PrincipalId = std::string; // "github:JasonHuang3D" — account-level auth
using ContentId   = std::string; // SHA-256 of canonical bytes (draft: FNV-1a hex).
                                 // A git commit SHA is ONE minter of ContentId
                                 // (transport); a K4 artifact digest is another.
                                 // Cognition never parses it — compares and records.
using Epoch = std::uint64_t;     // runtime fencing token (authority layer)

// THREE ORTHOGONAL IDENTITIES (review §3) — never share a value space again:
struct SnapshotDigest
{ // integrity identity of the canonical snapshot bytes
    std::string value;
};
struct RevisionId
{ // storage history identity — under a GitStore this is the commit OID
    std::string value;
};
struct GrantId
{ // runtime authority capability, minted by the authority plane only
    std::string value;
};
[[nodiscard]] inline bool operator==(const SnapshotDigest& a, const SnapshotDigest& b)
{
    return a.value == b.value;
}
[[nodiscard]] inline bool operator!=(const SnapshotDigest& a, const SnapshotDigest& b)
{
    return !(a == b);
}
[[nodiscard]] inline bool operator==(const RevisionId& a, const RevisionId& b)
{
    return a.value == b.value;
}
[[nodiscard]] inline bool operator!=(const RevisionId& a, const RevisionId& b)
{
    return !(a == b);
}
[[nodiscard]] inline bool IsEmpty(const SnapshotDigest& d)
{
    return d.value.empty();
}
[[nodiscard]] inline bool IsEmpty(const RevisionId& r)
{
    return r.value.empty();
}
[[nodiscard]] inline bool IsEmpty(const GrantId& g)
{
    return g.value.empty();
}
[[nodiscard]] inline bool operator==(const GrantId& a, const GrantId& b)
{
    return a.value == b.value;
}
[[nodiscard]] inline bool operator!=(const GrantId& a, const GrantId& b)
{
    return !(a == b);
}
using Yaml = std::string; // draft: raw yaml text; kernel will type these

enum class Lifecycle // decision lifecycle (record-lifecycle contract)
{
    Proposed,
    Accepted,
    Superseded,
    Rejected,
};

struct Provenance
{                                     // every canonical record carries it (const. #9)
    std::vector<std::string> sources; // user_statement / git_commit / audit / ...
                                      // transport SHAs are historical facts: live
                                      // verifiable (Phase B), never required to
                                      // reconstruct semantics (Phase A)
};

struct Governance              // authority RULES live here; identities are
{                              // session parameters verified at runtime (R4)
    PrincipalId rootPrincipal; // "github:JasonHuang3D"
};

struct Constitution // meta-rules about cognition itself (const. #4 #5 #13...)
{
    std::vector<std::string> articles; // full article texts: completeness lives
                                       // in the tree, economy in the Bundle (DR-007)
};

// --- authority policy as cognition data (DR-002 / DR-005) --------------------

enum class RefusalReason
{
    GrantRefused,       // a competing flow holds the single-writer lease (P-01)
    UnverifiedActor,    // the identity port refused the actor (P-02)
    StaleBase,          // durable divergence: re-read before writing
    HandoffMissing,     // policy requires typed handoff evidence; none given
    HandoffInvalid,     // evidence not bound to this exact delta (P-05)
    UnattendedMutation, // unattended grants carry no write right (P-08)
    InvariantFailed,    // record invariant violated: design review, not patch
    GovernanceDenied,   // quarantined state or policy refuses this actor/op
    StoreDiverged,      // store-level CAS failed: fail closed
    OutcomeUnresolved,  // commit outcome unknown: dependent work stops
    KeyConflict,        // idempotency key reused with different content
    ConflictUnresolved, // an open canonical conflict blocks this acceptance
};

enum class RecoveryAction
{
    RereadRethink,     // park the delta, re-read, re-think; never blind-retry
    HaltEscalate,      // stop and escalate; retrying is forbidden (no-verbal-waiver)
    DeferToSupervised, // park until a supervised session can act
    DesignReview,      // known hazard class: review, not mechanical patch (const. #17)
    FailClosed,        // stop; competing flow / policy / store refusal
    Block,             // outcome unknown: dependent mutations stop
};

struct RecoveryRule
{
    RefusalReason reason;
    RecoveryAction action;
};

enum class OperationClass
{
    DecisionAcceptance, // merge-class semantics (accept + supersede)
    MemoryWrite,
    ObligationWrite,
    StateUpdate,
    ConflictWrite,
    EvidenceWrite,
};

struct Conflict // constitution 11: conflict is first-class epistemic state;
{               // an OPEN conflict blocks intersecting acceptances (DR-006)
    ContentId id;
    enum class Status
    {
        Open,
        Resolved,
    };
    Status status { Status::Open };
    std::string scope; // empty = global (blocks everything)
    std::string description;
    std::string resolution; // required when Resolved
};

struct HandoffPolicy // the ADR-0036 classification table, as data
{
    OperationClass opClass;
    bool requiresH2;        // H2_Review evidence over the exact delta
    bool rootPrincipalOnly; // governance-mutation class
};

struct PolicyTable
{
    std::vector<HandoffPolicy> handoff;
    std::vector<RecoveryRule> recovery;
};

// --- records ---------------------------------------------------------------

struct Decision // decisions/ADR-####.md
{
    std::int64_t id {};
    Lifecycle status { Lifecycle::Proposed };
    std::string title;
    std::string content;
    std::vector<std::int64_t> supersedes;
    std::vector<std::int64_t> supersededBy;
    Provenance provenance;
};

struct MemoryRecord // memory/ — durable facts, lessons, risks
{
    enum class Kind
    {
        Fact,
        Lesson,
        Protocol,
        Risk,
        Invariant,
        NegativeKnowledge, // rejected alternatives are first-class (const. #7)
    };
    enum class Status
    {
        Active,
        Superseded,
    };
    Kind kind { Kind::Fact };
    Status status { Status::Active };
    std::string title;
    std::string statement;
    Provenance provenance;
};

struct Obligation // obligations/ — unfinished cognition (const. #6)
{
    enum class Status
    {
        Open,
        Deferred,
        Blocked,
        Done,
        Cancelled,
        Superseded,
    };
    enum class TriggerKind
    {
        OnTouch,
        Before,
        After,
        OnChange,
        OnDate,
        Manual,
    };
    Status status { Status::Open };
    TriggerKind trigger { TriggerKind::Manual }; // non-terminal obligations carry one
    std::int64_t id {};                          // canonical repo uses OBL-... string ids
    std::string statement;
    std::string completionCriteria;
};

struct EvidenceRecord // evidence/audits — immutable once written
{
    ContentId boundTo;   // binds to the exact candidate (content-addressed)
    std::string content; // PASS/FAIL, identities, digests
};

struct StateView                 // compact current operational state, TYPED (pit P-15 and
                                 // pit.no_live_refs_in_state: the
{                                // 2026-09-19 boot found prose state surfaces contradicting
                                 // each other; references make coherence gate-checkable)
    std::string objective;       // the active objective
    std::string current;         // compact narrative (UpdateState writes this)
    ContentId checkpointRef;     // latest session checkpoint identity (empty = none)
    std::string candidateRef;    // current candidate identity ("not-created" allowed)
    std::string nextBoundaryRef; // MUST reference an existing OPEN obligation id
    Yaml repositories;           // STABLE INVENTORY ONLY — never caches live refs
    Yaml roadmap;                // ordered plan items
};

struct ProfileRecord  // the in-snapshot resolution target for view profileRefs
{                     // (review §7: refs must RESOLVE, not merely be non-empty)
    ContentId id;     // e.g. "views/environments/jasonpc"
    std::string kind; // environment / preference / workflow
    std::string summary;
};

struct ViewSpec                           // DURABLE participant adaptation — context, not runtime (DR-008,
{                                         // P-40: exports must carry it; integrity is gate-checked). The
                                          // live session binding stays a runtime parameter.
    ContentId id;                         // "zcode-jason"
    std::string agent;                    // agent family
    std::string human;                    // human id
    std::string summary;                  // what this view adapts
    std::vector<std::string> profileRefs; // preference/environment/workflow refs:
                                          // non-empty, no duplicates (integrity)
};

enum class Role // canonical authority packages (R4)
{
    Owner,
    Brother,
    Worker,
};

enum class Handoff // typed human handoffs (ADR-0036)
{
    H1_Execution,        // owner physically runs the acceptance producer
    H2_Review,           // owner or DELEGATED reviewer accepts exact delta
    H3_Authority,        // human succession
    H4_RecoveryPresence, // cryptographic local presence (ADR-0029)
};

struct Snapshot // PURE VALUE TREE — zero pointers, ever (R1); assignable,
{               // copyable; immutable once minted (DR-001)
    Governance governance;
    Constitution constitution;
    PolicyTable policy;
    StateView state;
    std::vector<Decision> decisions;
    std::vector<MemoryRecord> memory;
    std::vector<Obligation> obligations;
    std::vector<EvidenceRecord> evidence;
    std::vector<Conflict> conflicts;
    std::vector<ProfileRecord> profiles; // view profileRefs resolve against these
    std::vector<ViewSpec> views;
};
} // namespace qiven::context
