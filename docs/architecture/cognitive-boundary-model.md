# Cognitive Boundary Model — Judgment, Control, and Mechanism

**Status: candidate design input for v4.**
**Scope:** cognition activation, procedural recall, epistemic invocation, and the semantic boundary between judgment and mechanism.

v3 established what durable cognition is, how it survives participants, how authority fences mutation, how snapshots are materialized, and how accepted state advances through gated transactions.

v4 addresses a different failure class:

> **The required cognition already exists, but it is not brought back into reasoning when the action that requires it occurs.**

This is not primarily a persistence failure.

It is an **activation failure**.

The practical symptoms are already visible:

* a naming convention is canonical, but a new session violates it;
* a lower-layer implementation already exists, but a participant reimplements it;
* an exact tool invocation is documented, but the participant guesses the command line and repeatedly retries;
* a pit was recorded days ago, but a fresh session repeats it;
* a rule is present in context, but the reasoning that needed the rule proceeds as if the rule did not exist.

The v4 problem is therefore not:

> How do we store more memory?

It is:

> **How do we make the cognition required by an action reliably present at the moment that action is reasoned about or executed?**

A concise statement of the v4 objective is:

> **Qiven v4 does not model thought. It makes forgetting at an action boundary an engineering failure instead of an incidental model behavior.**

---

## 1. Substrate is not the boundary

Human, LLM, C++, Python, GPU kernels, databases, and tools are execution substrates.

They do not determine the epistemic role of the computation they perform.

A human following a checklist may be executing a completely mechanical procedure.

An LLM classifying records into a fixed schema may be acting as a mechanism.

A human choosing between architectural trade-offs may be making a judgment.

An LLM choosing between architectural trade-offs may also be making a judgment.

Therefore these axes must remain orthogonal:

```text
execution substrate
    Human
    LLM
    deterministic code
    stochastic code
    external service
    tool

epistemic role
    Judgment
    Cognitive Control
    Mechanism
```

The question:

> "Who or what executes this?"

must never be used as a substitute for:

> "What epistemic responsibility does this computation carry?"

This preserves participant independence.

A future model, a small local model, a frontier API model, and a human can all participate in the same architecture without changing the ontology.

---

## 2. Judgment and Mechanism are separated by specification closure

The useful distinction is not:

```text
thinking vs code
```

and not:

```text
neural vs symbolic
```

The distinction is whether acceptable behavior is **specification-closed**.

### 2.1 Mechanism

A mechanism operates under a contract sufficiently complete that acceptable behavior can be mechanically determined.

Its implementation may be deterministic or stochastic.

It may use:

* ordinary code;
* search;
* concurrency;
* randomized algorithms;
* an ML classifier;
* an LLM constrained to a schema;
* an external service.

Determinism is not the defining property.

The defining property is:

> **Before execution, the system can specify what counts as an acceptable result, what is forbidden, and how failure is detected or surfaced.**

Examples:

```text
CAS succeeds only when base == current revision
```

```text
a DecisionAcceptance transaction requires verified H2 evidence
```

```text
a tool invocation must conform to a declared argv schema
```

```text
a classifier output must belong to one of six declared categories
```

```text
a lower-layer primitive must be searched before a new equivalent primitive is introduced
```

A mechanism can be wrong.

When it is wrong, the appropriate response is normally:

```text
fix the contract or implementation
add or strengthen a mechanical check
retain the regression
```

That failure class is a **bug**.

### 2.2 Judgment

A judgment exists where the available specification does not uniquely determine the acceptable answer.

Examples:

```text
Which architecture better serves the long-term objective?
```

```text
Does this evidence justify accepting the candidate?
```

```text
Is this failure local, or does it expose a design defect?
```

```text
Which competing requirement should dominate here?
```

```text
Does the current evidence reveal a new invariant?
```

Judgment operates in an underspecified or open world.

Its output cannot be proven correct merely by showing that a construction was followed.

It must instead be:

```text
explained
reviewed
supported by evidence
attributed
accepted or rejected
and, when later found wrong, superseded rather than silently rewritten
```

That failure class is a **judgment error**, not a mechanism bug.

---

## 3. The specification-closure test

The former "same input → same output" and "enumerate every behavior" tests are too strong.

Many valid engineering mechanisms contain nondeterminism.

v4 instead uses the following test:

> **Can acceptable behavior be specified before execution precisely enough that conformance can be mechanically checked?**

If yes, the responsibility belongs to Mechanism.

If no, and a participant must still interpret meaning, choose values, compare uncertain alternatives, or decide whether evidence is sufficient, the responsibility belongs to Judgment.

This test applies regardless of substrate.

For example:

```text
LLM produces arbitrary architecture recommendation
    → Judgment

LLM extracts one of a fixed set of schema values
and invalid values are rejected
    → Mechanism

Human chooses an architectural direction
    → Judgment

Human follows an exact release checklist
    → Mechanism
```

