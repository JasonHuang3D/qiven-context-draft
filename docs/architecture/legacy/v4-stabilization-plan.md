# v4 Stabilization Remediation Plan — Pre-ADL Semantic Closure

**Status:** remediation plan for the accepted v4 executable specification
**Purpose:** stabilize v4 before Runtime ADL begins
**Scope:** `qiven-context-draft` v4 semantics plus the minimal cross-repository canonical reconciliation required to make those semantics truthful
**Not a v5 roadmap. Not a production-runtime design.**

---

## 1. Audit baseline

This remediation plan is bound to the following remote heads reviewed before drafting:

### qiven-context-draft

```text
repository:
    JasonHuang3D/qiven-context-draft

main:
    b15e41f3a5233b06129a78d394705cca97b62c7b

subject:
    merge(draft): v4.6 complete
```

Important reviewed surfaces:

```text
include/qiven/context/cognition.hpp
include/qiven/context/runtime.hpp
src/persistence.cpp
src/runtime.cpp
apps/cognition_loop.cpp

tests/control_plane.cpp
tests/tool_and_retry.cpp

docs/architecture/cognitive-boundary-model.md
docs/architecture/v4-roadmap.md
docs/architecture/v4-validation-report.md

README.md
```

### canonical qiven-context

```text
repository:
    JasonHuang3D/qiven-context

main:
    6cd93db90ef4cd83d63655c98729c1a45014c8b2

subject:
    merge(context): v4 closeout state
```

Important reviewed surfaces:

```text
evidence/audits/context-v4-activation-trial-2026-09-20.md

obligations/
    OBL-20260918T215000Z-B4D6A8.md

collaboration/
state/
memory/
```

---

# 2. Current judgment

v4 is accepted as an architecture.

That judgment does **not** need to be reopened.

The following central semantics are sound and remain frozen unless this remediation exposes a direct contradiction:

```text
Judgment
    ≠
Mechanism

substrate
    ≠
epistemic responsibility

Cognitive Control
    =
the plane deciding what cognition/evidence/action
must be present before a meaningful boundary is crossed

ActionIntent
    =
externally meaningful proposed action,
not private thought trace

InvocationPolicy
    =
canonical cognition data

PreparationPacket
    =
boundary object returning required cognition/evidence to Judgment

mandatory invocation
    >
participant confidence

retrieval
    =
boundary-triggered rather than continuous

known scar
    →
trigger
    →
future activation

hidden reasoning interior
    =
opaque

authority
    ⟂
epistemic preparation
```

The v4.6 activation trial provides meaningful evidence that carried cognition can causally alter a fresh consumer's behavior.

The remaining problems are therefore not reasons to redesign v4.

They are **semantic-closure defects in the executable reference specification**.

The objective is:

> Make the executable v4 say exactly what the architecture means, no more and no less, before production ADL starts consuming it as a normative base.

---

# 3. Why stabilization is required before ADL

The current draft contains several places where a reference miniature is semantically weaker than the names and comments surrounding it.

That is tolerable while discovering the architecture.

It is no longer tolerable once the draft becomes the normative input to Runtime ADL.

For example:

```text
PreparationPacket::ready()
```

currently means approximately:

```text
"no snapshot-local recall lookup failed"
```

but the natural reading of `ready()` is:

```text
"the governed action is ready to proceed"
```

Those are not equivalent.

Likewise:

```text
ExistingImplementationReport::searched = true
```

is sufficient for a miniature proof of the A8 semantic boundary, but it is not proof that a real lower-layer search occurred.

The ADL must be free to replace these reference mechanisms with production mechanisms without first having to reverse-engineer which parts of the draft were normative semantics and which parts were demonstration scaffolding.

v4 stabilization therefore has one governing rule:

> **Close semantic ambiguity now; defer production mechanism choices to ADL.**

---

# 4. Non-goals

This remediation MUST NOT design or implement:

```text
qiven-contextd
Windows service / daemon
Named Pipe IPC
MCP adapter
ExecutionGateway
PreparedActionPermit
production GitStore
filesystem execution mediation
real symbol index
real dependency graph
real live-source verifier
semantic/vector retrieval
SQLite
production tool registry
production trust capabilities
production sandboxing
authority cutover
files-as-projection migration
```

Those belong to Runtime ADL and detailed design.

This remediation MAY type a missing semantic state if the absence makes v4 itself ambiguous.

It MUST NOT choose the eventual production topology.

---

# 5. Severity model

The findings are classified as:

```text
S0 — semantic contradiction
     executable behavior contradicts accepted v4 meaning

S1 — semantic ambiguity
     reference implementation permits a materially wrong interpretation

S2 — truth/documentation drift
     accepted reality and canonical/documented state disagree

S3 — explicit production boundary
     intentionally incomplete mechanism that must be marked honestly
```

The remediation finishes only when all S0/S1 findings are closed and all S2/S3 findings are truthfully represented.

---

# 6. S0-01 — InvocationPolicy is absent from real genesis

## Current state

`default_invocation_policy()` exists and the v4 tests explicitly install it into their sample snapshots.

However current `makeGenesis()` initializes:

```cpp
governance
constitution
policy
state
```

but does not initialize:

```cpp
invocation
```

Therefore a real empty-store `CanonicalRemote` boot produces cognition whose:

```cpp
Snapshot::invocation.rules
```

is empty.

The v4 control policy exists in code but is not actually present in the real genesis cognition.

This contradicts the v4 statement that `InvocationPolicy` is cognition data and that the compiled scar backlog forms the default activation policy.

## Required remediation

`makeGenesis()` MUST seed:

```cpp
genesis.invocation = default_invocation_policy();
```

The compiled default has one role only:

> bootstrap policy for a genuinely new cognition chain.

It MUST NOT overwrite policy loaded from an existing snapshot.

## Required proofs

Add executable assertions that:

```text
empty store
→ create_cognition(CanonicalRemote)
→ genesis Snapshot
→ invocation.rules is non-empty
→ exact expected default rows exist
```

Also prove:

```text
restore serialized Snapshot with explicit invocation policy
→ loaded policy preserved exactly
→ compiled default does not replace it
```

## Acceptance

A freshly created cognition chain cannot exist as a v4 chain without its activation policy.

---

# 7. S0-02 — Missing InvocationPolicy currently fails open

## Current state

The control-plane test currently contains the concept:

```cpp
Snapshot snapshot; // empty invocation policy
QCD_CHECK(derive_requirements(snapshot, any).empty());
```

with commentary describing absence as fail-closed.

But operationally:

```text
empty invocation policy
    ↓
derive_requirements()
    =
empty requirements

build_preparation_packet()
    ↓
unresolved.empty()
    =
true

PreparationPacket::ready()
    =
true
```

So the actual semantics are fail-open.

This is a direct contradiction.

A missing control policy must never mean:

> No cognitive requirements exist.

Those two worlds are epistemically different.

## Required remediation

Policy absence MUST become an explicit preparation failure.

Do not encode this by silently inventing requirements.

Introduce an explicit preparation-level error/state.

Conceptually:

```cpp
enum class PreparationFailure
{
    None,
    InvocationPolicyMissing,
    RequiredRecallMissing,
};
```

or an equivalent typed representation.

The exact type name is not normative.

The semantic requirement is:

```text
policy exists and contains zero applicable requirements
```

must remain distinguishable from:

```text
policy is absent / unavailable / invalid
```

For any governed v4 action:

```text
InvocationPolicy absent
    →
fail closed
```

## Backward compatibility rule

A pre-v7 artifact or intentionally old snapshot that contains no invocation policy MUST NOT silently receive the current compiled policy during deserialization.

That would rewrite historical cognition.

Instead:

```text
old cognition restores faithfully
+
v4 Cognitive Control reports policy unavailable
```

until an explicit governed cognition migration occurs.

## Required tests

Prove all four cases:

```text
1. genesis
   → policy present
   → preparation works

2. explicit policy with no rule for Action X
   → valid empty requirement set for X

3. missing policy
   → typed fail-closed preparation result

4. old serialized snapshot lacking v4 policy
   → restores faithfully
   → governed v4 action fails closed
   → no silent policy synthesis
```

---

# 8. S0-03 — `PreparationPacket::ready()` conflates derivation with satisfaction

## Current state

Current `PreparationPacket` contains:

```cpp
requirements
mandatoryContext
knownPits
liveFacts
unresolved
```

and:

```cpp
bool ready() const
{
    return unresolved.empty();
}
```

The builder automatically tries to resolve some snapshot-local requirements.

Execution-time requirements are merely listed.

Example:

```text
ActionKind::Publish
```

derives:

```text
RunMechanicalCheck
RequestReview
```

but neither is satisfied by `build_preparation_packet()`.

Nevertheless the current test expects:

```cpp
packet.ready() == true
```

because no snapshot-local recall failed.

As executable architecture this is too ambiguous.

A future runtime consumer can easily interpret `ready()` as:

> ready to publish.

That would violate v4.

## Required remediation

Requirement derivation and requirement satisfaction MUST become explicitly separate concepts.

Add a minimal requirement lifecycle.

Conceptually:

```cpp
enum class RequirementStatus
{
    Pending,
    Satisfied,
    Failed,
};
```

Each prepared requirement must carry its current state.

Also distinguish when the requirement must be satisfied.

Conceptually:

```cpp
enum class RequirementBoundary
{
    BeforeJudgment,
    BeforeExecution,
};
```

Examples:

```text
CreateCppSymbol
    MandatoryRecall(naming policy)
    BeforeJudgment

IntroducePrimitive
    SearchLowerLayer
    BeforeJudgment

RetryFailure
    InspectKnownPit
    BeforeJudgment

MakeCanonicalClaim
    VerifyCanonical
    BeforeJudgment

MakeLiveClaim
    VerifyLive
    BeforeJudgment

InvokeTool
    InspectToolContract
    BeforeExecution

Commit
    RunMechanicalCheck
    BeforeExecution

Publish
    RunMechanicalCheck
    BeforeExecution

Publish
    RequestReview
    BeforeExecution

AcceptCandidate
    RequestReview
    BeforeExecution
```

The exact classification may be adjusted only with an explicit rationale.

## Replace `ready()`

The single ambiguous readiness predicate MUST disappear.

Prefer two explicit questions:

```cpp
ready_for_judgment()
ready_for_execution()
```

Their semantic contract:

```text
ready_for_judgment
=
every blocking BeforeJudgment requirement is Satisfied

ready_for_execution
=
every blocking requirement at all boundaries is Satisfied
```

Therefore:

```text
Publish packet immediately after derivation
    ready_for_judgment()  → may be true
    ready_for_execution() → false
```

until the mechanical check and review requirements have actual satisfaction evidence.

## Required tests

At minimum:

```text
CreateCppSymbol:
    naming recall satisfied
    → ready_for_judgment true

CreateCppSymbol:
    naming recall missing
    → ready_for_judgment false

Publish:
    requirements derived only
    → ready_for_execution false

Publish:
    mechanical check satisfied only
    → ready_for_execution false

Publish:
    review satisfied only
    → ready_for_execution false

Publish:
    both satisfied
    → ready_for_execution true
```

No test may again use "listed requirement" as equivalent to "satisfied requirement".

---

# 9. S0-04 — requirement resolution currently mixes incompatible semantics

## Current state

`build_preparation_packet()` currently treats these as one `recallClass`:

```cpp
MandatoryRecall
VerifyCanonical
InspectKnownPit
```

and then attempts to satisfy all of them by substring-scanning:

```cpp
MemoryRecord.title
MemoryRecord.statement
ProfileRecord.id
```

using:

```cpp
find(requirement.subject)
```

This creates three semantic errors.

### Error A — VerifyCanonical is not generic memory lookup

The existence of a memory record containing the phrase:

```text
canonical record
```

must not satisfy:

```text
VerifyCanonical
```

Canonical verification is a different epistemic requirement.

### Error B — InspectKnownPit ignores FailureFingerprint

The actual A7 fingerprint-based lookup exists separately in:

```cpp
find_related_records(...)
```

but `build_preparation_packet()` does not use it.

Instead the default policy subject:

```text
failure fingerprint related pits
```

is searched literally in memory text.

The two proof paths are disconnected.

### Error C — substring matching creates accidental satisfaction

A record may satisfy a blocking requirement merely because unrelated prose happens to contain the subject string.

That is not a stable activation contract.

## Required remediation

### 9.1 Pure packet building must stop pretending to satisfy non-local requirements

`build_preparation_packet()` may automatically satisfy only requirements whose resolution is truly contained in the snapshot and has an explicit reference rule.

For stabilization:

```text
MandatoryRecall
```

may use deterministic snapshot lookup.

These remain Pending until an explicit resolver result exists:

```text
VerifyCanonical
VerifyLive
SearchLowerLayer
InspectToolContract
InspectKnownPit
RunMechanicalCheck
RequestReview
AskHuman
```

### 9.2 Snapshot recall matching becomes exact

For the v4 reference implementation, replace substring matching with deterministic exact keys.

Minimal acceptable reference semantics:

```text
MemoryRecord.title == requirement.subject

or

ProfileRecord.id == requirement.subject
```

Do not invent semantic ranking in this remediation.

If the architecture later needs richer selectors, ADL may introduce them.

The stabilization rule is:

> A blocking requirement must never be accidentally satisfied by incidental prose similarity.

### 9.3 A7 fingerprint lookup becomes the only reference path for pit recall

`InspectKnownPit` MUST use the actual:

```text
FailureFingerprint
```

carried by `ActionIntent::priorFailure`.

The current `find_related_records()` miniature may remain the reference implementation.

But `PreparationPacket` must reflect its result.

The control flow becomes:

```text
RetryFailure
    ↓
InspectKnownPit Pending
    ↓
fingerprint lookup
    ↓
pit evidence attached
    ↓
InspectKnownPit Satisfied
```

If fingerprint data required by the policy is absent:

```text
blocking requirement
→ Failed
```

or equivalent fail-closed state.

---

# 10. S1-01 — `ClaimClass` is modeled but does not affect policy matching

## Current state

`ActionIntent` contains:

```cpp
ClaimClass claimClass;
```

The v4 architecture explicitly states that invocation policy is:

```text
action- and claim-scoped
```

and should not be primarily thinker-scoped.

However current:

```cpp
derive_requirements()
```

matches only:

```cpp
rule.action == intent.kind
```

`claimClass` is operationally ignored.

This makes the type decorative.

## Required remediation

`InvocationRule` MUST be capable of expressing an optional claim-class predicate.

Conceptually:

```cpp
struct InvocationRule
{
    ActionKind action;

    std::optional<ClaimClass> claimClass;

    RequirementKind requirement;
    RequirementBoundary boundary;

    std::string subject;
    bool blocking;
};
```

Semantics:

```text
claimClass absent
    →
rule applies to every claim class for that action

claimClass present
    →
both action and claim class must match
```

At least one executable test MUST prove that changing only the claim class changes derived requirements.

This prevents the claim axis from becoming ceremonial data.

## Non-goal

Do not attempt to build a full epistemic risk engine here.

That belongs after real runtime evidence exists.

---

# 11. S1-02 — the discretionary `CognitiveNeed` path is currently disconnected

## Current state

v4 defines:

```cpp
CognitiveNeed
```

and correctly distinguishes:

```text
mandatory policy invocation
```

from:

```text
discretionary participant request
```

However the current control-plane builder consumes only:

```text
ActionIntent
```

The discretionary need type does not participate in the packet.

Thus the architecture says:

```text
Judgment may request an epistemic action
```

