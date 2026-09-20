# Participant Model — Independence, Dependency, and Authority

**Status: candidate design input (ADR-0037).** First post-Phase-4 corpus
increment. Authored under the `jason-extended-cognition` designation
(owner-granted, 2026-09-20 session) in response to the owner's three
questions on participant independence. Nothing here changes how qiven-context
runs today; code consequences are a proposed Phase 5 (§7), not landed
semantics.

---

## 1. The question this document answers

The v3 corpus fixed what participants ARE at runtime — a pointer graph with
O(1) rebinds (R3), an LLM structurally blind to Device/Human/Client (R5),
bindings resolved live (DR-008). It never stated the participant TYPOLOGY
itself: what a role is ontologically, who may create one, what each
participant class produces / consumes / authorizes, and which dependency
edges are lawful. The appearance of `jason-extended-cognition` — a third
role-shaped thing that is neither brother nor worker and does not fit the
closed `Role { Owner, Brother, Worker }` enum — exposed the gap.

The owner's three questions (2026-09-20), restated precisely:

- **Q1.** Are roles (brother / worker / extended-cognition) abstract
  authority objects granted by the human to an LLM? Can a role become fully
  independent — of device, client tool, LLM, remote? Who may create a role:
  the LLM or the human?
- **Q2.** Workflows (now `views/workflows/` in the canonical repository)
  were polished against a concrete LLM + role + device + client tool. Do they
  therefore depend on nearly all participants?
- **Q3.** Should we audit every participant for independence, dependency,
  production, consumption, and authority?

Short answers: **(1)** a role is a participant-independent *specification*
that is never participant-independent *in exercise*; roles are proposed by
whoever can design and ratified only by the owner; **(2)** yes as
historically accreted, no as a lawful end-state — workflows must be layered
projections with declared dependencies; **(3)** yes, and the audit is §5's
matrix plus a scoped invariant set, not a one-time review.

## 2. The three things "role" was conflating

Canonical practice already uses three distinct constructs that no type ever
separated:

1. **Canonical role** — an authority package: owner / brother / worker.
   Semantics are authorities, duties, and prohibitions, defined "independent
   of any model, product, or client tool" (`collaboration/operating-contract.md`).
2. **Session designation** — `jason-extended-cognition`: an owner-granted,
   session-scoped grant that widens the PROCESS surface (may bypass the
   brother/worker partition) while changing no authority topology.
3. **Binding** — which model instance fulfills a role through which
   client-tool capability class (ADR-0035; qualification-tracked,
   `verify_live`, disclosure duty).

The draft types only (1) — as a closed enum — and half of (3)
(`ParticipantBinding`). Construct (2) exists only in canonical prose and in
commit-trailer vocabulary. The conflation has a cost: every future
designation either gets force-fitted into the enum (freezing a session
adaptation into permanent authority topology) or stays untyped (grants that
the loop cannot audit). DR-017 separates them.

## 3. Q1 — Role independence and creation authority

### 3.1 The independence theorem (and its limit)

**Position.** A role is participant-independent *as a specification* and
cannot be *in exercise*, and both halves are load-bearing:

- **Specification independence (achievable, already true in canonical
  prose, to be typed here).** A `RoleSpec` references only authority
  classes, duties, and capability CLASSES. "Brother authors and reviews" is
  true on any model, any tool, any device. No field of a role's meaning may
  name a model instance, a client tool, a device, or a remote.
- **Exercise dependence (irreducible).** Every exercise of a role lands on
  a concrete binding: some model instance, through some client tool, on some
  device. The system's obligation is not to eliminate the landing but to
  RECORD it — ADR-0035 rule 4 exists precisely because the landing matters
  and must be disclosed, never silently assumed.

Interface versus implementation. The recurring failure mode this split
prevents: role semantics silently absorbing properties of whatever instance
happened to exercise them longest (the workflow-accretion problem of Q2 is
the same disease in a different artifact).

Two irreducible dependencies survive at the exercise layer and must be
stated, not hidden:

- **Identity always arrives from the human plane.** An LLM cannot
  self-provision an SSH/GitHub identity (operating contract, commit
  identity attribution). Every LLM-exercised role structurally depends on a
  human principal's authentication for its Git/publishing reach.