---

## 4. Accepted judgment can compile into mechanism

Judgment and Mechanism are not competing worlds.

They form a lifecycle.

A judgment may begin as:

```text
"We repeatedly duplicate primitives already implemented in qiven-foundation."
```

After investigation and acceptance it may become:

```text
"Before introducing a reusable primitive, search eligible lower layers."
```

That accepted judgment can then compile into:

```text
ActionIntent::IntroducePrimitive
    →
mandatory LowerLayerSearch
```

A previous judgment has become mechanism.

This is the deeper meaning of:

> **scars compile**

A pit should not merely become a memory record.

Where mechanically expressible, it should eventually become:

```text
judgment
    ↓
accepted invariant
    ↓
trigger
    ↓
mechanism
    ↓
regression
```

The forward provenance must remain visible.

A mechanism without a traceable accepted judgment behind it is either:

* infrastructure whose contract is independently obvious;
* accidental complexity;
* an unowned policy.

v4 should prefer mechanisms whose semantic reason is traceable.

---

## 5. Why Judgment and Mechanism are still insufficient

The recurring failures observed during development reveal a third responsibility.

A participant may possess the required cognition yet fail to use it.

Examples:

```text
The naming convention exists,
but the model does not recall it while creating an identifier.
```

```text
Foundation reuse is an accepted rule,
but the model begins implementing before inspecting Foundation.
```

```text
The exact qiven CLI contract exists,
but the model guesses command syntax.
```

```text
A previous failure has already been recorded,
but retry reasoning does not retrieve it.
```

The missing question is:

> **What cognitive operation should happen next before judgment or execution continues?**

That responsibility belongs to the:

# Cognitive Control Plane

---

## 6. Cognitive Control

Cognitive Control does not answer the domain problem.

It determines what must happen before the domain problem may continue.

Typical control questions are:

```text
Do I have enough evidence?

Is this claim canonical or live?

Must I retrieve before relying on memory?

Must I inspect a lower layer before implementation?

Must I verify the actual runtime environment?

Must I ask a human?

Must I obtain review?

Has this failure happened before?

May I retry, or must I stop and gather evidence?

Is this action safe to execute directly?

Does this transition require a new context bundle?
```

Cognitive Control therefore sits between a participant's proposed action and the mechanisms that can satisfy its epistemic requirements.

The architecture becomes:

```text
                    ┌──────────────────────┐
                    │       Judgment       │
                    │     Human / LLM      │
                    │                      │
                    │ interpret            │
                    │ hypothesize          │
                    │ compare              │
                    │ decide               │
                    └──────────┬───────────┘
                               │
                         ActionIntent
                               │
                               ▼
                    ┌──────────────────────┐
                    │  Cognitive Control   │
                    │                      │
                    │ recall requirements  │
                    │ verification         │
                    │ retrieval triggers   │
                    │ risk policy          │
                    │ escalation           │
                    │ retry discipline     │
                    └──────┬────────┬──────┘
                           │        │
                  epistemic│        │pragmatic
                     action│        │action
                           ▼        ▼
                    ┌──────────────────────┐
                    │      Mechanism       │
                    │                      │
                    │ store / retrieve     │
                    │ search / tools       │
                    │ compiler / tests     │
                    │ git / filesystem     │
                    │ CI / CAS / network   │
                    └──────────┬───────────┘
                               │
                      typed observations
                      evidence / verdicts
                               │
                               ▼
                    ┌──────────────────────┐
                    │   Cognition State    │
                    │                      │
                    │ decisions            │
                    │ memory               │
                    │ obligations          │
                    │ conflicts            │
                    │ evidence             │
                    │ policy               │
                    └──────────────────────┘
```

Cognitive Control is itself primarily engineering.

It may permit discretionary judgment, but mandatory invocation rules are mechanisms.

---

## 7. Cognition persistence and cognition activation are different properties

v3 primarily established **persistence**:

```text
accepted cognition survives:
    session loss
    participant replacement
    runtime rebirth
    serialization
    artifact transport
```

v4 establishes **activation**:

```text
the cognition required by Action X
is present when Action X is deliberated or executed
```

These properties must not be conflated.

A perfect durable store can still produce a bad agent if nothing causes relevant cognition to be recalled.

Likewise, retrieving every record into every prompt is not a solution.

The desired property is:

> **Relevant cognition is activated at semantic boundaries where omission would create a known failure class.**

This is procedural recall.

---

## 8. Declarative rules are insufficient without invocation triggers

Consider the canonical rule:

```text
Prefer existing lower-layer implementations over duplication.
```

As declarative cognition, this only means:

> the project knows the rule.

It does not imply:

> the rule will be active while a participant introduces a reusable primitive.

v4 therefore distinguishes:

```text
Knowledge
```

from:

```text
Invocation Trigger
```

A stronger representation is:

```text
WHEN ActionIntent == IntroduceReusablePrimitive
THEN LowerLayerSearch is mandatory before authoring implementation
```

Similarly:

```text
WHEN ActionIntent == CreateCppSymbol
THEN active naming convention + local neighboring examples are mandatory inputs
```

```text
WHEN ActionIntent == InvokeTool
THEN exact tool contract is mandatory;
guessing invocation syntax is forbidden when a contract is available
```

```text
WHEN a previously unseen failure occurs
THEN capture the failure fingerprint and retrieve related pits before retry
```

```text
WHEN the same failure signature occurs again
THEN blind retry is forbidden
```

This transforms memory from passive prose into activated procedure.

---

## 9. ActionIntent is the control-plane boundary

The thinker should not need to know which storage engine, search backend, tool registry, or index satisfies a cognitive requirement.

The thinker expresses what it is about to do.

Conceptually:

```cpp
enum class ActionKind
{
    BeginTask,
    EnterDomain,

    CreateCppSymbol,
    IntroducePrimitive,
    ModifyArchitecture,
    ModifyPublicAPI,

    InvokeTool,
    RetryFailure,

    MakeCanonicalClaim,
    MakeLiveClaim,

    Commit,
    Publish,
    AcceptCandidate
};

struct ActionIntent
{
    ActionKind kind;

    std::vector<std::string> files;
    std::vector<std::string> concepts;

    std::string tool;
    std::string operation;

    std::string failureFingerprint;
};
```

`ActionIntent` is not a thought trace.

It is an externally meaningful proposed transition.

The architecture deliberately does not record:

```text
every reasoning step
hidden chain-of-thought
internal attention
token-by-token planning
```

It records only the boundary required to govern the next externally meaningful act.

---

## 10. CognitiveNeed describes epistemic requirements, not tool calls

The thinker should not normally say:

```text
"Run SQLite query X."
```

or:

```text
"Call vector database Y."
```

It should express the epistemic need:

```text
"I need the currently accepted rule governing H2."
```

```text
"I need to know whether a lower layer already provides hashing."
```

```text
"I need the exact current CLI contract for qiven gate invocation."
```

```text
"I need to verify the live main revision."
```

Conceptually:

```cpp
enum class CognitiveNeedKind
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

struct CognitiveNeed
{
    CognitiveNeedKind kind;

    std::string subject;
    std::string scope;

    bool mandatory { false };
};
```

Cognitive Control maps needs onto mechanisms.

Today that may mean:

```text
Snapshot lookup
filesystem search
Git query
SQLite FTS
symbol index
compiler
test runner
web/API
human handoff
```

Tomorrow the backend may change.

The thinker contract should not.

---

## 11. Epistemic actions are first-class

Not every action directly changes the project.

Some actions exist to improve the state of knowledge before another action is selected.

Examples:

```text
retrieve a decision
inspect a neighboring implementation
search Foundation
run a compiler
run a test
verify HEAD
inspect a file
query a live service
ask a human
request review
```

These are **epistemic actions**.

They change what the thinker can justifiably know.

This distinction matters because many current agent failures result from prematurely taking a pragmatic action:

```text
write code
retry command
publish
accept
```

before performing the epistemic action required to justify it.

v4 therefore treats:

```text
need evidence
```

as a real state requiring resolution, not as an informal suggestion inside a prompt.

---

## 12. Invocation policy is action- and claim-scoped, not primarily thinker-scoped

Human and LLM memory have different failure profiles.

That difference is real but should not become the primary ontology.

A stronger policy is based first on the epistemic class of the claim.

Examples:

### Canonical facts

```text
"What does the currently accepted ADR require?"
```

Before a consequential action, canonical cognition must be consulted.

Human recollection and model recollection are both secondary.

### Live facts

```text
"What is main HEAD now?"
"Is this service reachable?"
"Which model is actually serving?"
```

These must be verified from the live source.

Neither human nor LLM memory is authoritative.

### Low-risk local recall

```text
"What was that helper approximately called?"
```

Native recall may be sufficient initially.

Failure can escalate into retrieval.

### Judgment

```text
"Should this architecture be split?"
```

Retrieval may supply evidence, but no canonical lookup can mechanically produce the judgment.

Therefore invocation policy should primarily derive from:

```text
ActionKind
ClaimClass
StalenessRisk
Impact
Verifiability
Canonicality
Participant capability
```

rather than:

```text
human → trust
LLM → distrust
```

Native-memory reliability remains useful participant metadata, but it does not define truth.

---

## 13. Structural invocation beats confidence-based invocation

LLM confidence is not a reliable signal that project state is current.

A stale continuation and a correct continuation can be equally fluent.

Therefore important invocation rules must not depend on:

```text
"I feel uncertain."
```

Mandatory retrieval is instead triggered structurally.

Typical structural boundaries include:

```text
cold boot

new task

task-domain transition

material architecture change

introduction of a reusable primitive

creation of public identifiers or public API

tool invocation

first failure

repeated failure

canonical claim

live-state claim

material transaction

commit

publication

acceptance
```

The exact set is policy data.

The architectural principle is:

> **High-value recall is triggered by semantics of the action, not by subjective confidence.**

---

## 14. Boundary-triggered recall, not retrieval at every reasoning step

v4 does not require retrieval before every thought.

That would create unnecessary latency, noise, and cost.

The target is:

```text
boundary-triggered activation
```

rather than:

```text
continuous retrieval
```

A reasoning process may internally explore freely.

Before a governed action crosses a semantic boundary, Cognitive Control evaluates the applicable invocation policy.

This keeps the opaque interior of reasoning free while making important transitions reliable.

---

## 15. Coding conventions are an activation problem

A naming convention stored in memory is insufficient.

For example:

```text
ActionIntent:
    CreateCppSymbol
```

should trigger:

```text
mandatory:
    repository naming policy
    language-specific convention
    neighboring canonical examples
```

The model is no longer expected to choose among:

```text
Google style
LLVM style
Microsoft style
Qt style
Unreal style
training-distribution preference
Qiven style
```

from latent memory.

The required project convention is explicitly activated.

The desired property becomes:

> **When a project identifier is created, the applicable naming convention is present in the preparation packet.**

That property can be tested.

---

## 16. Lower-layer reuse is an activation problem

The rule:

```text
prefer lower-layer implementation
```

must compile into an invocation trigger.

Before introducing a reusable primitive:

```text
hashing
serialization primitive
byte cursor
endian operation
string conversion
filesystem helper
process helper
math primitive
platform abstraction
```

Cognitive Control requires a lower-layer search.

Conceptually:

```text
ActionIntent::IntroducePrimitive
        ↓
SearchEligibleLowerLayers
        ↓
ExistingImplementationReport
        ↓
Judgment
```

Only after the report is available may the participant decide:

```text
reuse
extend
wrap
or intentionally create a new primitive
```

If a new primitive is still introduced, the decision should be explainable.

The aim is not to prohibit new implementation.

The aim is to prohibit **uninformed duplication**.

---

## 17. Tool invocation should be mechanism-owned

Exact command syntax is not a judgment problem.

If the system already has a declared tool contract, an LLM should not reconstruct that contract from memory.

For example, the thinker says:

```text
Intent:
    run the local gate
```

The mechanism layer should resolve:

```text
tool
operation
argv schema
working directory
environment requirements
timeout semantics
expected outputs
```

and execute the typed invocation.

The LLM should not repeatedly guess variants such as:

```text
python ...
cmd ...
qiven ...
--name ...
positional argument ...
```

when the exact contract is available.

The rule is:

> **When a machine-readable tool contract exists, invocation syntax belongs to Mechanism, not Judgment.**

This converts a large class of low-value reasoning failures into ordinary contract validation.

---

## 18. Failure is an invocation trigger

Failure should change the epistemic state.

A failed action creates new evidence.

The next action must therefore not be selected from the same unchanged reasoning state.

The minimum control rule is:

```text
first material failure
    ↓
capture FailureFingerprint
    ↓
retrieve:
    exact error
    applicable tool contract
    related pits
    relevant previous failures
    current environment facts
    ↓
re-judge
```

A repeated identical failure strengthens the rule:

```text
same failure signature repeated
    ↓
blind retry prohibited
```

The next action must add evidence.

Examples of evidence-producing actions:

```text
inspect documentation
inspect source
verify path
verify runtime version
query current tool schema
search prior incident
reduce the problem
run diagnostic command
ask human
```

A retry with no changed evidence is not a reasoning strategy.

---

## 19. FailureFingerprint

v4 should type recurring failures sufficiently to drive recall.

A conceptual structure:

```cpp
struct FailureFingerprint
{
    std::string operation;
    std::string tool;
    std::string category;

    std::string stableMessage;
    int exitCode {};

    std::vector<std::string> affectedFiles;
};
```

The fingerprint must avoid treating incidental text such as timestamps or temporary paths as identity where possible.

Its role is not perfect incident deduplication.

Its role is:

```text
failure
    →
known-pit lookup
    →
retry discipline
```

A pit that cannot be activated from a recognizable future situation is only partially compiled.

---

## 20. PreparationPacket

The output of Cognitive Control should be explicit.

Conceptually:

```cpp
struct PreparationPacket
{
    ActionIntent intent;

    std::vector<CanonicalRef> mandatoryContext;
    std::vector<RuleRef> constraints;

    std::vector<SearchResult> existingImplementations;
    std::vector<EvidenceRef> relevantEvidence;
    std::vector<PitRef> knownPits;

    std::vector<LiveFact> verifiedFacts;

    std::vector<CognitiveNeed> unresolvedNeeds;
};
```