while the executable miniature has no path for that request.

## Required remediation

Do not force `CognitiveNeed` into `RequirementKind`.

They are conceptually different:

```text
CognitiveRequirement
=
control policy obligation

CognitiveNeed
=
participant-requested epistemic assistance
```

Instead `PreparationPacket` should explicitly preserve discretionary needs.

Conceptually:

```cpp
struct PreparationPacket
{
    ActionIntent intent;

    std::vector<CognitiveRequirement> requirements;
    std::vector<CognitiveNeed> discretionaryNeeds;

    ...
};
```

The builder should accept optional needs:

```cpp
build_preparation_packet(
    snapshot,
    intent,
    needs);
```

Mandatory requirements continue to dominate.

A participant need may request more information.

It can never waive a mandatory requirement.

## Required proof

Demonstrate:

```text
same ActionIntent
+
different discretionary CognitiveNeed
→ mandatory requirements remain identical

and

CognitiveNeed cannot remove or mark a mandatory requirement satisfied
```

---

# 12. S1-03 — proof-like booleans overstate the reference miniature

## Current state

Current v4 contains:

```cpp
ExistingImplementationReport
{
    bool searched;
}
```

and:

```cpp
primitive_judgment_authorized(...)
```

Likewise:

```cpp
retry_permitted(
    ...,
    bool evidenceAdded)
```

As miniature proofs these are acceptable.

But their current names can be misread as a trust boundary.

Any caller can create:

```cpp
ExistingImplementationReport report;
report.searched = true;
```

or invoke:

```cpp
retry_permitted(..., true);
```

No mechanism has proved anything.

## Required remediation

Do **not** solve this by inventing production capabilities in v4 stabilization.

Instead make the reference nature explicit.

### Lower-layer case

Rename the semantic predicate away from authority vocabulary.

For example:

```cpp
primitive_judgment_precondition_met(...)
```

instead of:

```cpp
primitive_judgment_authorized(...)
```

The function proves:

> the reference state supplied to it represents a completed search.

It does not prove who performed that search.

### Retry case

Replace the raw semantic name:

```text
evidenceAdded
```

with an explicit reference-model observation type or at minimum a name that does not imply verified evidence.

For example:

```cpp
struct RetryEvidenceState
{
    bool hasNewEvidence;
};
```

and document:

> This is reference state, not a production evidence capability.

The future ADL is responsible for defining how real mechanisms mint trustworthy search/evidence receipts.

## Required comments

The reference implementation must explicitly state:

```text
caller-constructed proof state is acceptable only because
this repository proves semantics, not trust provenance.
```

This protects the ADL from accidentally treating these values as security capabilities.

---

# 13. S1-04 — `ToolContract::allowedFlags` advertises behavior that does not exist

## Current state

`ToolContract` currently has:

```cpp
std::vector<std::string> allowedFlags;
```

and comments state that allowed trailing flags pass through.

But:

```cpp
construct_invocation()
```

does not consume requested flags.

`ActionIntent` has no flag field.

And:

```cpp
validate_invocation()
```

requires:

```text
argv.size() == argvTemplate.size()
```

therefore any trailing flag fails validation.

The field is dead data and its comments are false.

## Required remediation

For stabilization, remove:

```cpp
allowedFlags
```

and every claim that this reference contract supports flags.

Do not design the production argv schema now.

The semantic proof required by v4 is only:

> a declared contract mechanically determines invocation shape; guessed fixed syntax is not accepted.

The current fixed-template miniature is sufficient for that proof once it stops claiming unsupported flag semantics.

Production structured arguments belong to ADL.

---

# 14. S1-05 — empty applicable rule set and absent policy must remain different

This requirement is related to S0-02 but deserves its own invariant.

The following are valid different states:

### World A — policy missing

```text
InvocationPolicy unavailable
```

Result:

```text
control cannot prove requirements
→ fail closed
```

### World B — policy present, no row applies

```text
InvocationPolicy valid
ActionIntent has no mandatory rule
```

Result:

```text
zero mandatory requirements
→ this is a valid policy decision
```

Do not infer World B from World A.

Add explicit tests preventing future regression.

This distinction becomes especially important after ADL, where a transport/storage failure must never masquerade as:

```text
"no control requirement"
```

---

# 15. S1-06 — compiled defaults are bootstrap defaults, not hidden authority

`default_invocation_policy()` currently represents the compiled scar backlog.

That is useful.

But its architectural status must be clarified.

## Required rule

The compiled default is:

```text
bootstrap seed
```

for an empty cognition chain.

Once cognition contains an explicit `InvocationPolicy`:

```text
snapshot policy
```

is authoritative.

The running binary MUST NOT silently merge new compiled defaults into an existing snapshot.

Otherwise upgrading the executable could mutate cognition semantics without a governed cognition transaction.

Therefore:

```text
binary update
≠
policy update
```

If a future executable introduces a new default rule:

```text
existing cognition
```

does not receive it automatically.

A governed migration/transaction must add the rule.

Add this as a normative comment and executable compatibility test.

---

# 16. S2-01 — draft README is materially stale

Current draft README still states:

```text
Status: draft, v3 phase 2C (semantic closure)
```

while remote main is:

```text
v4.6 complete
```

This is no longer harmless documentation lag.

README is an entry surface for humans and agents.

