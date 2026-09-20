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
#include <optional>
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

struct SessionDesignation                     // owner-granted, session-scoped process widening
{                                             // (DR-017); RUNTIME SIDECAR, never cognition. The
                                              // authority-delta-∅ invariant (I-PM3) is enforced BY
                                              // ABSENCE: the type has no field that could express
                                              // an authority — designations bypass PROCESS only.
    std::string id;                           // "jason-extended-cognition"
    Role base { Role::Brother };              // which canonical role it adapts
    std::string grantRef;                     // owner-grant evidence pointer; a
                                              // designation without a grant is
                                              // NEVER valid (never self-assigned)
    std::vector<std::string> processBypasses; // e.g. brother/worker partition
};

[[nodiscard]] inline bool designation_valid(const SessionDesignation& d)
{
    return !d.id.empty() && !d.grantRef.empty();
}

// --- v4 cognitive control plane (runtime exchange types; seed §9/§10/§19/§20) --
// These are CONTROL-PLANE objects, not cognition: like ParticipantBinding
// they never enter the tree. The packet is built from the snapshot ONLY -
// a fresh runtime generation restoring the same bytes derives the same
// preparation (activation independent of participant memory, §41).

struct FailureFingerprint // seed §19: identity of a failure for pit recall;
{                         // incidental text (timestamps, temp paths) excluded
    std::string operation;
    std::string tool;
    std::string category;
    std::string stableMessage;
    int exitCode {};
    std::vector<std::string> affectedFiles;
};

struct ActionIntent // seed §9: an externally meaningful PROPOSED transition;
{                   // not a thought trace - the interior stays opaque (§23)
    ActionKind kind { ActionKind::BeginTask };
    ClaimClass claimClass { ClaimClass::LocalRecall };
    std::vector<std::string> concepts;
    std::vector<std::string> files;
    std::string tool;
    std::string operation;
    std::optional<FailureFingerprint> priorFailure; // RetryFailure carries one
};

enum class CognitiveNeedKind // seed §10: epistemic needs, not tool calls
{
    Recall,
    VerifyCanonicalFact,
    VerifyLiveFact,
    SearchExistingImplementation,
    SearchEvidence,
    Calculate,
    Simulate,
    RequestReview,
    AskHuman
};

struct CognitiveNeed // the DISCRETIONARY path (seed §24): judgment recognizes
{                    // uncertainty and requests; policy may REQUIRE regardless
    CognitiveNeedKind kind { CognitiveNeedKind::Recall };
    std::string subject;
    std::string scope;
    bool mandatory { false };
};

struct CognitiveRequirement // a derived, action-scoped requirement
{
    RequirementKind kind { RequirementKind::MandatoryRecall };
    std::string subject;
    bool blocking { true };
};

enum class PreparationFailure // S0-02: absence must be a TYPED failure, never
{                             // an implicit "no requirements" fail-open state
    None,
    InvocationPolicyMissing, // World A: policy absent/unavailable -> fail closed
    RequiredRecallMissing,   // a blocking recall failed to resolve
};

struct PreparationPacket // seed §20: the bridge back into Judgment. Content
{                        // is selected by POLICY OBLIGATIONS attached to the
                         // action, not by similarity alone (§21).
    ActionIntent intent;
    std::vector<CognitiveRequirement> requirements;          // all derived demands
    std::vector<std::string> mandatoryContext;               // resolved cognition
    std::vector<std::string> knownPits;                      // resolved pit records
    std::vector<std::string> liveFacts;                      // filled by live ports
    std::vector<CognitiveRequirement> unresolved;            // blocking recall failures
    PreparationFailure failure { PreparationFailure::None }; // S0-02

    [[nodiscard]] bool ready() const
    {
        return failure == PreparationFailure::None && unresolved.empty();
    }
};

// Derive the control-plane demands for an intent from the snapshot's
// invocation policy. Pure: same snapshot + intent => same requirements.
[[nodiscard]] inline std::vector<CognitiveRequirement> derive_requirements(
    const Snapshot& snapshot, const ActionIntent& intent)
{
    std::vector<CognitiveRequirement> out;
    for (const auto& rule : snapshot.invocation.rules)
    {
        if (rule.action == intent.kind)
        {
            out.push_back(CognitiveRequirement { rule.requirement, rule.subject,
                                                 rule.blocking });
        }
    }
    return out;
}

