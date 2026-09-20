# v4 Stabilization Residue Cleanup — Final Freeze Patch

**Status:** final pre-ADL cleanup patch
**Purpose:** remove the remaining semantic defect and corpus drift found during post-V4S review
**Scope:** `qiven-context-draft` plus minimal canonical `qiven-context` truth reconciliation
**Classification:** residue cleanup, not a new v4 feature phase
**Next boundary after acceptance:** Runtime ADL

---

# 1. Baseline

This patch is based on the reviewed remote heads:

```text id="6e69p5"
qiven-context-draft/main
388480ea6d4f603551c086babb2da33b586d00c7
merge(draft): v4 stabilization (V4S-01..07)
```

```text id="c4tzek"
qiven-context/main
15fbcc03c949f091ce421e794b1e0a05b64a486e
merge(context): v4 stabilization canonical closeout
```

V4S-01..07 are accepted as successful.

This patch does **not** reopen the following stabilized semantics:

```text id="nruohd"
InvocationPolicy presence and fail-closed absence

RequirementStatus:
    Pending
    Satisfied
    Failed

RequirementBoundary:
    BeforeJudgment
    BeforeExecution

ready_for_judgment()
ready_for_execution()

exact-key MandatoryRecall

FailureFingerprint-driven InspectKnownPit

VerifyCanonical remaining external-resolver-owned

claim-scoped InvocationRule matching

CognitiveNeed as the discretionary request path

ToolContract as a fixed-template reference miniature

reference-state honesty for non-production proofs
```

The only goal is to ensure the frozen v4 base contains no known contradiction between these semantics and its executable/documented form.

---

# 2. Findings

Post-stabilization review found four residue classes.

## R-01 — Empty FailureFingerprint selectors accidentally wildcard-match

**Severity:** semantic defect
**Required:** yes

Current `find_related_records()` contains behavior equivalent to:

```cpp id="b5jcq4"
record.title.find(fingerprint.tool) != std::string::npos
```

and:

```cpp id="9ark55"
record.title.find(fingerprint.category) != std::string::npos
```

In C++:

```cpp id="egbpjx"
text.find("")
```

always succeeds.

Therefore a fingerprint such as:

```cpp id="3rr8a2"
FailureFingerprint {
    .tool = "",
    .category = ""
};
```

may match arbitrary `Lesson` or `Risk` records.

This can create the false causal chain:

```text id="mjy9h6"
invalid / under-specified FailureFingerprint
    ↓
unrelated Lesson/Risk accidentally matches
    ↓
knownPits becomes non-empty
    ↓
InspectKnownPit becomes Satisfied
    ↓
ready_for_judgment() becomes true
```

That violates stabilized v4's fail-closed principle.

A blocking epistemic requirement must never be satisfied by an empty selector.

---

## R-02 — `CognitiveNeed::mandatory` advertises nonexistent semantics

**Severity:** semantic ambiguity
**Required:** yes

The stabilized architecture now has an explicit distinction:

```text id="vdusjt"
CognitiveNeed
=
discretionary request originating from Judgment
```

versus:

```text id="0x8afm"
CognitiveRequirement
=
mandatory obligation derived by Cognitive Control policy
```

However the current type still contains:

```cpp id="m3wrog"
struct CognitiveNeed
{
    CognitiveNeedKind kind;
    std::string subject;
    std::string scope;
    bool mandatory { false };
};
```

The field:

```cpp id="2vhge5"
mandatory
```

has no executable meaning.

Setting:

```cpp id="81c4mp"
need.mandatory = true;
```

does not:

```text id="bjh4kd"
create a CognitiveRequirement
change readiness
block execution
change InvocationPolicy
```

Therefore the type advertises a capability it does not possess.

This is the same defect class that V4S removed from:

```text id="32gkf0"
ToolContract::allowedFlags
```

A participant must not be able to promote its own discretionary request into a mandatory policy obligation merely by setting a boolean.

---

## R-03 — reference comments and closeout documents still contain obsolete claims

**Severity:** truth/documentation drift
**Required:** yes

Known drift includes:

```text id="lpch9k"
ToolContract comment still claims trailing allowed flags pass through

v4-validation-report.md says FINAL CLOSEOUT
while still containing:
    "What remains for v4 acceptance"
    "remaining acceptance step"

validation table still names:
    primitive_judgment_authorized

some validation prose still names:
    serialization v7

pit-regression-map.md still calls:
    A6 / A7 / A8
a backlog

README lineage stops at:
    v3 phases 1-2C
```

These are individually small.

Together they violate the freeze requirement:

> A fresh consumer should not have to infer which statement in a final closeout document is stale.

---

## R-04 — canonical `state/current.md` contains historical "next" statements written as current truth

**Severity:** canonical truth drift
**Required:** yes

The canonical current-state document correctly ends with:

```text id="w3bb6y"
v4 stabilized
OBL-B4D6A8 done
Runtime ADL next
```

but earlier prose still contains statements such as:

```text id="hf38ex"
v3 Phase 5 is the next draft phase

OBL-B4D6A8 remains the governing validation boundary
```

Those statements were historically true.

They are no longer current.

The problem is not preserving history.

The problem is that historical state is written in present-tense/current-state form inside:

```text id="viflip"
state/current.md
```

A fresh consumer can therefore activate stale state before reaching the later correction.

That is exactly the failure class v4 exists to eliminate.

---

# 3. Patch principles

This patch follows six rules.

### P1 — No new architecture

Do not introduce:

```text id="nctcmf"
new ActionKind
new RequirementKind
new control-plane layer
new resolver port
new trust capability
new runtime topology
new harness adapter
```

---

### P2 — No Runtime ADL implementation

Do not implement:

```text id="bkj6qh"
ZCode hooks
MCP adapter
ExecutionGateway
daemon
GitStore production adapter
real lower-layer search
real canonical resolver
real live verifier
```

Those start only after v4 freeze.

---

### P3 — Repair false satisfaction, not retrieval quality

R-01 is not an invitation to build better fuzzy pit retrieval.

The patch only guarantees:

```text id="6ylt20"
empty selector
≠
wildcard
```

Semantic retrieval quality remains future mechanism work.

---

### P4 — Discretionary cannot self-promote to mandatory

Mandatory status comes only from:

```text id="lvymq0"
InvocationPolicy
→ CognitiveRequirement
```

not:

```text id="mr79w7"
participant request
→ bool mandatory
```

---

### P5 — Final documents describe final truth

Historical intermediate states may remain in Git history.

They should not remain written as future/current work inside documents titled:

```text id="iwbvoh"
FINAL CLOSEOUT
Current State
```

---

### P6 — No new V4S feature numbering

This work is:

```text id="u39wbk"
v4 stabilization residue cleanup
```

not:

```text id="mycwhp"
v4.7
V4S-08
v5
```

unless repository tooling mechanically requires a batch identifier.

---

# 4. Patch R-01 — Empty FailureFingerprint must fail closed

## 4.1 Current defect

Current semantic shape:

```cpp id="t7oqce"
const bool matches =
    record.title.find(fingerprint.tool) != std::string::npos ||
    record.statement.find(fingerprint.tool) != std::string::npos ||
    record.title.find(fingerprint.category) != std::string::npos;
```

Empty strings create wildcard matches.

---

## 4.2 Required implementation

Every selector must be explicitly non-empty before matching.

Reference implementation:

```cpp id="1ehocm"
const bool toolMatch =
    !fingerprint.tool.empty() &&
    (record.title.find(fingerprint.tool) != std::string::npos ||
     record.statement.find(fingerprint.tool) != std::string::npos);

const bool categoryMatch =
    !fingerprint.category.empty() &&
    (record.title.find(fingerprint.category) != std::string::npos ||
     record.statement.find(fingerprint.category) != std::string::npos);

const bool matches =
    toolMatch || categoryMatch;
```

If other selectors are later included, the same law applies:

```text id="zpw31c"
selector empty
→ contributes no match
```

Never:

```text id="uisz5j"
selector empty
→ match everything
```

---

## 4.3 Stable-message handling

Do not add `stableMessage` fuzzy matching in this patch unless it is already part of the accepted reference semantics.

The defect being fixed is selector validity, not search quality.

The existing v4 division remains:

```text id="zcy59h"
FailureFingerprint
    identifies the failure

find_related_records()
    is only a reference miniature

production pit retrieval
    Runtime ADL / later mechanism design
```

---

## 4.4 Required regression

Add a named pit regression.

Suggested name:

```text id="hivbny"
pit.empty_failure_selector_never_matches
```

Required scenario:

```cpp id="udvhlr"
Snapshot snapshot;

MemoryRecord unrelated;
unrelated.kind = MemoryRecord::Kind::Lesson;
unrelated.title = "unrelated compiler lesson";
unrelated.statement = "completely unrelated";
snapshot.memory.push_back(unrelated);

FailureFingerprint fingerprint;
fingerprint.tool = "";
fingerprint.category = "";

const auto result =
    find_related_records(snapshot, fingerprint);

QCD_CHECK(result.empty());
```

And control-plane-level proof:

```text id="vgn67k"
RetryFailure
+
blocking InspectKnownPit
+
priorFailure exists
+
tool/category selectors empty
+
snapshot contains unrelated Lesson/Risk
    ↓
knownPits empty
InspectKnownPit Failed
ready_for_judgment == false
```

This second assertion matters.

The test must prove not only search behavior, but the final Cognitive Control consequence.

---

## 4.5 Positive control

Retain or add:

```text id="k91uvx"
tool = "qiven"
```

matching:

```text id="nzvg99"
"qiven gate odyssey"
```

to prove the fix did not disable legitimate reference lookup.

---

# 5. Patch R-02 — Remove `CognitiveNeed::mandatory`

## 5.1 Required type change

Change:

```cpp id="cbmxq5"
struct CognitiveNeed
{
    CognitiveNeedKind kind { CognitiveNeedKind::Recall };
    std::string subject;
    std::string scope;
    bool mandatory { false };
};
```

to:

```cpp id="pk42z6"
struct CognitiveNeed
{
    CognitiveNeedKind kind { CognitiveNeedKind::Recall };
    std::string subject;
    std::string scope;
};
```

---

## 5.2 Normative semantics

Add a concise normative comment:

```text id="5gi5zo"
CognitiveNeed is always discretionary.

A participant may request additional epistemic work,
but cannot create, satisfy, waive or upgrade a mandatory
CognitiveRequirement.

Mandatory obligations originate only from InvocationPolicy.
```

---

## 5.3 No replacement field

Do not replace:

```text id="52i2c7"
mandatory
```

with:

```text id="cv0zsl"
priority
required
mustResolve
blocking
```

in this patch.

That would recreate the same ambiguity.

If future ADL needs scheduling priority for discretionary needs, it can design that independently.

---

## 5.4 Required tests

Existing:

```text id="m57x5a"
CognitiveNeed cannot waive mandatory naming recall
```

remains.

Strengthen the test contract conceptually:

```text id="ee7pol"
same ActionIntent
+
zero discretionary needs

and

same ActionIntent
+
arbitrary discretionary needs

→ identical mandatory requirement set
→ identical blocking semantics
```

The packet may differ in:

```text id="4euhei"
discretionaryNeeds
```

but not:

```text id="a29x3o"
requirements
requirement blocking state
policy-derived readiness
```

except where a future resolver actually satisfies something.

A need itself never does.

---

# 6. Patch R-03A — ToolContract comment truth

Current stale wording includes:

```text id="nfj8ca"
trailing allowed flags pass through in order
```

There are no allowed flags in the stabilized miniature.

Delete this statement.

The final comment should say only what the code implements:

```text id="zrniar"
Construct the invocation from the fixed declared template.
Fixed words remain verbatim.
Known placeholders are substituted from ActionIntent.
```

P-59's own implementation must obey P-59.

---

# 7. Patch R-03B — Finalize `v4-validation-report.md`

The file is already titled:

```text id="vc90nl"
v4 Validation Report — FINAL CLOSEOUT
```

It must therefore contain no unresolved future acceptance step that has already happened.

---

## 7.1 Remove obsolete future section

Replace:

```text id="a4105p"
## What remains for v4 acceptance — the H1 boundary
```

and the future-tense trial instructions with a historical completed section:

```text id="yu5rmz"
## Artifact-causal activation acceptance
```

That section should state:

```text id="db799e"
The artifact-causal activation trial was executed on 2026-09-20.

Verdict:
PASS with finding F1.

Fresh isolated consumer:
GLM-5.3-Flash.

The consumer operated artifact-only during Phase A.

Finding F1:
cross-repository guard landing was absent from carried canonical cognition.

The consumer correctly abstained rather than inventing the missing fact.

F1 was subsequently closed in canonical cognition.

This trial supplies the causal activation evidence required beyond
the compiled repo-semantics proofs.
```

No wording such as:

```text id="xv99or"
remaining
will run
on PASS
needs H1
```

should remain in a final closeout report.

---

## 7.2 Update renamed symbols

Replace stale:

```text id="jodl86"
primitive_judgment_authorized
```

with:

```text id="0fq1ls"
primitive_judgment_precondition_met
```

where the current implementation is being named.

---

## 7.3 Update serialization version

Replace any current-state statement implying:

```text id="e0u6zx"
InvocationPolicy serialization v7
```

with the stabilized truth:

```text id="c569aa"
v8:
    explicit policy presence
    optional ClaimClass predicate
    RequirementBoundary
```

Historical v7 references may remain only when explicitly describing the v7 historical format.

---

## 7.4 Clarify the validated versus merge head

Current stabilization evidence distinguishes:

```text id="nbepjl"
validated semantic head:
55a8fa9

validation-record head:
61d9c22

merged main:
388480e
```

Keep that distinction.

Do not casually say all tests executed against `388480e` if the actual exact tested semantic head was `55a8fa9`.

Truthful wording:

```text id="0gl0ne"
V4S semantic candidate 55a8fa9 was gate/merge-proof validated.
61d9c22 added the validation record.
The accepted branch was subsequently merged to main as 388480e.
```

---

# 8. Patch R-03C — Clean `v4-roadmap.md`

The post-V4S section is generally correct.

Only remove or correct residual stale terminology.

Important rule:

```text id="2splpl"
v4.0-v4.6
=
feature program complete

V4S
=
post-closeout semantic stabilization complete

residue cleanup
=
final truth repair only
```

Do not append a new architecture phase.

At the end, add one concise final freeze statement after this cleanup is accepted:

```text id="97dj2t"
Final residue review completed after V4S.

No remaining known semantic contradiction is carried into Runtime ADL.

v4 semantic contract is frozen.
```

Only add this after the patch passes final validation.

---

# 9. Patch R-03D — Clean `pit-regression-map.md`

Current tail says approximately:

```text id="v3gzmw"
Backlog:
A6 tool-contract routing
A7 failure-fingerprint evidence
A8 lower-layer search
```

This is stale because their **semantic reference forms** have already landed.

Replace with an explicit distinction.

Suggested wording:

```text id="m5rd9u"
## Runtime mechanisms deferred after v4 semantic closeout

The following semantic boundaries are compiled in v4,
while their production mechanisms remain Runtime-ADL work:

- A6:
  tool-contract semantics compiled;
  real harness/tool routing deferred.

- A7:
  failure fingerprint + blind-retry semantics compiled;
  trustworthy evidence provenance and operational failure storage deferred.

- A8:
  lower-layer-search obligation and judgment precondition compiled;
  real symbol/dependency search mechanism deferred.

These are not unresolved v4 semantic backlog items.
```

---

## 9.1 Add the empty-selector scar

Add a new pit entry using the next available ID.

Conceptually:

```text id="9blovi"
Pit:
empty FailureFingerprint tool/category selectors
were passed to std::string::find, where "" matches every string,
allowing an unrelated Lesson/Risk to falsely satisfy InspectKnownPit.

Activation trigger:
RetryFailure / InspectKnownPit

Compiled guard:
empty selectors never participate in matching;
control-plane regression proves unrelated scars cannot satisfy the requirement.
```

Classification:

```text id="qzvy41"
PROC
```

This is a genuine executable scar and deserves to compile.

---

## 9.2 `CognitiveNeed::mandatory` cleanup

This may be recorded in the same cleanup record rather than a dedicated pit if the repository's pit policy reserves IDs for observed operational failures.

However, at minimum the architecture corpus must record the rule:

```text id="d9au3s"
discretionary CognitiveNeed cannot self-promote into mandatory policy.
```

If the pit ledger is used for all discovered design scars, give it its own row.

---

# 10. Patch R-03E — README truth cleanup

## 10.1 Design lineage

Current bottom lineage stops at approximately:

```text id="us34va"
draft v0
v1-v2
v3 phases 1-2C
```

Update to include:

```text id="3713e2"
v3 complete architecture hardening
v4 cognition activation
V4S semantic stabilization
final residue cleanup
Runtime ADL next
```

Keep it compact.

---

## 10.2 Preserve status truth

The good current status text should remain:

```text id="s1fqpt"
executable specification
not production runtime

v4 Cognitive Control semantics compiled
not yet wired into a production work/execution loop

Runtime ADL owns integration
```

Do not weaken this distinction.

---

# 11. Patch R-04 — Reconcile canonical `state/current.md`

This is the only canonical `qiven-context` change required beyond recording the cleanup acceptance.

The goal is not to delete history.

It is to ensure current-state prose has one temporal meaning.

---

## 11.1 Historicalize stale paragraphs

Statements such as:

```text id="049vae"
The owner's 2026-09-20 direction accepts v3 roadmap Phase 5
as the next draft phase.
```

must become:

```text id="wqdrwj"
Prior to the v4 program, the owner's 2026-09-20 direction
had selected v3 Phase 5 as the next draft boundary.
That boundary was subsequently superseded by the completed
v4 activation program and V4S stabilization.
```

Likewise:

```text id="jdpagf"
OBL-B4D6A8 remains the governing validation boundary
```

must no longer appear as current truth.

It may become:

```text id="d7zllc"
OBL-B4D6A8 governed acceptance of the executable specification
until its completion during V4S canonical reconciliation.
It is now done.
```

---

## 11.2 Current next boundary must be singular

At the end of cleanup, the only current design boundary should be:

```text id="uof00s"
Runtime ADL against frozen v4 semantics.
```

K5 may remain an open obligation.

But "open work exists" must not be confused with:

```text id="lhitks"
current next architecture boundary
```

Canonical state should say:

```text id="gsuvkl"
K5 remains open.
Its sequencing will be decided inside or against the Runtime ADL plan.

Runtime ADL is the current design boundary.
```

---

# 12. Do not encode the ZCode mapping in this patch

The post-review discussion established an important Runtime-ADL insight:

```text id="2j5bfk"
observable tool call
can act as a physical carrier of ActionIntent

harness interception
can act as a physical Cognitive Control enforcement point
```

This is valuable.

It must **not** be added to frozen v4 as normative semantics.

Why:

```text id="hjylx8"
ActionIntent
is architecture-level

ZCode PreToolUse
is one physical realization
```

v4 remains runtime/harness independent.

Do not add:

```text id="fu68hv"
ZCode
PreToolUse
PostToolUse
Stop
MCP
declare_intent
```

to v4 core types or normative architecture.

Those belong to Runtime ADL as candidate adapter mappings.

The residue patch must preserve this clean boundary.

---

# 13. Required test changes

The final residue candidate must include at minimum the following executable proofs.

### T1 — Empty selector is not wildcard

```text id="4elugv"
unrelated Lesson exists
+
FailureFingerprint.tool == ""
+
FailureFingerprint.category == ""
→ no hit
```

### T2 — Control remains blocked

```text id="7nq8g8"
RetryFailure
+
blocking InspectKnownPit
+
empty selectors
+
unrelated Lesson/Risk exists
→ InspectKnownPit Failed
→ ready_for_judgment false
```

### T3 — Positive pit recall still works

```text id="wdvn2d"
tool == "qiven"
+
qiven-related Lesson exists
→ hit
→ InspectKnownPit Satisfied
→ ready_for_judgment true
```

### T4 — CognitiveNeed cannot alter mandatory requirements

```text id="xpgva4"
packet without discretionary needs
and
packet with arbitrary discretionary needs

→ same mandatory requirement shape
```

No `CognitiveNeed::mandatory` field exists.

### T5 — Existing v4/V4S suite remains green

Every previously accepted proof remains green.

---

# 14. Serialization impact

Removing:

```cpp id="1g34h7"
CognitiveNeed::mandatory
```

does **not** affect Snapshot serialization because `CognitiveNeed` is a runtime/control-plane exchange type and is not part of the pure cognition value tree.

Therefore:

```text id="7qpqgr"
serialization v8 remains v8
```

unless implementation discovers an unexpected serialized dependency.

Do not bump snapshot serialization solely for this cleanup.

R-01 similarly does not alter snapshot binary representation.

---

# 15. Suggested patch sequence

This work should be one compact stabilization-residue branch.

Suggested branch identity:

```text id="jf2a9q"
jason-brother/v4-freeze-cleanup
```

or the project's current active authoring-role namespace if that branch ownership policy is required.

Logical order:

```text id="2gcapk"
1. fix FailureFingerprint empty-selector bug

2. add regression tests

3. remove CognitiveNeed::mandatory

4. clean stale ToolContract comment

5. clean validation report

6. clean roadmap / pit map / README

7. reconcile canonical state/current

8. run full validation

9. record final freeze
```

Do not interleave Runtime ADL design into this branch.

---

# 16. Final validation

At the exact draft candidate head run the normal complete validation profile.

Required result:

```text id="m53op2"
Debug PASS

Release PASS

all prior v4.6 proofs PASS

all V4S proofs PASS

new residue regressions PASS

pit traceability PASS

serialization golden unchanged unless an unrelated real reason appears

corruption fail-closed PASS

merge-proof PASS
```

Canonical reconciliation then runs:

```text id="eb8d1e"
context-local PASS
merge-proof PASS
```

on the exact canonical candidate head.

---

# 17. Final freeze evidence

After acceptance, add a small final record stating:

```text id="a8p37v"
v4 feature closeout:
    complete

V4S semantic stabilization:
    complete

post-V4S residue review:
    complete

known semantic defects at freeze:
    none

next boundary:
    Runtime ADL
```

Bind:

```text id="zo6m7d"
exact draft commit
exact canonical commit
full-gate result
merge-proof result
new pit regression names
```

Do not claim:

```text id="z9bgu0"
production Cognitive Control exists
```

The correct claim remains:

> v4 is a stable executable specification of the semantics that the production Cognitive Control runtime must preserve.

---

# 18. Freeze rule

Once this cleanup is accepted:

# v4 is frozen.

Future Runtime ADL may decide:

```text id="x4ufr8"
process topology
daemon vs embedded runtime
interception ports
harness adapters
MCP adapters
hook adapters
storage adapter
GitStore
canonical/live resolvers
evidence receipts
execution mediation
semantic-intent inference
tool-call observation
output/Stop interception
```

without changing v4.

It must preserve:

```text id="wclti7"
missing policy fails closed

mandatory requirements originate from policy

CognitiveNeed remains discretionary

listed != satisfied

judgment readiness != execution readiness

canonical/live facts use authoritative resolvers

known failure triggers pit recall

blind retry requires new evidence

lower-layer search precedes reusable-primitive judgment

tool syntax is mechanism-owned where a contract exists

private reasoning remains opaque

Cognitive Control governs boundaries, not hidden thought
```

---

# 19. Exit criterion

Runtime ADL begins only after a final review can state:

> **The frozen v4 executable specification contains no known semantic contradiction. Remaining incompleteness consists only of production realization: interception, trusted evidence, external resolution, storage, execution mediation, and runtime topology.**

At that point the architecture question changes from:

```text id="aohmsn"
What should Cognitive Control mean?
```

to:

```text id="u5cnvl"
How should a real runtime enforce the already-frozen Cognitive Control contract?
```

That is the intended handoff into ADL.

---

# 20. Final patch thesis

V4S corrected the major activation semantics.

This final cleanup corrects the last places where the executable/reference corpus could still violate its own rule:

```text id="jxygln"
empty knowledge accidentally looks sufficient

discretionary data advertises mandatory power

obsolete future state remains active in final documentation
```

The final stable state should therefore satisfy one simple principle:

> **Nothing in frozen v4 may claim that cognition, evidence, policy, or completion exists unless the executable or canonical state actually supports that claim.**

After this patch passes exact-head validation, freeze v4 and begin Runtime ADL.