A stale entry surface is itself an activation defect.

## Required replacement status

The README should state approximately:

```text
Status:
Executable specification through v4.6.

v4 cognition-activation semantics are accepted design material.
This repository is still a specification/reference implementation,
not the production Context runtime.

The current runnable cognition_loop demonstrates the v3 causal loop.
v4 Cognitive Control has executable semantic proofs but is not yet
wired into a production work/execution loop.
```

Also update design lineage.

Remove any wording implying current v4 reference helpers are production runtime mechanisms.

---

# 17. S2-02 — v4 validation report still describes a pre-acceptance world

Current:

```text
docs/architecture/v4-validation-report.md
```

still begins as:

```text
Status: candidate record
```

and says the artifact-causal activation trial remains to be executed.

But the canonical trial later passed.

This document therefore freezes an obsolete intermediate state while `v4-roadmap.md` says complete.

## Required remediation

Turn the validation report into a final closeout report.

It must bind:

```text
final draft v4 head:
    b15e41f3...

canonical acceptance evidence:
    context-v4-activation-trial-2026-09-20.md

canonical closeout head:
    6cd93db9...
```

It should explicitly distinguish:

```text
compiled repo-semantics proofs
```

from:

```text
artifact-causal activation proof
```

and record that the trial passed.

The report should also record any stabilization deltas introduced by this remediation and point to the later re-validation commit.

---

# 18. S2-03 — canonical obligation lifecycle contradicts accepted closeout

The canonical repository currently states:

```text
v4 complete
```

and contains a PASS audit.

However:

```text
OBL-20260918T215000Z-B4D6A8
```

remains:

```yaml
status: open
```

Its own completion condition has effectively been satisfied.

This is a canonical lifecycle drift.

## Required remediation

Close the obligation through the ordinary canonical lifecycle.

Do not delete or rewrite history.

Update its lifecycle to reflect completion and bind the completion evidence.

Conceptually:

```yaml
status: completed
completed_at: ...
completion_evidence:
  - evidence/audits/context-v4-activation-trial-2026-09-20.md
  - final v4 validation report
```

Use the repository's actual obligation schema and lifecycle conventions rather than inventing fields if those names differ.

Its historical statement remains.

Only lifecycle state advances.

## Proof

The normal record/lifecycle tests and context gate must pass.

---

# 19. S2-04 — final v4 closeout must mention stabilization

The existing closeout currently means:

```text
v4 feature phases completed
```

After this remediation, we need a more precise distinction:

```text
v4.6
=
architecture feature/proof closeout

v4 stabilization
=
post-closeout semantic consistency repair before ADL
```

Do not rename this work to v4.7 if that falsely suggests new architecture scope.

A suitable label is:

```text
v4 stabilization
```

or:

```text
v4 semantic closeout amendment
```

The roadmap should record it as post-v4 stabilization, not a new feature phase.

---

# 20. S3-01 — real runtime integration is intentionally absent

Current `LLM::work()` does not run:

```text
ActionIntent
→ Cognitive Control
→ PreparationPacket
→ requirement satisfaction
→ Judgment
```

The comment still leaves:

```text
VerifyLiveFacts + Thinking + CallTools
```

opaque.

This is not a defect in v4 stabilization.

It is the next architectural boundary.

## Required remediation

Do not wire the control plane into `LLM::work()` in this batch.

Instead make the boundary explicit in:

```text
README
v4-roadmap
v4-validation-report
runtime comments
```

Normative statement:

> v4 proves Cognitive Control semantics. The draft does not yet claim an operational Cognitive Control runtime integrated into the participant work loop.

This prevents future readers from conflating:

```text
semantic executable proof
```

with:

```text
runtime deployment
```

Runtime integration begins only after ADL.

---

# 21. S3-02 — real lower-layer search remains outside the draft

Current v4.5 truthfully calls A8 a miniature.

Keep that boundary.

The draft may prove:

```text
search is mandatory
report reaches Judgment
uninformed primitive judgment is refused
```

It must not claim to provide:

```text
real dependency discovery
real C++ symbol indexing
real cross-repository search
```

Those are execution-time mechanisms.

Update any language that could imply otherwise.

---

# 22. S3-03 — live verification remains outside the draft

Likewise:

```text
MakeLiveClaim
→ VerifyLive
```

is currently a policy demand only.

That is correct for the specification.

The draft must state:

```text
VerifyLive requirement semantics
    ✅

live verifier implementation
    ❌ intentionally deferred to Runtime ADL
```

Do not create fake live data in the draft merely to make the path look complete.

---

# 23. S3-04 — ToolContract remains a semantic miniature

After removal of the unsupported `allowedFlags` field, the fixed argv template remains sufficient to prove:

```text
declared contract
→ mechanical invocation shape

guess
→ rejected
```

Do not generalize it further during stabilization.

Production tool descriptors, MCP schemas, Operator integration, environment binding and execution receipts are ADL concerns.

---

# 24. Required v4 control-plane shape after remediation

The stabilized reference flow should read conceptually as:

```text
Snapshot
    │
    ├── InvocationPolicy PRESENT?
    │       no → fail closed
    │
    ▼
ActionIntent
    +
CognitiveNeed[]        // optional discretionary path
    │
    ▼
derive requirements
    │
    ▼
Prepared requirements
    │
    ├── BeforeJudgment
    │       Pending / Satisfied / Failed
    │
    └── BeforeExecution
            Pending / Satisfied / Failed
    │
    ▼
snapshot-local deterministic resolution
    │
    ├── MandatoryRecall
    │       exact key lookup
    │
    └── everything requiring an external mechanism remains Pending
    │
    ▼
PreparationPacket
    │
    ├── ready_for_judgment()
    └── ready_for_execution()
```

External-mechanism satisfaction remains representable in tests, but no production mechanism is designed here.

---

# 25. Candidate stabilized type shape

This is explanatory, not a demand for exact spelling.

```cpp
enum class RequirementStatus
{
    Pending,
    Satisfied,
    Failed,
};

enum class RequirementBoundary
{
    BeforeJudgment,
    BeforeExecution,
};

struct InvocationRule
{
    ActionKind action { ActionKind::BeginTask };

    std::optional<ClaimClass> claimClass;

    RequirementKind requirement {
        RequirementKind::MandatoryRecall
    };

    RequirementBoundary boundary {
        RequirementBoundary::BeforeJudgment
    };

    std::string subject;
    bool blocking { true };
};

struct PreparedRequirement
{
    CognitiveRequirement requirement;

    RequirementBoundary boundary {
        RequirementBoundary::BeforeJudgment
    };

    RequirementStatus status {
        RequirementStatus::Pending
    };

    std::vector<std::string> evidence;
};

struct PreparationPacket
{
    ActionIntent intent;

    std::vector<CognitiveNeed> discretionaryNeeds;

    std::vector<PreparedRequirement> requirements;

    std::vector<std::string> mandatoryContext;
    std::vector<std::string> knownPits;
    std::vector<std::string> liveFacts;

    std::optional<PreparationFailure> failure;

    bool ready_for_judgment() const;
    bool ready_for_execution() const;
};
```

Again:

> Production provenance of `Satisfied` is deliberately NOT solved here.

The ADL must later decide which mechanism is allowed to produce trustworthy satisfaction evidence.

---

# 26. Default invocation policy after stabilization

The default policy should remain small and scar-derived.

Every current row must explicitly declare its boundary.

A reasonable stabilized table is:

| Action                     | Requirement                                                                 | Boundary        |
| -------------------------- | --------------------------------------------------------------------------- | --------------- |
| `CreateCppSymbol`          | `MandatoryRecall("naming policy")`                                          | BeforeJudgment  |
| `IntroducePrimitive`       | `SearchLowerLayer("eligible lower layers")`                                 | BeforeJudgment  |
| `InvokeTool`               | `InspectToolContract("declared tool argv contract")`                        | BeforeExecution |
| `RetryFailure`             | `InspectKnownPit("failure fingerprint")`                                    | BeforeJudgment  |
| `MakeCanonicalClaim`       | `VerifyCanonical("canonical record")`                                       | BeforeJudgment  |
| `MakeLiveClaim`            | `VerifyLive("live source")`                                                 | BeforeJudgment  |
| `ModifyReferencedContract` | `MandatoryRecall("reference integrity sweep (P-52)")`                       | BeforeJudgment  |
| `Commit`                   | `RunMechanicalCheck("attribution subject-position lint (P-51)")`            | BeforeExecution |
| `Publish`                  | `RunMechanicalCheck("full default gate PASS receipt at exact head (P-53)")` | BeforeExecution |
| `Publish`                  | `RequestReview("H2 exact-delta review")`                                    | BeforeExecution |
| `AcceptCandidate`          | `RequestReview("content-bound H2 evidence")`                                | BeforeExecution |

The exact strings remain reference keys, not production selectors.

---

# 27. Test remediation matrix

The stabilization should expand the existing v4 proof suite rather than create a separate parallel testing philosophy.

## 27.1 Policy presence

```text
pit/control-policy-missing-fails-closed
```

Proves:

```text
missing InvocationPolicy
≠
empty applicable rule set
```

## 27.2 Genesis activation

```text
genesis_contains_default_invocation_policy
```

Proves the real boot path carries v4.

## 27.3 Bootstrap policy does not overwrite carried policy

```text
restored_policy_beats_compiled_default
```

Proves binary defaults are bootstrap-only.

## 27.4 Legacy snapshot

```text
legacy_snapshot_does_not_gain_v4_policy_silently
```

Proves historical semantics remain historical.

## 27.5 Readiness stages

```text
publish_not_execution_ready_when_requirements_only_listed
```

and:

```text
publish_execution_ready_only_after_all_blocking_requirements_satisfied
```

## 27.6 Naming recall

Existing scenario remains, but use exact lookup rather than substring matching.

Also add:

```text
similar_prose_does_not_satisfy_naming_policy
```

## 27.7 Canonical verification

Prove a random memory record containing:

```text
"canonical record"
```

does not satisfy `VerifyCanonical`.

## 27.8 Failure fingerprint integration

`RetryFailure` packet must receive the actual known-pit result produced from its fingerprint.

The current A7 isolated test and control-plane test should become one connected semantic path.

## 27.9 ClaimClass

Two otherwise identical intents differing only in claim class must produce different requirements under a claim-scoped rule.

## 27.10 CognitiveNeed

Prove discretionary needs survive into the packet and cannot erase mandatory policy requirements.