The packet is the bridge back into Judgment.

The thinker receives:

```text
the proposed action
+
the cognition that action requires
+
verified live facts
+
known hazards
+
remaining uncertainty
```

and may then deliberate.

This is stronger than generic RAG because the content is not selected solely by semantic similarity.

It is selected partly by **policy obligations attached to the action**.

---

## 21. Policy-directed retrieval is not generic RAG

Generic RAG usually resembles:

```text
user query
    ↓
semantic search
    ↓
top-k
    ↓
LLM
```

v4 requires:

```text
ActionIntent
    ↓
InvocationPolicy
    ↓
mandatory deterministic retrieval
    +
optional lexical / semantic retrieval
    ↓
PreparationPacket
    ↓
Judgment
```

For:

```text
IntroducePrimitive(hash)
```

semantic similarity alone is insufficient.

Policy already knows that an eligible lower-layer search is mandatory.

The preferred retrieval hierarchy is therefore:

```text
1. exact / structural resolution
2. symbol and dependency lookup
3. lexical / FTS retrieval
4. semantic retrieval
5. model-assisted relevance judgment
```

Higher-cost semantic machinery should be used where lower-cost deterministic machinery is insufficient.

Qiven must define the retrieval contract.

It does not need to own the best retrieval algorithm.

---

## 22. Qiven is not a model-memory research project

v4 must explicitly preserve the boundary between external cognition infrastructure and model research.

Qiven may own:

```text
canonical cognition
decisions
memory records
negative knowledge
obligations
conflicts
evidence
authority
invocation policy
retrieval contracts
context bundles
continuity
handoffs
state lifecycle
failure fingerprints
cognitive preparation
```

Qiven core does not own:

```text
foundation-model weights
custom transformer architectures
custom attention mechanisms
persistent neural hidden state as canonical truth
KV cache as project cognition
learned latent memory as canonical truth
custom chain-of-thought language
ThoughtStep DSL
training a domain embedding model
training a reranker
training a neural memory controller
training a reasoning model
```

External models and retrieval components remain replaceable participants or mechanisms.

The architectural test is:

> **If GPT is replaced by Claude, GLM, Qwen, a local model, or a human, does this Qiven property still make sense?**

If yes, it likely belongs in Qiven.

If the property exists only by modifying a particular model's weights or hidden state, it belongs to model research rather than Qiven core.

---

## 23. The hidden interior of reasoning remains opaque

v4 explicitly rejects modeling the inside of reasoning.

No attempt should be made to persist or type:

```text
every internal thought
private chain-of-thought
token-level deliberation
attention traces
a universal reasoning DSL
```

The architecture controls boundaries:

```text
inputs to judgment
requested epistemic actions
proposed externally meaningful actions
evidence returned
accepted outputs
```

The inside remains opaque.

This is sufficient.

A thinking system can be governed without reconstructing its private cognitive implementation.

---

## 24. Judgment may request retrieval; policy may require retrieval

There are two valid paths into an epistemic action.

### Discretionary request

The thinker recognizes uncertainty:

```text
"I need more evidence."
```

and emits a `CognitiveNeed`.

### Mandatory invocation

Policy determines that an action cannot proceed without retrieval, regardless of the thinker's confidence.

Example:

```text
Publish
    →
verify current base
run required gate
resolve mandatory acceptance evidence
```

Mandatory invocation dominates discretionary confidence.

The model cannot waive it by saying:

```text
"I already remember."
```

This is the cognitive analogue of the existing no-verbal-waiver principle.

---

## 25. Accepted cognition and native memory have different authority

Human recollection, LLM context salience, model weights, and session state are useful.

They are not canonical project truth.

Native memory may guide exploration.

Canonical cognition governs claims whose class requires canonical authority.

Live sources govern live facts.

The architecture therefore distinguishes at least:

```text
native recall
canonical cognition
live observation
derived evidence
judgment
```

A participant may remember correctly.

That does not make memory the authority source.

---

## 26. V4 core invariants

The following are candidate normative v4 invariants.

### V4-R1 — No consequential action depends on latent recall alone

If omission of a known rule can invalidate a consequential action, the applicable cognition must be activated through policy before the action proceeds.

### V4-R2 — A known pit requires an invocation path

Recording a pit is not sufficient.

Where mechanically expressible, the pit must identify the future action or failure condition that activates its recall or guard.

```text
pit
→ trigger
→ preparation
→ guard/test
```

### V4-R3 — Blind retry is not recovery

A material failure must produce new evidence before an equivalent retry.

Repeated identical failure without new evidence is prohibited.

### V4-R4 — Tool syntax is mechanism-owned when a contract exists