// Build the preparation packet: recall-class requirements resolve against
// the snapshot (memory titles / profile ids as retrieval keys, v1); action-
// class requirements (verify-live, run-check, ask-human...) are listed as
// demands whose satisfaction is execution-time, outside this pure builder.
// A blocking recall that finds nothing lands in `unresolved` - the action
// may not proceed (seed §28 step 7).
[[nodiscard]] inline PreparationPacket build_preparation_packet(
    const Snapshot& snapshot, const ActionIntent& intent)
{
    PreparationPacket packet;
    packet.intent = intent;
    if (!snapshot.invocation.present)
    {
        // S0-02: an absent policy is NOT the statement "no requirements
        // exist". For governed v4 actions, control fails closed here.
        packet.failure = PreparationFailure::InvocationPolicyMissing;
        return packet;
    }
    packet.requirements = derive_requirements(snapshot, intent);
    for (const auto& requirement : packet.requirements)
    {
        const bool recallClass = requirement.kind == RequirementKind::MandatoryRecall ||
                                 requirement.kind == RequirementKind::VerifyCanonical ||
                                 requirement.kind == RequirementKind::InspectKnownPit;
        if (!recallClass)
        {
            continue; // an action-class demand: listed, satisfied at execution
        }
        bool resolved = false;
        for (const auto& record : snapshot.memory)
        {
            if (record.title.find(requirement.subject) != std::string::npos ||
                record.statement.find(requirement.subject) != std::string::npos)
            {
                packet.mandatoryContext.push_back(record.title + ": " + record.statement);
                resolved = true;
            }
        }
        for (const auto& profile : snapshot.profiles)
        {
            if (profile.id.find(requirement.subject) != std::string::npos)
            {
                packet.mandatoryContext.push_back(profile.id + ": " + profile.summary);
                resolved = true;
            }
        }
        if (!resolved && requirement.blocking)
        {
            packet.unresolved.push_back(requirement);
        }
    }
    return packet;
}

// --- v4.4 / A6: tool invocation is mechanism-owned when a contract exists
// (seed §17): participants express intent; the declared argv contract
// constructs and validates the concrete invocation. Guessing syntax where
// a contract exists is the A6 failure class (the CLI-retry odyssey).

struct ToolContract // the declared, machine-readable argv contract
{
    std::string tool;                      // "qiven"
    std::string operation;                 // "gate"
    std::vector<std::string> argvTemplate; // fixed words and {placeholders}:
                                           // e.g. {"{python}", "tools/qiven.py",
                                           //  "{operation}", "{subject}"}
    std::vector<std::string> allowedFlags; // "--json", "--verbose", ...
};

[[nodiscard]] inline bool is_placeholder(const std::string& word)
{
    return word.size() >= 2 && word.front() == '{' && word.back() == '}';
}

// Construct the invocation from the contract, never from memory: template
// fixed words are kept verbatim; {operation}/{tool}/{subject} substitute
// from the intent; trailing allowed flags pass through in order.
[[nodiscard]] inline std::vector<std::string> construct_invocation(
    const ToolContract& contract, const ActionIntent& intent)
{
    std::vector<std::string> argv;
    for (const auto& word : contract.argvTemplate)
    {
        if (word == "{operation}")
        {
            argv.push_back(intent.operation.empty() ? contract.operation : intent.operation);
        }
        else if (word == "{tool}")
        {
            argv.push_back(intent.tool.empty() ? contract.tool : intent.tool);
        }
        else if (word == "{subject}")
        {
            argv.push_back(intent.concepts.empty() ? std::string {} : intent.concepts.front());
        }
        else
        {
            argv.push_back(word);
        }
    }
    return argv;
}

// Validate an argv AGAINST the contract: same arity, fixed positions
// verbatim, placeholder positions non-empty. A guessed variant (the A6
// failure class) differs in a fixed position or arity and fails here.
[[nodiscard]] inline bool validate_invocation(const ToolContract& contract,
                                              const std::vector<std::string>& argv)
{
    if (argv.size() != contract.argvTemplate.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < argv.size(); ++i)
    {
        const std::string& word = contract.argvTemplate[i];
        if (is_placeholder(word))
        {
            if (argv[i].empty())
            {
                return false;
            }
        }
        else if (argv[i] != word)
        {
            return false;
        }
    }
    return true;
}

// --- v4.5 / A8: lower-layer reuse is checked before primitive creation
// (seed §16): the search itself is an EXECUTION-TIME mechanism (filesystem,
// dependency graph, symbol index live outside cognition); the control
// plane owns the boundary - the search is mandatory, the report must
// exist before judgment, and uninformed duplication is refused.

struct LowerLayerHit // one existing implementation the search surfaced
{
    std::string repo;    // "qiven-foundation"
    std::string symbol;  // "qiven::fnv1a64"
    std::string summary; // what it provides
};

struct ExistingImplementationReport // the evidence judgment receives
{
    bool searched { false };         // the mechanism actually ran
    std::vector<LowerLayerHit> hits; // what already exists
};

// Authorization to proceed to JUDGMENT on a reusable primitive: the search
// must have run. Hits do NOT decide reuse - they inform it (the aim is to
// prohibit uninformed duplication, not new implementation: seed §16).
[[nodiscard]] inline bool primitive_judgment_authorized(const ActionIntent& intent,
                                                        const ExistingImplementationReport& report)
{
    if (intent.kind != ActionKind::IntroducePrimitive)
    {
        return true; // not this boundary
    }
    return report.searched;
}

// --- v4.4 / A7: failure is an invocation trigger (seed §18/§19) ----------
// A material failure creates evidence; the next action must not be an
// equivalent retry with nothing new (V4-R3: blind retry is not recovery).