- **The owner role is fulfilled by a human.** Roles are not LLM-only
  constructs; `Owner`'s capability floor is "verified by the identity port",
  which today means the human principal. Role theory must therefore describe
  fulfillment by any participant class whose capabilities satisfy the floor
  and whose grant is lawful — not "roles for LLMs".

Evidence from scars that the split is the correct one:

- **2026-09-16 split-brain.** Two concurrent flows shared one DCR
  capability. Role semantics were never in question; EXECUTION authority
  needed its own plane (ADR-0026 Host broker). The incident taught that
  execution authority is a plane, not a role attribute — a role spec that
  encoded "may execute on JasonPC" would have been wrong at the
  specification level.
- **Flash substitution under GLM-5.3 provider failures**
  (`jason-worker-glm5-3-flash` qualification): the role did not change; the
  binding and its disclosure did. The system survived an instance swap with
  zero role-semantic churn — specification independence working as designed.

### 3.2 Capability floors

Each canonical role carries a `CapabilityRequirements` floor so that "can
this binding exercise this role?" is checkable rather than vibes:

```cpp
struct CapabilityRequirements {   // CLASSES, never instances
                                  // (pit: environment profiles carry families
                                  //  only; exact values are verify-live)
    bool reviewGradeReasoning;    // brother floor: ADR-0035 qualification
    CapabilityClass execution;    // worker: LocalSupervised | HostBrokered | None
    bool identityPortVerified;    // owner: human principal authentication
};
```

The floor is the honest formalization of "jason-brother presumes review-grade
model reasoning capability" and "jason-worker requires local execution".
Note what the floor does NOT contain: any device, any tool product, any
model id, any environment fact. "MSVC/CMake builds, CUDA/GPU work" in the
operating contract's worker description is an INSTANCE family of the
local-execution capability class, not part of the role.

### 3.3 Who creates roles

**Position.** Creation authority is two-tier, and the tier is exactly the
canonical-role / session-designation split:

- **Canonical role creation or amendment is a governance mutation** —
  owner (root principal) + H2 + ADR, no verbal waiver (ADR-0036 table).
  An LLM may PROPOSE a `RoleSpec` — author the authorities, duties, floor,
  and argue the case — but never ratify one. Reason: the role set defines
  what "authorized" means; a participant cannot enlarge the space of
  authorities it may hold. This is the same class of rule as the
  owner-reserved qualification upgrade (reviewer self-certification ban).
- **Session designations are owner-granted, per session, never
  self-assigned** — already the letter of the extended-cognition view. A
  designation may widen PROCESS convenience but carries a structural
  invariant: **authority delta = ∅**. It may bypass role-stage handoffs
  inside the draft's process scaffolding; it may never bypass H1-H4,
  governance rows, or merge-class gates.
- **Bindings create no authority at all.** Declaring a binding names an
  IMPLEMENTATION. Qualification evidence makes it trustworthy, not
  powerful.

**Position (open vs closed role set).** The canonical role set stays CLOSED
at the type level: an enum plus a cognition-resident `RoleSpec` registry,
widened only by governance transaction + code change. Designations are the
open, rate-limited escape hatch. Rejected: runtime-mintable free-form
`RoleId` — that is authority forgery with extra steps; the 2026-09-20 audit
(finding 1) already caught publication sweeping unaccepted commits, and an
open role mint would multiply exactly that failure class.

### 3.4 Typed shape (proposal; lands in Phase 5 if accepted)

```cpp
struct RoleSpec {                 // IN the tree (R4: authority rules are
    Role id;                      //  cognition data; PolicyTable-adjacent)
    AuthoritySet authorities;     // operation classes this role may perform
    DutySet duties;               // disclosure duty, review duty, worker
                                  // discipline, escalation-on-doubt...
    CapabilityRequirements floor; // what any binding must satisfy
    RatificationRef provenance;   // ADR + H2 evidence that minted/amended it
};

struct SessionDesignation {       // RUNTIME sidecar, not cognition (R1/R3);
    std::string id;               // "jason-extended-cognition"
    Role base;                    // adapts brother-class authoring
    GrantRef grant;               // per-session owner grant; never self-assigned
    std::vector<ProcessBypass> bypasses;  // e.g. brother/worker partition
    // INVARIANT (test-enforced): authorities(designation) ==
    //                        authorities(base role); delta == ∅
};

struct AuthenticatedActor {       // extended: the actor now carries all
    PrincipalId principal;        // three constructs it actually has
    Role role;
    std::optional<SessionDesignation> designation;
    BindingId binding;
    ServingDisclosure disclosure; // ADR-0035 r4: declared vs served
};
```