Participants express tool intent.

Declared tool contracts generate or validate the concrete invocation.

### V4-R5 — Lower-layer reuse is checked before reusable primitive creation

Introducing a reusable primitive requires a search of eligible lower layers before implementation is accepted.

### V4-R6 — Canonical and live claims use their authoritative sources

Canonical claims activate canonical cognition.

Live claims activate live verification.

Native memory is not substituted for either where policy requires authority.

### V4-R7 — Retrieval is boundary-triggered, not continuous

Reasoning interiors remain opaque and unrestricted.

Cognitive Control runs at semantically meaningful transitions.

### V4-R8 — Cognitive Control does not become a hidden thinker

Control decides which epistemic obligations must be satisfied.

It does not silently make domain judgments that belong to Human or LLM participants.

### V4-R9 — Mechanism may be stochastic

A mechanism does not need deterministic output.

It must instead have a mechanically enforceable acceptance contract.

### V4-R10 — Model independence remains structural

No v4 semantic may require persistence of model-specific hidden state in canonical cognition.

---

## 27. Candidate v4 types

The following types illustrate semantic boundaries only.

They are not yet an implementation commitment.

```cpp
enum class ClaimClass
{
    LocalRecall,
    CanonicalFact,
    LiveFact,
    EvidenceInterpretation,
    Judgment
};

enum class ActionKind
{
    BeginTask,
    EnterDomain,

    CreateCppSymbol,
    IntroducePrimitive,
    ModifyArchitecture,
    ModifyPublicAPI,

    InvokeTool,
    RetryFailure,

    MakeCanonicalClaim,
    MakeLiveClaim,

    Commit,
    Publish,
    AcceptCandidate
};

struct ActionIntent
{
    ActionKind kind;
    ClaimClass claimClass;

    std::vector<std::string> concepts;
    std::vector<std::string> files;

    std::string tool;
    std::string operation;

    std::optional<FailureFingerprint> priorFailure;
};

enum class RequirementKind
{
    MandatoryRecall,
    SearchLowerLayer,
    VerifyCanonical,
    VerifyLive,
    InspectToolContract,
    InspectKnownPit,
    RunMechanicalCheck,
    RequestReview,
    AskHuman
};

struct CognitiveRequirement
{
    RequirementKind kind;
    std::string subject;
    bool blocking { true };
};

struct PreparationPacket
{
    ActionIntent intent;

    std::vector<CognitiveRequirement> requirements;

    std::vector<CanonicalRef> mandatoryContext;
    std::vector<EvidenceRef> evidence;
    std::vector<PitRef> knownPits;
    std::vector<LiveFact> liveFacts;

    std::vector<CognitiveRequirement> unresolved;
};
```

The important part is not the exact names.

The important boundary is:

```text
Judgment proposes intent
        ↓
Control derives requirements
        ↓
Mechanisms satisfy requirements
        ↓
Judgment receives prepared evidence
```

---

## 28. The causal loop

The v4 causal loop is:

```text
1. Observe current cognition / task state.

2. Judgment proposes the next externally meaningful ActionIntent
   or requests an epistemic action.

3. Cognitive Control classifies the intent.

4. Applicable invocation policy produces CognitiveRequirements.

5. Mechanisms satisfy mandatory requirements:
      retrieve
      inspect
      search
      verify
      test
      calculate
      request review
      ask human

6. A PreparationPacket is produced.

7. If blocking requirements remain unresolved:
      the original action may not proceed.

8. Judgment deliberates using the prepared cognition/evidence.

9. Mechanism executes the accepted action.

10. Result / evidence / failure is typed.

11. Material cognition advances through the existing gated transaction path.

12. Any failure may create:
      evidence
      a new pit
      a new trigger
      or a new accepted judgment that later compiles into mechanism.
```

This loop extends v3.

It does not replace the v3 authority, persistence, or transaction semantics.

---

## 29. Example — naming convention

Without v4:

```text
LLM begins writing
    ↓
latent training distribution chooses plausible naming style
    ↓
review discovers Qiven convention violation
```

With v4:

```text
ActionIntent:
    CreateCppSymbol

Cognitive Control:
    naming policy required

Mechanism:
    resolve Qiven C++ naming policy
    inspect neighboring accepted implementation

PreparationPacket:
    naming constraints + examples

Judgment:
    choose semantically appropriate name within constraints

Mechanism:
    code + compiler / format checks
```

The LLM does not need perfect long-term procedural memory.

---

## 30. Example — lower-layer duplication

Without v4:

```text
task requires hashing
    ↓
LLM knows how to implement FNV
    ↓
implements it
    ↓
human later notices Foundation already owns it
```

With v4:

```text
ActionIntent:
    IntroducePrimitive(hash)

Control:
    SearchLowerLayer is blocking

Mechanism:
    inspect dependency graph
    search qiven-foundation symbols

Evidence:
    qiven::fnv1a64 exists

Judgment:
    reuse Foundation implementation

Mechanism:
    integrate and validate
```

The failure class disappears without making the thinker smarter.

---

## 31. Example — command-line failure

Without v4:

```text
guess command
↓
fail
↓
guess variant
↓
fail
↓
guess variant
```

With v4:

```text
ActionIntent:
    InvokeTool(qiven, gate/local)

Control:
    exact tool contract mandatory

Mechanism:
    resolve argv schema
    construct invocation

if invocation fails:
    capture FailureFingerprint
    retrieve exact error + known tool incidents
    blind retry prohibited

Judgment:
    decide next diagnostic step from new evidence
```

The reasoning budget is spent on the abnormal case, not on reconstructing known syntax.

---

## 32. Example — canonical architectural rule

Question:

```text
"May this merge-class change proceed without H2?"
```

This is not answered from native recall.

The claim class is:

```text
CanonicalFact
```

Therefore:

```text
VerifyCanonical
```

is mandatory.

The accepted policy is activated from cognition.

Only then does the participant reason about the concrete operation.

---

## 33. Human and LLM remain replaceable thinkers

Humans and LLMs differ in native memory, salience, confidence calibration, and procedural habit formation.

Those differences matter for capability and risk policy.

They do not change the architecture.

Both produce judgments.

Both can forget.

Both can rely on stale facts.

Both can benefit from external cognition.

Both are subject to mandatory canonical/live verification where required.

The architecture should therefore become stronger as thinker quality improves, not become obsolete.

A better future model may request fewer unnecessary lookups and make better judgments.

It should not be allowed to bypass required epistemic boundaries merely because it is more capable.

---

## 34. Progress is retained cognition plus reliable activation

Persistence alone does not create progress.

Activation alone does not create progress.

Project progress requires both:

```text
accepted cognition persists
        +
required cognition activates
        +
new judgment is reviewed
        +
accepted judgment advances cognition
```

The full loop is:

```text
experience
    ↓
judgment
    ↓
acceptance
    ↓
canonical cognition
    ↓
trigger / mechanism
    ↓
future activation
    ↓
better next judgment
```

This is how a project stops repeatedly paying for the same mistake.

---

## 35. Non-goals

v4 does not attempt to:

* reproduce human memory architecture;
* make an LLM permanently remember every project fact;
* store every conversation token;
* create an infinite prompt;
* model private chain-of-thought;
* define a universal reasoning DSL;
* train a foundation model;
* train a custom reasoning model;
* make vector search the canonical truth source;
* make embeddings or rerankers part of project semantics;
* eliminate judgment;
* mechanically prove architecture quality;
* convert every engineering decision into an if/else;
* treat model confidence as evidence;
* make Cognitive Control a replacement thinker.

The target is narrower:

> **Make known cognitive requirements reliably active at the boundaries where they matter.**

---

## 36. Retrieval implementation boundary

Qiven owns:

```text
when retrieval is required
what epistemic requirement must be satisfied
what evidence class is acceptable
how retrieved material is typed
how omissions are surfaced
how failures affect control flow
```

Qiven does not need to own:

```text
the best embedding model
the best reranker
the best vector database
the best semantic search model
```

Those are replaceable mechanism implementations.

A simple implementation may begin with:

```text
exact references
Git / filesystem lookup
symbol search
dependency graph
SQLite FTS
known-pit lookup
failure fingerprint index
```

Semantic retrieval can remain a fallback capability.

The ontology must not depend on it.

---

## 37. Relationship to v3

v3 established:

```text
Snapshot
Materialization
Revision identity
authority grants
transactions
verdicts
conflicts
views
bundles
session sidecars
runtime rebirth
artifact continuity
```

v4 does not invalidate these abstractions.

It adds the missing causal layer between:

```text
cognition exists
```

and:

```text
participant uses cognition correctly
```

The conceptual progression is:

```text
v3:
What is cognition?
How does it survive?
Who may mutate it?

v4:
When must cognition become active?
What evidence must exist before the next action?
How do known lessons alter future reasoning before the mistake repeats?
```

---

## 38. Relationship to ContextBundle

`ContextBundle` remains the presentation object.

Cognitive Control determines part of its required content.

In v3:

```text
Query
    →
Bundle
```

v4 moves toward:

```text
ActionIntent
    →
InvocationPolicy
    →
CognitiveRequirements
    →
retrieval / verification
    →
PreparationPacket
    →
ContextBundle / equivalent judgment input
```

This makes the reason for inclusion explicit.

A mandatory rule appears because an action required it, not merely because retrieval ranked it highly.

---

## 39. Relationship to the pit ledger

The pit ledger currently proves that incidents are remembered.

v4 extends its meaning.