## 27.11 Tool contract

Remove the false flag contract.

Retain:

```text
contract-generated invocation passes
guessed fixed-layout variants fail
```

## 27.12 Reference proof honesty

Tests must not call caller-created state an authenticated receipt or authority.

Use "reference state", "semantic precondition", or equivalent terminology.

---

# 28. Pit-ledger requirements

Every newly discovered stabilization defect should receive a pit entry using the next available repository IDs.

At minimum record the following scars:

```text
InvocationPolicy present in type/tests but absent from real genesis

missing control policy was described as fail-closed
while executable behavior was fail-open

PreparationPacket::ready conflated requirement listing
with requirement satisfaction

VerifyCanonical / InspectKnownPit were accidentally routed through
generic substring recall

ClaimClass existed but did not affect policy

ToolContract.allowedFlags claimed unsupported behavior
```

These are not cosmetic cleanup items.

They are exactly the class of failure v4 exists to prevent:

> semantics known in one surface but not causally active in another.

Each pit should map to its executable regression.

---

# 29. Serialization impact

The current snapshot serialization version is:

```text
v7
```

because `InvocationPolicy` entered cognition.

If stabilization changes the serialized shape of:

```text
InvocationRule
```

for example by adding:

```text
claimClass
RequirementBoundary
```

the serialization version MUST advance.

Do not overload v7 with a changed binary layout.

Likely:

```text
v8
```

but use the repository's normal version sequencing at implementation time.

Required properties remain:

```text
versioned
bounded
fail-closed
golden-vector pinned
old-version semantics explicit
```

A new golden digest is expected if the canonical binary shape changes.

That is not a regression when bound to an intentional schema version change.

---

# 30. No silent legacy upgrade

This deserves a separate hard invariant.

Deserializing an older snapshot may perform syntactic compatibility work.

It MUST NOT invent later accepted cognition.

Therefore:

```text
deserialize(v6)
```

may populate type defaults necessary to construct a C++ object.

But it must not semantically assert:

```text
"this historical cognition had the v4 policy"
```

when it did not.

If the result cannot participate safely in v4 control:

```text
control policy unavailable
```

is the correct typed state.

Migration is a governed cognition event.

Deserialization is not migration.

---

# 31. Documentation truth after remediation

The following surfaces must tell the same story.

## `README.md`

Must say:

```text
v4.6 semantic architecture complete
post-v4 stabilization complete
production runtime not landed
```

## `v4-roadmap.md`

Must preserve:

```text
v4.0-v4.6 complete
```

and append:

```text
post-closeout stabilization
```

with exact proof refs.

Do not pretend this is v5.

## `v4-validation-report.md`

Must become final, not candidate.

Must bind:

```text
feature closeout
activation trial
stabilization closeout
```

## `cognitive-boundary-model.md`

Should remain unchanged unless stabilization finds a direct contradiction.

This plan currently finds implementation drift, not an ontology defect.

## `invariant-inventory.md`

Any v4 invariant whose executable semantics change during stabilization must point to the updated proof.

## `pit-regression-map.md`

New stabilization scars must be present and executable.

---

# 32. Canonical reconciliation

The following changes occur in `qiven-context`, not because it owns the draft implementation, but because it owns acceptance truth.

Required:

```text
OBL-20260918T215000Z-B4D6A8
    open
→
completed
```

after stabilization is accepted.

The completion evidence should include:

```text
original v4 activation audit
+
draft final v4 validation report
+
stabilization validation evidence
+
exact final draft commit
```

Canonical current state should then say approximately:

```text
v4 executable semantics stabilized;
Runtime ADL is the next design boundary.
```

It should no longer say K5 is automatically next if the owner has superseded that sequencing with Runtime Landing.

That sequencing update is a canonical project judgment and should be recorded normally.

---

# 33. Recommended implementation batches

Do not perform this as one giant patch.

The recommended order is:

## V4S-01 — Policy presence and fail-closed semantics

Files:

```text
src/persistence.cpp
include/qiven/context/runtime.hpp
tests/control_plane.cpp
tests/persistence_fencing.cpp
```

Work:

```text
genesis gets default InvocationPolicy

missing policy becomes typed failure

empty valid policy remains distinguishable

legacy policy absence remains historical
```

This is the highest-priority batch.

---

## V4S-02 — Requirement lifecycle and readiness

Files:

```text
include/qiven/context/cognition.hpp
include/qiven/context/runtime.hpp
src/persistence.cpp
tests/control_plane.cpp
```

Work:

```text
RequirementBoundary

RequirementStatus

PreparedRequirement

remove ambiguous ready()

ready_for_judgment()

ready_for_execution()

serialization bump if required
```

No external resolver ports yet.

---

## V4S-03 — Resolver semantic closure

Files:

```text
include/qiven/context/runtime.hpp
tests/control_plane.cpp
tests/tool_and_retry.cpp
```

Work:

```text
MandatoryRecall uses exact snapshot key semantics

VerifyCanonical no longer resolved as generic memory search

InspectKnownPit uses FailureFingerprint path

execution-time requirements remain Pending
```

Connect the A7 miniature to the main packet semantics.

---

## V4S-04 — Claim and discretionary paths

Files:

```text
include/qiven/context/cognition.hpp
include/qiven/context/runtime.hpp
src/persistence.cpp
tests/control_plane.cpp
```

Work:

```text
optional claim-class rule matching

ClaimClass executable proof

CognitiveNeed carried into PreparationPacket

mandatory rules cannot be waived by CognitiveNeed
```

---

## V4S-05 — Reference-mechanism honesty

Files:

```text
include/qiven/context/runtime.hpp
tests/tool_and_retry.cpp
tests/control_plane.cpp
```

Work:

```text
remove ToolContract.allowedFlags

remove false trailing-flag claims

rename authorization-like reference predicates

mark bool/reference evidence as non-production trust state
```

No production receipt/capability types.

---

## V4S-06 — Corpus and canonical reconciliation

Draft:

```text
README.md
v4-roadmap.md
v4-validation-report.md
invariant-inventory.md
pit-regression-map.md
```

Canonical:

```text
OBL-20260918T215000Z-B4D6A8
current state/checkpoint
stabilization audit record
```

Make every surface agree.

---

## V4S-07 — Stabilization validation

Final validation must run on the exact candidate head.

It should prove:

```text
all original v4.6 proofs remain green

all new stabilization regressions green

Debug + Release

serialization golden updated intentionally

artifact corruption still fails closed

activation survives serialize/restore

missing policy fails closed

Publish cannot become execution-ready by requirement listing alone

fingerprint recall is actually connected

ClaimClass is not decorative

CognitiveNeed cannot waive policy
```

Then run the normal full local gate and merge-proof.

---

# 34. Final stabilization acceptance matrix

v4 is stable only when every row below is true.

| Property                | Required final state                                |
| ----------------------- | --------------------------------------------------- |
| v4 ontology             | unchanged and accepted                              |
| Genesis                 | carries default InvocationPolicy                    |
| Missing policy          | fail-closed                                         |
| Empty applicable rules  | valid and distinct from missing policy              |
| Legacy snapshot         | no silent semantic upgrade                          |
| Requirement derivation  | separate from satisfaction                          |
| Judgment readiness      | explicit                                            |
| Execution readiness     | explicit                                            |
| Publish requirements    | listed ≠ satisfied                                  |
| Mandatory recall        | deterministic exact reference semantics             |
| Canonical verification  | not generic memory substring search                 |
| Pit recall              | connected to FailureFingerprint                     |
| ClaimClass              | materially participates in rule matching            |
| CognitiveNeed           | represented in packet and cannot waive policy       |
| ToolContract            | no unsupported advertised semantics                 |
| Lower-layer report      | clearly reference semantics, not trusted capability |
| Retry evidence          | clearly reference semantics, not trusted capability |
| v4 runtime integration  | explicitly NOT claimed                              |
| Lower-layer real search | explicitly deferred to ADL                          |
| Live verifier           | explicitly deferred to ADL                          |
| README                  | current                                             |
| v4 validation report    | final                                               |
| canonical obligation    | lifecycle reconciled                                |
| pit ledger              | stabilization scars compiled                        |
| full gate               | PASS                                                |
| merge-proof             | PASS                                                |

---

# 35. Freeze rule after stabilization

Once V4S-07 is accepted:

> **v4 semantics freeze.**

From that point, Runtime ADL may choose different production mechanisms but must preserve these semantic obligations.

ADL may replace:

```text
string lookup
reference bool
fixed argv vector
in-memory miniature
```

with:

```text
resolver port
verified receipt
tool schema
execution gateway
daemon
GitStore
```

without reopening v4.

ADL may not silently change:

```text
when an action requires cognition

which requirements are blocking

the distinction between Judgment and Mechanism

the distinction between mandatory and discretionary invocation

the requirement that missing policy fails closed

the requirement that listed evidence is not equivalent to satisfied evidence

the requirement that known cognition be causally activated at the relevant boundary
```

Any proposed change to those semantics is no longer implementation detail.

It requires an explicit architecture amendment.

---

# 36. Exit condition into Runtime ADL

Do not start Runtime ADL merely because the remediation patches compile.

The transition occurs only when the final review can truthfully state:

> **qiven-context-draft v4 is a self-consistent executable specification of cognition activation. Its remaining incompleteness is exclusively implementation topology and production mechanism, not ambiguity about the semantics those mechanisms must preserve.**

At that point the ADL question becomes clean:

```text
Given this frozen semantic contract,

what runtime components,
ports,
trust boundaries,
process boundaries,
storage topology,
tool mediation,
and authority cutover

must exist to make it real?
```

That is the correct boundary between stabilized v4 and Runtime ADL.

---

# 37. Final remediation thesis

v4 discovered the correct problem:

```text
cognition exists
but is not activated
```

The stabilization itself exposes the same failure class inside the executable specification:

```text
InvocationPolicy existed
but genesis did not activate it

"fail closed" existed in commentary
but control flow did not enforce it

requirements existed
but "listed" and "satisfied" were not separated

FailureFingerprint existed
but pit lookup was not connected to packet preparation

ClaimClass existed
but policy matching ignored it
```

These are not reasons to discard v4.

They are the strongest possible reason to finish it correctly.

The stabilization goal is therefore:

> **Make the v4 executable specification obey the very activation discipline that v4 requires from future cognition runtimes.**

Once that is true, freeze v4 and move to Runtime ADL.
