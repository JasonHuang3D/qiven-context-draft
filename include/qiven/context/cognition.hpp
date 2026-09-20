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

enum class EpistemicType // constitution §8: hypothesis ≠ fact ≠ observation
{                        // (review §12: the axis is now a type, not a comment)
    Observation,
    Hypothesis,
    Candidate,
    Accepted,
    Verified,
    Rejected,
};

enum class Role // canonical authority packages (R4)
{
    Owner,
    Brother,
    Worker,
};

enum class CapabilityClass // participant capability CLASSES (DR-017); never
{                          // instances — the floor a binding must satisfy
    None,                  // no execution reach required
    LocalSupervised,       // foreground owner-supervised local execution
    HostBrokered,          // through the accepted Host authority path (ADR-0026)
};

struct CapabilityRequirements // the role floor: "can this binding exercise it?"
{
    bool reviewGradeReasoning { false }; // brother floor (ADR-0035 qualification)
    CapabilityClass execution { CapabilityClass::None };
    bool identityPortVerified { false }; // owner floor: human principal authentication
};

enum class WorkflowLayer   // DR-018: workflow-profile dependency layering; the
{                          // default overclaims DEPENDENCE, never independence —
                           // an undeclared layer must fail classification review,
                           // not silently pass as participant-independent
    CanonicalLaw,          // participant-independent process law
    CapabilityProcedure,   // capability-class procedure
    RoleChoreography,      // role-partition choreography (instances never)
    EnvironmentErgonomics, // concrete device/tool/human ergonomics (verify-live)
};

enum class Handoff // typed human handoffs (ADR-0036)
{
    H_None,              // no handoff required for this operation class
    H1_Execution,        // owner physically runs the acceptance producer
    H2_Review,           // owner or DELEGATED reviewer accepts exact delta
    H3_Authority,        // human succession
    H4_RecoveryPresence, // cryptographic local presence (ADR-0029)
};

enum class RetrievalTrigger // constitution §16: a correct engine that is
{                           // not invoked at transitions is a miss (P-18)
    ColdBoot,
    TaskTransition,
    DomainChange,
    MaterialTransaction,
    SessionRollover,
};

struct SourceType // typed provenance source (constitution #9, review §12)
{
    enum class Kind
    {
        UserStatement,
        ChatSession,
        GitCommit,
        Audit,
        RepositoryFile,
        CiRun,
    };
    Kind kind { Kind::UserStatement };
    std::string reference; // the pointer: commit SHA, file path, session id...
    std::string note;      // optional context
};

struct Provenance
{ // every canonical record carries it (const. #9; review §12: typed, not strings)
    std::vector<SourceType> sources;

    [[nodiscard]] bool empty() const noexcept
    {
        return sources.empty();
    }
    [[nodiscard]] std::size_t size() const noexcept
    {
        return sources.size();
    }
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

struct HandoffPolicy // the ADR-0036 classification table, as data;
                     // expresses ALL four handoff types (review §12)
{
    OperationClass opClass;
    Handoff required { Handoff::H2_Review }; // which handoff the class requires
    bool rootPrincipalOnly;                  // governance-mutation class
};

struct PolicyTable
{
    std::vector<HandoffPolicy> handoff;
    std::vector<RecoveryRule> recovery;
};

// --- v4 control-plane policy as cognition data (cognitive-boundary-model §8/§12) ---

enum class ClaimClass // what kind of claim rides on the action (seed §12:
{                     // invocation policy is claim-scoped, not thinker-scoped)
    LocalRecall,      // low-risk; native memory may suffice, failure escalates
    CanonicalFact,    // canonical cognition is the authority; recall mandatory
    LiveFact,         // the live source is the authority; verify mandatory
    EvidenceInterpretation,
    Judgment, // no lookup can mechanically produce it
};

enum class ActionKind // externally meaningful proposed transitions (seed §9);
{                     // NOT thought traces - the interior stays opaque (§23)
    BeginTask,
    EnterDomain,

