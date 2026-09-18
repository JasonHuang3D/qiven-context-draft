#pragma once

// ============================================================================
// cognition.hpp — the LLMCognition value tree (PART 1 of the architecture)
//
// R1: LLMCognition is a PURE VALUE TREE. Zero pointers, zero references to
//     runtime participants. This is what makes it snapshot-able, restorable
//     and compressible (K4/K5), and it is the drift detector: a repo surface
//     that cannot be pointed at as a member below is not context.
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace qiven::context
{
using PrincipalId = std::string;      // "github:JasonHuang3D" — account-level auth
using ContentId   = std::string;      // SHA-256 of canonical bytes (draft: FNV-1a hex).
                                      // A git commit SHA is ONE minter of ContentId
                                      // (transport); a K4 artifact digest is another.
                                      // Cognition never parses it — compares and records.
using Epoch       = std::uint64_t;    // runtime fencing token (authority layer)
using Yaml        = std::string;      // draft: raw yaml text; kernel will type these

enum class Lifecycle                  // decision lifecycle (record-lifecycle contract)
{
    Proposed,
    Accepted,
    Superseded,
    Rejected,
};

struct Provenance
{                                      // every canonical record carries it (const. #9)
    std::vector<std::string> sources;  // user_statement / git_commit / audit / ...
                                       // transport SHAs are historical facts: live
                                       // verifiable (Phase B), never required to
                                       // reconstruct semantics (Phase A)
};

struct Governance                      // authority RULES live here; identities are
{                                      // session parameters verified at runtime (R4)
    PrincipalId rootPrincipal;         // "github:JasonHuang3D"
    // role -> authority table and account-level auth semantics: prose contracts
    // in qiven-context; future ADR formalizes the structured form
};

struct Constitution                    // meta-rules about cognition itself (const. #4 #5 #13...)
{
    std::vector<std::string> articles; // 18 article titles; full text stays canonical
};

// --- records ---------------------------------------------------------------

struct Decision                        // decisions/ADR-####.md
{
    std::int64_t id{};
    Lifecycle status{Lifecycle::Proposed};
    std::string title;
    std::string content;
    std::vector<std::int64_t> supersedes;
    std::vector<std::int64_t> supersededBy;
    Provenance provenance;
};

struct MemoryRecord                    // memory/ — durable facts, lessons, risks
{
    enum class Kind
    {
        Fact,
        Lesson,
        Protocol,
        Risk,
        Invariant,
        NegativeKnowledge,             // rejected alternatives are first-class (const. #7)
    };
    enum class Status
    {
        Active,
        Superseded,
    };
    Kind kind{Kind::Fact};
    Status status{Status::Active};
    std::string title;
    std::string statement;
    Provenance provenance;
};

struct Obligation                      // obligations/ — unfinished cognition (const. #6)
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
    Status status{Status::Open};
    TriggerKind trigger{TriggerKind::Manual}; // non-terminal obligations carry one
    std::int64_t id{};                         // canonical repo uses OBL-... string ids
    std::string statement;
    std::string completionCriteria;
};

struct EvidenceRecord                  // evidence/audits — immutable once written
{
    ContentId boundTo;                 // binds to the exact candidate (content-addressed)
    std::string content;               // PASS/FAIL, identities, digests
};

struct State                           // compact current operational state
{
    Yaml activeWork;                   // objective, checkpoint, candidate, next boundary
    Yaml current;                      // compact narrative
    Yaml repositories;                 // STABLE INVENTORY ONLY — never caches live refs
    Yaml roadmap;                      // ordered plan items
};

enum class Role                        // canonical authority packages (R4)
{
    Owner,
    Brother,
    Worker,
};

enum class Handoff                     // typed human handoffs (ADR-0036)
{
    H1_Execution,                      // owner physically runs the acceptance producer
    H2_Review,                         // owner or DELEGATED reviewer accepts exact delta
    H3_Authority,                      // human succession
    H4_RecoveryPresence,               // cryptographic local presence (ADR-0029)
};

struct CollaborationRule               // v0 TODO "more specific rules" — form starts here
{
    enum class Domain
    {
        GitWorkflow,
        Validation,
        HandoffBoundary,
        ExecutionMode,
        Operator,
        DcrHost,
    };
    Domain domain{Domain::GitWorkflow};
    std::string rule;                  // TODO(structure): prose -> machine-checkable constraint
};

struct LLMCognition                    // PURE VALUE TREE — zero pointers, ever (R1)
{
    const Epoch epoch;                 // runtime fencing token; immutable per instance
    ContentId contentId;               // content identity of the current state; advances on
                                       // every accepted write — the DURABLE fencing token
                                       // (same concept as epoch, persistence layer)
    Governance governance;
    Constitution constitution;
    std::vector<CollaborationRule> collaborations;
    State state;
    std::vector<Decision> decisions;
    std::vector<MemoryRecord> memory;
    std::vector<Obligation> obligations;
    std::vector<EvidenceRecord> evidence;
    // nothing else. devices/preferences/bindings CANNOT appear here (R1/R3).
};
} // namespace qiven::context
