#pragma once

// ============================================================================
// runtime.hpp — PART 3/4: participants (pointer graph) and the session sidecar
//
// Participant changes (model / tool / device / human) are pointer rebinds —
// O(1), never a cognition write (R3). The LLM is blind to Device/Human/Client
// by construction (R5): its only side effects are result + gated transaction
// + tool calls. Participants hold shared_ptr<const Materialization>: they can
// read and propose, never mutate (DR-001).
// ============================================================================

#include <qiven/context/persistence.hpp>

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace qiven::context
{
struct Device // [J] pure data; verify live, never cached in cognition
{             // pit.profiles_family_only: families only, exact values live
    std::string name;
    std::string os;
    std::string env; // "git,gh,MSVC,etc" — families, verify-live
    bool reachable { true };
};

struct RemoteDevice final : Device     // [S] github.com: a SECOND device participating
{                                      // in the loop — a cloud replica of the store
    bool ciEnabled { true };           // that validates appended states; the evidence
    std::string role { "ci+replica" }; // flows back through ordinary gated writes
};

struct Qualification // ADR-0035: evidence-based binding qualification; the
{                    // upgrade to Qualified is owner-reserved (self-cert ban)
    enum class Status
    {
        Untested,
        Provisional,
        Qualified,
    };
    Status status { Status::Untested };
    std::string evidenceRef; // audits / PR records that prove the status
    std::string date;
};

struct ParticipantBinding        // runtime; CANNOT live in cognition (R1/R3)
{                                // qualification evidence enters cognition via
    Role role { Role::Worker };  // normal transactions (audits); the current
    std::string modelId;         // binding itself never does. caller-declared
    Qualification qualification; // serving model = disclosure duty (ADR-0035)
};

struct EvidenceGap          // typed absence: gaps are RECORDED, never synthesized
{                           // (constitution #5; the v2-v5 missing sessions lesson)
    std::string what;       // what is missing or unresolved
    std::string whyUnknown; // why it cannot be stated now
};

enum class CheckpointTrigger // DR-012: the loop writes the checkpoint, and
{                            // every write names WHY (P-16: lagging
    TurnBoundary,            // checkpoints were the drift class)
    MaterialTransaction,
    AsyncExit,
    SessionClose,
};

struct TurnBudget                           // the observed ~26-minute Chat tool-turn boundary is the
{                                           // budget that makes checkpoint discipline non-optional
    std::chrono::seconds soft { 26 * 60 };  // finish the safe checkpoint, stop
    std::chrono::seconds hard { 2 * soft }; // halt outright
};

struct BoundedWait // tools: dispatch != completion (v5 lesson); bounded
{                  // observations, no sleep/poll loops
    int maxObservations { 3 };
    std::chrono::milliseconds perObservation { 60000 };
};

struct SessionCheckpoint   // PART 4: session sidecar — continuity evidence
{                          // only, never canonical (const. #5); NOT a
    std::string sessionId; // Snapshot member. includes serving-model
    std::string exactTask; // identity (disclosure duty, ADR-0035 rule 4)
    std::vector<RevisionId> acceptedRefs;
    std::vector<RevisionId> unacceptedCandidates;
    std::vector<EvidenceGap> evidenceGaps; // typed absence, never synthesized
    std::string knownGaps;                 // gaps recorded, never synthesized
    std::string nextAction;
    CheckpointTrigger lastTrigger { CheckpointTrigger::TurnBoundary };
    std::string servingDisclosure; // who actually served the turns
}; // pit.undisclosed_substitution_flagged: this field is mandatory per turn

struct HumanPreference // owner-side adaptation; session-injected;
{                      // never alters acceptance topology (R4)
    std::string key;
    std::string value; // relay style, merge-confirmation path, ...
};

struct LLM
{
    std::string name;           // value: intrinsic identity ("GPT5")
    TurnBudget turnBudget;      // session economics (DR-012)
    CognitionHandle pCognition; // lifetime pinned per Work cycle;
                                // const: cognition is not mutable
                                // through participants (DR-001)
    // draft hooks — production replaces with real reasoning + tool runtime
    std::function<Query(const std::string&)> queryBuilder;
    std::function<ContextTransaction(const std::string&, const Snapshot&)> deltaGenerator;

    SessionCheckpoint checkpoint;                 // sidecar this session owns (v9-style)
    std::vector<ContextTransaction> parkedDeltas; // refused-but-held deltas
                                                  // (StaleBase recovery only)

    // one causal Work cycle — the numbered order is normative:
    // acquire grant -> read -> think -> propose -> gated write ->
    // recovery-table next action (DR-002: the model never chooses its own
    // recovery; the policy table in cognition does)
    void Work(const std::string& prompt, const AuthenticatedActor& actor, std::string& result,
              WorkMode mode = WorkMode::SupervisedForeground);

private:
    // the causal body; Work() wraps this and guarantees annotate_budget on
    // every completed turn (DR-012: the budget invariant covers ALL paths)
    void work_impl(const std::string& prompt, const AuthenticatedActor& actor,
                   std::string& result, WorkMode mode,
                   std::chrono::steady_clock::time_point turnStart);

public:
    // truthful budget annotation on the checkpoint (observable state only —
    // no invented percentages; MEM-8F2C41)
    void annotate_budget(std::chrono::steady_clock::time_point turnStart);
};

struct LLMClientTool // the relay; NO cognition access (R5)
{
    std::string name;                      // "chatGPT" / "zcode-desktop" / ...
    std::shared_ptr<LLM> pCurrentLLM;      // rebindable (model switch)
    std::shared_ptr<Device> pTargetDevice; // rebindable (device migration)
    ParticipantBinding binding;            // current runtime binding (rebindable)
    bool operational { true };

    bool ControlLLMFromHuman(const AuthenticatedActor& actor, const std::string& prompt,
                             std::string& result)
    {
        pCurrentLLM->Work(prompt, actor, result);
        WaitForLLM();                   // async: dispatch != completion (v5 lesson)
        return CollectFeedBack(result); // relay itself is fallible (UI-send incident)
    }

    static void WaitForLLM()
    {
    } // draft: synchronous stand-in
    bool CollectFeedBack(const std::string& result)
    {
        lastFeedback = result; // human consumes RESULTS, never raw context
        return true;           // draft: relay succeeds; production surfaces
    } // transport failures here (UI-send incident)
    std::string lastFeedback;
};

struct Human // all-value, zero pointers — the most
{            // replaceable element (H3 = instance swap)
    std::string name;
    std::vector<HumanPreference> preferences;
    std::string verifiedPrincipal; // session-injected identity, verified against
                                   // governance rootPrincipal at the port (R4)

    bool UseLLMToWork(const std::shared_ptr<LLMClientTool>& pClient, const std::string& prompt,
                      std::string& result) const
    {
        // the actor's identity comes from the human's verified principal; the
        // role/binding/serving disclosure come from the client-tool binding
        // disclosure consistency (review §10): the serving model names the
        // ACTUALLY bound LLM, so a runtime rebind cannot drift the disclosure
        AuthenticatedActor actor { verifiedPrincipal, pClient->binding.role,
                                   pClient->binding.modelId, pClient->pCurrentLLM->name,
                                   "standard" };
        return pClient->ControlLLMFromHuman(actor, prompt, result);
    }
};
} // namespace qiven::context