A mature pit should answer:

```text
What failed?

What accepted lesson resulted?

What future condition indicates
that the lesson is relevant again?

What mechanism activates or enforces it?

What regression proves that activation?
```

Thus a future pit record may conceptually trace:

```text
P-XX
    ↓
Accepted judgment
    ↓
InvocationTrigger
    ↓
CognitiveRequirement
    ↓
Gate / preparation / tool contract
    ↓
Regression
```

"Scars compile" therefore gains a second dimension:

```text
v3:
scar → gate

v4:
scar → recall trigger → gate / evidence
```

A remembered scar that is never activated can still be re-lived.

---

## 40. Relationship to authority

Cognitive Control does not grant authority.

A `PreparationPacket`, retrieval result, or successful verification cannot itself authorize mutation.

The v3 authority model remains independent:

```text
knowing
≠
being authorized
```

Likewise:

```text
retrieval success
≠
acceptance

test PASS
≠
review

evidence
≠
judgment
```

Epistemic and authority planes remain orthogonal.

---

## 41. Proposed v4 proof obligations

Before v4 semantics are considered mature, the executable specification should be able to demonstrate at least the following scenarios.

### Known naming rule is activated

A fresh runtime generation attempts to create C++ symbols.

The applicable naming convention is present in the preparation input without relying on the participant's prior session memory.

### Lower-layer duplication is intercepted

A participant proposes a reusable primitive already present in Foundation.

A lower-layer search is mandatory before implementation proceeds, and the existing implementation is surfaced.

### Tool syntax is not guessed

A participant requests a known tool operation.

The concrete invocation is constructed from the tool contract.

### First failure changes the reasoning state

A failed operation produces a typed failure fingerprint and evidence lookup before another attempt.

### Repeated blind retry is refused

The same failure signature cannot simply execute the same action indefinitely without new evidence.

### Canonical claim retrieves canonical truth

A consequential canonical claim cannot rely solely on participant memory.

### Live claim verifies live state

A live fact such as current revision or serving model is resolved through its live source.

### Participant replacement preserves invocation semantics

Human A, Model A, Model B, and future participants receive the same mandatory cognitive requirements for the same action class.

### Retrieval backend replacement preserves semantics

Replacing FTS, semantic retrieval, or another backend does not change the invocation policy or canonical cognition model.

---

## 42. Proposed v4 landing discipline

v4 should not begin by implementing a generalized retrieval engine.

The recommended sequence is semantic:

```text
1. freeze this boundary model;

2. inventory recurring development failures that were
   persistence-success / activation-failure;

3. derive a minimal ActionKind vocabulary from real scars;

4. type InvocationPolicy and CognitiveRequirement;

5. implement deterministic triggers for the highest-value cases:
      tool contract lookup
      lower-layer reuse
      naming convention
      canonical/live fact verification
      failure-triggered recall;

6. make PreparationPacket observable;

7. add executable regressions;

8. only then evaluate whether lexical/semantic retrieval quality
   needs stronger machinery.
```

The first v4 success criterion is not retrieval sophistication.

It is reduction of repeated known mistakes.

---

## 43. Rejected alternatives

### Thinking = Human/LLM, Engineering = code

Rejected.

It classifies substrate rather than epistemic responsibility.

### Engineering means deterministic execution

Rejected.

Many valid mechanisms are stochastic.

Specification closure, not determinism, is the boundary.

### Every reasoning step should retrieve

Rejected.

It creates cost and noise without improving semantic discipline.

Retrieval is boundary-triggered.

### Trust stronger models to remember

Rejected as an architectural guarantee.

Stronger thinkers reduce error probability but do not remove session boundaries, salience competition, stale state, tool drift, or participant replacement.

### Put the whole repository into every prompt

Rejected.

Persistence completeness and activation relevance are different goals.

### Generic RAG is sufficient

Rejected.

Semantic similarity does not encode mandatory procedural obligations such as lower-layer search or live verification.

### Model the inside of reasoning

Rejected.

The interior is intentionally opaque.

Inputs, action intents, epistemic requests, outputs, evidence, and acceptance boundaries are sufficient.

### Solve activation by training a project-specific model

Rejected from Qiven core.

The architecture must remain useful when the thinker changes.

---

## 44. The v4 design thesis

v3 made project cognition durable and governable.

v4 must make it **causally available**.

The central failure to eliminate is:

```text
the project already knows,
but the current reasoning proceeds as if it does not.
```

The solution is not to demand perfect memory from the thinker.

It is to move the responsibility into an engineering control plane:

```text
meaningful action
    →
required cognition
    →
evidence
    →
judgment
    →
mechanism
```

The intended end state is:

> **A participant is free to think, but it is not free to cross a consequential boundary while silently missing cognition that the project already knows is required there.**

That is the semantic purpose of Qiven v4.