The declaration/grant split mirrors DR-008's durable-vs-runtime rule: the
EXISTENCE and RULES of a designation may be context (the view registration
of extended-cognition is a published record), while each session's grant is
runtime sidecar state.

## 4. Q2 — Workflow dependency layering

**Diagnosis.** Yes: as historically authored, a workflow document depends on
nearly every participant axis — because it ACCRETED four different layers in
one file. That is lawful view material but an unlawful end-state: the
dependency is real, undeclared, and unauditied.

**Position.** Every sentence in a workflow document belongs to exactly one
of four layers, classifiable by a substitution test — *"does this sentence
survive replacing the concrete tool / model / device with another of the
same class?"*

| Layer | Content | Depends on | Lawful home |
| --- | --- | --- | --- |
| **L0 canonical process law** | exact-head discipline, fail-fast gates, H1-H4, no-verbal-waiver, bounded waits, no sleep/poll, truthful liveness, gate-before-publication | nothing participant-shaped | `collaboration/` (canonical) |
| **L1 capability-class procedure** | how a `local_supervised_agent` discovers and invokes the Operator; `--json` vs `--verbose` views; machine-JSON spill semantics | capability CLASS only | `views/workflows/`, class-named docs |
| **L2 role choreography** | brother authors / worker validates-and-publishes; gate failure is a handoff trigger; delegated batch review | the role partition (instances never) | process law (canonical); views only instantiate it |
| **L3 environment & human ergonomics** | Windows path boundary, Git Bash `/tmp` invisibility, CMD `&&` semantics, owner-legible `--verbose` | concrete device/tool/human preferences | view + environment profile, `verify_live` |

The 2026-09-20 views restructure began this (renaming to the capability-class
`supervised-agent.md`); this section is the rule that finishes it.

**Forbidden dependency edges** (a workflow profile may never have them):

- workflow → concrete model instance (never lawful);
- workflow → a named human beyond a preference profile reference (never
  lawful);
- workflow → authority PRODUCTION (workflows consume policy and choreograph
  its gates; they never mint, waive, or reinterpret authority — the
  no-verbal-waiver principle applied to artifacts).

**Formalization for the draft.** A workflow profile is a DERIVED artifact:
a read-time pure projection of `(policy, role partition) × capability class
× environment class × human preferences` — same shape as view resolution
(S1-R2): resolution never grants authority, and an unresolvable input falls
back to the identity-independent path, never an invented adaptation. A
workflow doc that RESTATES L0/L2 instead of referencing it is duplication
drift: when canonical law changes, copies silently diverge (the exact
stale-reference class the 2026-09-20 restructure had to clean up).

## 5. Q3 — The participant dependency matrix

The audit the owner asked for, as corpus law rather than a one-time review.

### 5.1 Matrix

| Participant class | Produces | Consumes | May authorize | Structurally independent of |
| --- | --- | --- | --- | --- |
| **Human principal** (root, `github:JasonHuang3D`) | intent; grants H1-H4; ratifications; preferences | reports; review deltas; checkpoints | ALL authority — it is the source | nothing; it IS the governance root |
| **Canonical role** (spec) | authority semantics as data | — | nothing by itself; exercised only through an actor | every instance of every kind |
| **Session designation** | process-surface widening | an owner grant | ∅ (invariant: authority delta = ∅) | — |
| **LLM instance** | proposals; transactions; reviews; attribution-bearing commits | ContextBundle (floors + constraints + candidates; `authorization: not_granted`) | nothing — reading never grants | device / human / client tool (R5 blindness) |
| **Client tool** (capability class) | transport; permission surface; relay | — | nothing — the permission surface can BLOCK (safety) but cannot GRANT project authority; relay is not authority | cognition content |
| **Device** (local, e.g. JasonPC) | execution; storage materializations; gate runs | specs; artifacts | nothing — possession ≠ authority (constitution §12); ADR-0026 makes this structural via the broker | governance |
| **Remote device** (GitHub + CI) | canonical published state; CI evidence | pushes; dispatches | authenticates the root principal; remote refs answer "what is published now" (question-scoped, constitution §10) — an authentication and published-state channel, not an independent authority source | cognition semantics |
| **Store / CAS** | snapshots; receipts | transactions | admission only (fencing, quarantine — mechanical safety), never content authority | participants |

