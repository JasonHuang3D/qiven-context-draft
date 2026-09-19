#pragma once

// ============================================================================
// runtime.hpp — PART 3/4: participants (pointer graph) and the session sidecar
//
// Participant changes (model / tool / device / human) are pointer rebinds —
// O(1), never a cognition write (R3). The LLM is blind to Device/Human/Client
// by construction (R5): its only side effects are result + gated delta write
// + tool calls.
// ============================================================================

#include <qiven/context/persistence.hpp>

#include <functional>
#include <string>
#include <vector>

namespace qiven::context
{
struct Device // [J] pure data; verify live, never cached in cognition
{
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

struct ParticipantBinding       // runtime; CANNOT live in cognition (R1/R3)
{                               // qualification evidence enters cognition via
    Role role { Role::Worker }; // normal transactions (audits); the current
    std::string modelId;        // binding itself never does. caller-declared
                                // serving model = disclosure duty (ADR-0035)
};

struct SessionCheckpoint   // PART 4: session sidecar — continuity evidence
{                          // only, never canonical (const. #5); NOT a
    std::string sessionId; // cognition member. includes serving-model
    std::string exactTask; // identity (disclosure duty, ADR-0035 rule 4)
    std::vector<ContentId> acceptedRefs;
    std::vector<ContentId> unacceptedCandidates;
    std::string knownGaps; // gaps recorded, never synthesized
    std::string nextAction;
};

struct HumanPreference // owner-side adaptation; session-injected;
{                      // never alters acceptance topology (R4)
    std::string key;
    std::string value; // relay style, merge-confirmation path, ...
};

struct LLM
{
    std::string name;                         // value: intrinsic identity ("GPT5")
    std::shared_ptr<LLMCognition> pCognition; // lifetime pinned per Work cycle
                                              // (shared_ptr); authority fenced (epoch)
    // draft hooks — production replaces with real reasoning + tool runtime
    std::function<Query(const std::string&)> queryBuilder;
    std::function<TransactionDelta(const std::string&, const LLMCognition&)> deltaGenerator;

    SessionCheckpoint checkpoint;               // sidecar this session owns (v8-style)
    std::vector<TransactionDelta> parkedDeltas; // rejected-but-held deltas (never lost)

    // one causal Work cycle — the numbered order is normative:
    void Work(const std::string& prompt, std::string& result,
              WorkMode mode = WorkMode::SupervisedForeground);
};

struct LLMClientTool // the relay; NO cognition access (R5)
{
    std::string name;                     // "chatGPT" / "zcode-desktop" / ...
    std::shared_ptr<LLM> pCurrentLLM;     // rebindable (model switch)
    std::shared_ptr<Device> pTargeDevice; // rebindable (device migration)
    ParticipantBinding binding;           // current runtime binding (rebindable)
    bool operational { true };

    bool ControlLLMFromHuman(const std::string& prompt)
    {
        std::string result;
        pCurrentLLM->Work(prompt, result);
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
                                   // governance rootPrincipal (R4)

    bool UseLLMToWork(const std::shared_ptr<LLMClientTool>& pClient,
                      const HumanPreference& pref, const std::string& prompt) const
    {
        static_cast<void>(pref); // draft: preferences adapt presentation only
        return pClient->ControlLLMFromHuman(prompt);
    }
};
} // namespace qiven::context