// Normalize a failure message for fingerprint stability: drop volatile
// tokens (long digit runs = timestamps/ids; temp-path fragments). The
// fingerprint identity must survive incidental text, not encode it.
[[nodiscard]] inline std::string normalize_failure_text(const std::string& message)
{
    std::string out;
    std::string word;
    const auto flush = [&]() {
        if (word.empty())
        {
            return;
        }
        bool volatileToken = false;
        std::size_t digits = 0;
        for (const char c : word)
        {
            if (c >= '0' && c <= '9')
            {
                ++digits;
            }
        }
        if (digits >= 4)
        {
            volatileToken = true; // timestamp / large id
        }
        if (word.find("tmp") != std::string::npos ||
            word.find(".generated-temp") != std::string::npos)
        {
            volatileToken = true; // temp path fragment
        }
        if (!volatileToken)
        {
            if (!out.empty())
            {
                out += ' ';
            }
            out += word;
        }
        word.clear();
    };
    for (const char c : message)
    {
        if (c == ' ' || c == '\t' || c == '\n')
        {
            flush();
        }
        else
        {
            word += c;
        }
    }
    flush();
    return out;
}

// Pit recall from a fingerprint: memory records (risk/lesson class) whose
// text matches the fingerprint's tool or category — the known-pit lookup
// that must precede any retry (seed §18).
[[nodiscard]] inline std::vector<std::string> find_related_records(
    const Snapshot& snapshot, const FailureFingerprint& fingerprint)
{
    std::vector<std::string> out;
    for (const auto& record : snapshot.memory)
    {
        if (record.kind != MemoryRecord::Kind::Risk &&
            record.kind != MemoryRecord::Kind::Lesson)
        {
            continue;
        }
        const bool matches = record.title.find(fingerprint.tool) != std::string::npos ||
                             record.statement.find(fingerprint.tool) != std::string::npos ||
                             record.title.find(fingerprint.category) != std::string::npos;
        if (matches)
        {
            out.push_back(record.title + ": " + record.statement);
        }
    }
    return out;
}

// The retry rule (V4-R3): an equivalent retry — same tool+operation, same
// normalized failure signature — is permitted ONLY with new evidence.
[[nodiscard]] inline bool retry_permitted(const FailureFingerprint& prior,
                                          const ActionIntent& candidate,
                                          bool evidenceAdded)
{
    if (evidenceAdded)
    {
        return true;
    }
    const bool sameOperation = candidate.tool == prior.tool &&
                               candidate.operation == prior.operation;
    const bool sameSignature =
        normalize_failure_text(candidate.priorFailure.has_value()
                                   ? candidate.priorFailure->stableMessage
                                   : prior.stableMessage) ==
        normalize_failure_text(prior.stableMessage);
    return !(sameOperation && sameSignature); // identical + no evidence: refused
}

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
    CognitionHandle pCognition; // lifetime pinned per work cycle;
                                // const: cognition is not mutable
                                // through participants (DR-001)
    // draft hooks — production replaces with real reasoning + tool runtime
    std::function<Query(const std::string&)> queryBuilder;
    std::function<ContextTransaction(const std::string&, const Snapshot&)> deltaGenerator;

    SessionCheckpoint checkpoint;                 // sidecar this session owns (v9-style)
    std::vector<ContextTransaction> parkedDeltas; // refused-but-held deltas
                                                  // (StaleBase recovery only)

    // one causal work cycle — the numbered order is normative:
    // acquire grant -> read -> think -> propose -> gated write ->
    // recovery-table next action (DR-002: the model never chooses its own
    // recovery; the policy table in cognition does)
    void work(const std::string& prompt, const AuthenticatedActor& actor, std::string& result,
              WorkMode mode = WorkMode::SupervisedForeground);

private:
    // the causal body; work() wraps this and guarantees annotate_budget on
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

    bool control_llm_from_human(const AuthenticatedActor& actor, const std::string& prompt,
                                std::string& result)
    {
        pCurrentLLM->work(prompt, actor, result);
        wait_for_llm();                  // async: dispatch != completion (v5 lesson)
        return collect_feedback(result); // relay itself is fallible (UI-send incident)
    }

    static void wait_for_llm()
    {
    } // draft: synchronous stand-in
    bool collect_feedback(const std::string& result)
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

    bool use_llm_to_work(const std::shared_ptr<LLMClientTool>& pClient, const std::string& prompt,
                         std::string& result) const
    {
        // the actor's identity comes from the human's verified principal; the
        // role/binding/serving disclosure come from the client-tool binding
        // disclosure consistency (review §10): the serving model names the
        // ACTUALLY bound LLM, so a runtime rebind cannot drift the disclosure
        AuthenticatedActor actor { verifiedPrincipal, pClient->binding.role,
                                   pClient->binding.modelId, pClient->pCurrentLLM->name,
                                   "standard" };
        return pClient->control_llm_from_human(actor, prompt, result);
    }
};
} // namespace qiven::context