Read the matrix column-wise for the two invariants it encodes: authority has
exactly one source (the human principal, exercised through policy), and
every other participant class is a capability, a channel, or a safety
control.

### 5.2 Artifacts in the matrix (non-participant producers/consumers)

- **`RoleSpec`s** — produced by owner ratification of an (often LLM-authored)
  proposal; consumed by the policy table at admission time.
- **Workflow profiles** — co-produced by humans and agents (Q2); consumed by
  session resolution at read time.
- **View declarations / bindings** — produced by view transactions; consumed
  by resolution; create no authority (DR-008, ADR-0035).
- **Checkpoints** — produced by the loop; consumed by continuity; never
  canonical (constitution §5).

### 5.3 Checkable invariants (I-PM set)

Naive independence lints over-fire and then get ignored; scope is the
difference between a real gate and noise. Provenance and evidence surfaces
LAWFULLY name instances (a qualification record that cannot say
"GLM-5.3-Flash" is useless); normative surfaces must not.

- **I-PM1** No NORMATIVE canonical surface (contracts, role specs, policy,
  workflow profiles, requirements) names a concrete model instance, tool
  product, or device. Evidence, audits, qualification records, and state
  are exempt — they record what happened. *[repo lint, scoped]*
- **I-PM2** Every authority grant traces to principal + policy table
  (DR-003 corollary). *[admission-time check]*
- **I-PM3** Authority delta of any session designation = ∅. *[designation
  construction test — Phase 5]*
- **I-PM4** Workflow profiles reference capability classes and environment
  classes, never tool/model instances. *[repo lint, scoped like I-PM1]*
- **I-PM5** Serving-model disclosure completeness: every LLM-exercised
  transaction carries declared-vs-served disclosure (ADR-0035 r4). The
  commit-attribution trailer is the Git-plane instance of this invariant;
  its 2026-09-20 position regression (role block ahead of the subject) is a
  live demonstration that disclosure ARTIFACTS need position discipline too,
  or the summary surface degrades to noise. *[transaction schema + lint]*
- **I-PM6** LLM-side blindness (R5): the LLM-facing bundle schema contains
  no device/human/client fields. *[bundle schema test]*

## 6. What this resolves and what it deliberately does not

Resolves: the ontology gap that made `jason-extended-cognition`
unrepresentable; the creation-authority question (propose vs ratify); the
workflow dependency question (layered projections, declared homes, forbidden
edges); the audit question (matrix + I-PM set with honest scoping).

Deliberately does not: widen the canonical role set; land any code; touch
the chatgpt-jason / zcode-jason views; change ADR-0035/0036/0037 semantics
(it refines their shared substrate). The draft-vs-canonical boundary is
unchanged: canonical prose contracts remain normative until
`OBL-20260918T215000Z-B4D6A8` validation upgrades this corpus.

## 7. Proposed Phase 5 (NOT accepted; pending owner review)

- `RoleSpec` registry as cognition data (PolicyTable-adjacent), with
  provenance; `Role` enum stays closed.
- `SessionDesignation` runtime-sidecar type with the authority-delta-∅
  construction test; `AuthenticatedActor` gains the optional designation.
- Workflow profile layer/kind metadata on `ViewSpec` profile refs.
- I-PM scoped lints (I-PM1/I-PM4) and the I-PM6 bundle blindness test.

Proof obligations if accepted: designation delta test green; I-PM lints
green on the canonical repositories (after their L3 sentences are either
reclassified or exempted as evidence); bundle blindness test; full default
gate at the exact head.