    CreateCppSymbol,    // naming policy must be active (A2/P-51 family)
    IntroducePrimitive, // lower-layer search first (A8)
    ModifyArchitecture,
    ModifyPublicAPI,

    InvokeTool,   // declared contract drives argv (A6)
    RetryFailure, // fingerprint + evidence before retry (A7)

    MakeCanonicalClaim, // canonical source mandatory (P-38 family)
    MakeLiveClaim,      // live source mandatory

    ModifyReferencedContract, // dependent sweep first (A3/A4/A9; inventory §4)

    Commit,          // attribution format check (A1/P-51)
    Publish,         // full-gate receipt + H2 (A5/P-53)
    AcceptCandidate, // review evidence content-bound (P-05 family)
};

enum class RequirementKind // what Cognitive Control demands before the
{                          // action may proceed (seed §27)
    MandatoryRecall,       // resolve named cognition into the packet
    SearchLowerLayer,      // eligible lower layers before authoring
    VerifyCanonical,       // the accepted record, from the tree
    VerifyLive,            // from the live source, never memory
    InspectToolContract,   // declared argv schema, never guessed
    InspectKnownPit,       // fingerprint -> related pits before retry
    RunMechanicalCheck,    // a compiled guard (gate/lint/sweep)
    RequestReview,         // typed handoff evidence (H2 class)
    AskHuman,              // H1/H3/H4 class boundaries
};

struct InvocationRule // one policy row: WHEN this action crosses the control
{                     // plane, THESE requirements are mandatory (seed §8)
    ActionKind action { ActionKind::BeginTask };
    RequirementKind requirement { RequirementKind::MandatoryRecall };
    std::string subject; // retrieval key / check name, e.g. "naming policy"
    bool blocking { true };
};

struct InvocationPolicy // cognition data (PolicyTable-adjacent): the
{                       // activation half of the authority table; derives
                        // from ACCEPTED judgments - default_invocation_policy()
                        // is the compiled scar backlog (P-51/52/53 as data)
    std::vector<InvocationRule> rules;
};

struct RoleSpec // canonical role as governance DATA (R4; DR-017): the registry
{               // is PolicyTable-adjacent cognition; roles stay participant-
                // independent by construction — authorities, duties and the
                // floor reference classes, never instances. An LLM may
                // propose one; only the owner ratifies (self-certification
                // ban class), which is what non-empty provenance records.
    Role id { Role::Worker };
    std::vector<OperationClass> authorities; // operation classes it may perform
    std::vector<std::string> duties;         // disclosure, review, worker discipline...
    CapabilityRequirements floor;            // any binding exercising it satisfies this
    Provenance provenance;                   // ratification evidence (owner + H2/ADR)
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
    EpistemicType epistemic { EpistemicType::Observation }; // const. #8: these never collapse
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
    std::string triggerValue;                    // the concrete trigger: a date, a
                                                 // revision, an event name (review §12)
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
    WorkflowLayer layer { WorkflowLayer::EnvironmentErgonomics }; // DR-018:
                                                                  // dominant layer for workflow profiles; the default
                                                                  // OVERCLAIMS dependence (fail-safe for I-PM4: an
                                                                  // undeclared layer is treated as instance-shaped)
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

struct Snapshot // PURE VALUE TREE — zero pointers, ever (R1); assignable,
{               // copyable; immutable once minted (DR-001)
    Governance governance;
    Constitution constitution;
    PolicyTable policy;
    InvocationPolicy invocation; // v4 control-plane policy as cognition data
    StateView state;
    std::vector<Decision> decisions;
    std::vector<MemoryRecord> memory;
    std::vector<Obligation> obligations;
    std::vector<EvidenceRecord> evidence;
    std::vector<Conflict> conflicts;
    std::vector<ProfileRecord> profiles; // view profileRefs resolve against these
    std::vector<ViewSpec> views;
    std::vector<RoleSpec> roles; // canonical role registry (DR-017): governance
                                 // data; widened only by ratified transaction
};

// --- role registry accessors (fail-closed: an unregistered role has NO
// authorities — the registry is authority, so absence refuses everything) ----

[[nodiscard]] inline const RoleSpec* find_role_spec(const Snapshot& snapshot, Role role)
{
    for (const auto& spec : snapshot.roles)
    {
        if (spec.id == role)
        {
            return &spec;
        }
    }
    return nullptr;
}

[[nodiscard]] inline bool role_spec_valid(const RoleSpec& spec)
{
    // ratification provenance is REQUIRED: an unprovenanced role spec is a
    // proposal, not a registry entry (constitution #9; DR-017 propose/ratify)
    return !spec.provenance.empty() && !spec.duties.empty();
}

// The compiled scar backlog as POLICY DATA: every row cites the pit whose
// acceptance it encodes (judgment -> mechanism traceability, seed §4). These
// are the v4.1/v4.2 guards and the seed's structural boundaries, expressed
// as invocation rules the control plane enforces at action boundaries.
[[nodiscard]] inline InvocationPolicy default_invocation_policy()
{
    return {
        {
            { ActionKind::Commit, RequirementKind::RunMechanicalCheck,
              "attribution subject-position lint (P-51)", true },
            { ActionKind::Publish, RequirementKind::RunMechanicalCheck,
              "full default gate PASS receipt at exact head (P-53)", true },
            { ActionKind::Publish, RequirementKind::RequestReview,
              "H2 exact-delta review", true },
            { ActionKind::CreateCppSymbol, RequirementKind::MandatoryRecall,
              "naming policy", true },
            { ActionKind::IntroducePrimitive, RequirementKind::SearchLowerLayer,
              "eligible lower layers", true },
            { ActionKind::InvokeTool, RequirementKind::InspectToolContract,
              "declared tool argv contract", true },
            { ActionKind::RetryFailure, RequirementKind::InspectKnownPit,
              "failure fingerprint related pits", true },
            { ActionKind::MakeCanonicalClaim, RequirementKind::VerifyCanonical,
              "canonical record", true },
            { ActionKind::MakeLiveClaim, RequirementKind::VerifyLive,
              "live source", true },
            { ActionKind::ModifyReferencedContract, RequirementKind::MandatoryRecall,
              "reference integrity sweep (P-52)", true },
            { ActionKind::AcceptCandidate, RequirementKind::RequestReview,
              "content-bound H2 evidence", true },
        }
    };
}

// The canonical three, as DATA (mirrors collaboration/operating-contract.md):
// owner governs; brother authors/reviews; worker executes locally under
// worker discipline. Authority sets here are candidate design input — the
// provenance names the ratification source, and amendments are governance
// transactions, never silent edits.
[[nodiscard]] inline std::vector<RoleSpec> default_role_specs()
{
    const Provenance ratified { std::vector<SourceType> { SourceType {
        SourceType::Kind::RepositoryFile, "collaboration/operating-contract.md", "" } } };
    return {
        { Role::Owner,
          { OperationClass::DecisionAcceptance, OperationClass::MemoryWrite,
            OperationClass::ObligationWrite, OperationClass::StateUpdate,
            OperationClass::ConflictWrite, OperationClass::EvidenceWrite },
          { "ratify and supersede governance", "accept checkpoints", "delegate authority" },
          { false, CapabilityClass::None, true },
          ratified },
        { Role::Brother,
          { OperationClass::DecisionAcceptance, OperationClass::MemoryWrite,
            OperationClass::ObligationWrite, OperationClass::StateUpdate,
            OperationClass::ConflictWrite, OperationClass::EvidenceWrite },
          { "architecture and review", "disclosure duty (ADR-0035 rule 4)",
            "escalate on doubt" },
          { true, CapabilityClass::None, false },
          ratified },
        { Role::Worker,
          { OperationClass::StateUpdate, OperationClass::EvidenceWrite },
          { "worker discipline: no push, PR or merge without per-task owner authorization" },
          { false, CapabilityClass::LocalSupervised, false },
          ratified },
    };
}
} // namespace qiven::context
