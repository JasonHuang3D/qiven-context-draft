# Participant Model — Independence, Dependency, and Authority

**Status: frozen semantic contract layer (revised subset, 2026-09-26 PR4
doc-repair).** The original v3 design-input version is preserved verbatim at
`legacy/participant-model-v3-design-input.md`. This live path keeps the
still-valid identity/authority semantics — the roles/designations/bindings
separation (§2), capability floors and creation authority (§3), the workflow
dependency layering (§4, with its L2 role-choreography row superseded by
ADR-0044), the participant dependency matrix and I-PM invariants (§5). Under
ADR-0044, roles are attribution and qualification labels exercised by one
capable owner-supervised session — never behavioral stages; canonical
authority and handoffs live in `JasonHuang3D/qiven-context` (ADR-0035/0036/
0044).

---

## 1. The question this document answers

The v3 corpus fixed what participants ARE at runtime — a pointer graph with
O(1) rebinds (R3), an LLM structurally blind to Device/Human/Client (R5),
bindings resolved live (DR-008). It never stated the participant TYPOLOGY
itself: what a role is ontologically, who may create one, what each
participant class produces / consumes / authorizes, and which dependency
edges are lawful. The appearance of `jason-extended-cognition` — a third
role-shaped thing that does not fit the closed `Role { Owner, Brother,
Worker }` enum — exposed the gap.

The owner's three questions (2026-09-20), restated precisely:

- **Q1.** Are roles abstract authority objects granted by the human to an
  LLM? Can a role become fully independent — of device, client tool, LLM,
  remote? Who may create a role: the LLM or the human?
- **Q2.** Do workflows depend on nearly all participants?
- **Q3.** Should we audit every participant for independence, dependency,
  production, consumption, and authority?

Short answers: **(1)** a role is a participant-independent *specification*
that is never participant-independent *in exercise*; roles are proposed by
whoever can design and ratified only by the owner; **(2)** yes as
historically accreted, no as a lawful end-state — workflows must be layered
projections with declared dependencies; **(3)** yes, and the audit is §5's
matrix plus a scoped invariant set, not a one-time review.

## 2. The three things "role" was conflating

Canonical practice uses three distinct constructs:

1. **Canonical role** — an authority/attribution package (owner / brother /
   worker; per ADR-0044 an attribution and qualification label, never a
   behavioral stage).
2. **Session designation** — `jason-extended-cognition`: an owner-granted,
   session-scoped grant that widens the PROCESS surface while changing no
   authority topology.
3. **Binding** — which model instance fulfills a role through which
   client-tool capability class (ADR-0035; qualification-tracked,
   `verify_live`, disclosure duty).

## 3. Q1 — Role independence and creation authority

### 3.1 The independence theorem (and its limit)

**Position.** A role is participant-independent *as a specification* and
cannot be *in exercise*, and both halves are load-bearing:

- **Specification independence (achievable).** A `RoleSpec` references only
  authority classes, duties, and capability CLASSES. No field of a role's
  meaning may name a model instance, a client tool, a device, or a remote.
- **Exercise dependence (irreducible).** Every exercise of a role lands on a
  concrete binding: some model instance, through some client tool, on some
  device. The system's obligation is not to eliminate the landing but to
  RECORD it — ADR-0035 rule 4 exists precisely because the landing matters
  and must be disclosed, never silently assumed.

Two irreducible dependencies survive at the exercise layer and must be
stated, not hidden:

- **Identity always arrives from the human plane.** An LLM cannot
  self-provision an SSH/GitHub identity; every LLM-exercised role
  structurally depends on a human principal's authentication for its
  Git/publishing reach.
- **The owner role is fulfilled by a human.** `Owner`'s capability floor is
  "verified by the identity port".

### 3.2 Capability floors

Each canonical role carries a `CapabilityRequirements` floor so that "can
this binding exercise this role?" is checkable rather than vibes:

```cpp
struct CapabilityRequirements {   // CLASSES, never instances
    bool reviewGradeReasoning;    // review-grade floor: ADR-0035 qualification
    CapabilityClass execution;    // LocalSupervised | HostBrokered | None
    bool identityPortVerified;    // owner: human principal authentication
};
```

The floor contains no device, tool product, model id, or environment fact.

### 3.3 Who creates roles

**Position.** Creation authority is two-tier, exactly the canonical-role /
session-designation split:

- **Canonical role creation or amendment is a governance mutation** —
  owner (root principal) + H2 + ADR, no verbal waiver (ADR-0036). An LLM
  may PROPOSE a `RoleSpec`, but never ratify one.
- **Session designations are owner-granted, per session, never
  self-assigned** — with the structural invariant: **authority delta = ∅**.
  A designation may widen process convenience; it may never bypass H1-H4,
  governance rows, or merge-class gates.
- **Bindings create no authority at all.** Qualification evidence makes a
  binding trustworthy, not powerful.

**Position (open vs closed role set).** The canonical role set stays CLOSED
at the type level: an enum plus a cognition-resident `RoleSpec` registry,
widened only by governance transaction + code change. Designations are the
open, rate-limited escape hatch. Rejected: runtime-mintable free-form
`RoleId` — authority forgery with extra steps.

### 3.4 Typed shape (design proposal)

```cpp
struct RoleSpec {                 // IN the tree (R4: authority rules are
    Role id;                      //  cognition data; PolicyTable-adjacent)
    AuthoritySet authorities;     // operation classes this role may perform
    DutySet duties;               // disclosure duty, review discipline,
                                  //  escalation-on-doubt...
    CapabilityRequirements floor; // what any binding must satisfy
    RatificationRef provenance;   // ADR + H2 evidence that minted/amended it
};

struct SessionDesignation {       // RUNTIME sidecar, not cognition (R1/R3);
    std::string id;               // "jason-extended-cognition"
    Role base;
    GrantRef grant;               // per-session owner grant; never self-assigned
    std::vector<ProcessBypass> bypasses;
    // INVARIANT (test-enforced): authorities(designation) ==
    //                        authorities(base role); delta == ∅
};

struct AuthenticatedActor {       // the actor carries all three constructs
    PrincipalId principal;
    Role role;
    std::optional<SessionDesignation> designation;
    BindingId binding;
    ServingDisclosure disclosure; // ADR-0035 r4: declared vs served
};
```

## 4. Q2 — Workflow dependency layering

**Position.** Every sentence in a workflow document belongs to exactly one
of four layers, classifiable by a substitution test — *"does this sentence
survive replacing the concrete tool / model / device with another of the
same class?"*

| Layer | Content | Depends on | Lawful home |
| --- | --- | --- | --- |
| **L0 canonical process law** | exact-head discipline, fail-fast gates, H1-H4, no-verbal-waiver, bounded waits, no sleep/poll, truthful liveness, gate-before-publication | nothing participant-shaped | `collaboration/` (canonical) |
| **L1 capability-class procedure** | how a `local_supervised_agent` discovers and invokes the Operator; `--json` vs `--verbose` views; machine-JSON spill semantics | capability CLASS only | `views/workflows/`, class-named docs |
| **L2 execution choreography** | *(superseded as role staging by ADR-0044: one owner-supervised session performs the full loop; the former brother/worker partition survives only as session duties — exact-delta review, publication discipline, escalation)* | the accepted process law | `collaboration/` (canonical); views only instantiate it |
| **L3 environment & human ergonomics** | Windows path boundary, Git Bash `/tmp` invisibility, CMD `&&` semantics, owner-legible `--verbose` | concrete device/tool/human preferences | view + environment profile, `verify_live` |

**Forbidden dependency edges** (a workflow profile may never have them):

- workflow → concrete model instance (never lawful);
- workflow → a named human beyond a preference profile reference (never
  lawful);
- workflow → authority PRODUCTION (workflows consume policy and choreograph
  its gates; they never mint, waive, or reinterpret authority).

**Formalization.** A workflow profile is a DERIVED artifact: a read-time
pure projection of policy × capability class × environment class × human
preferences — resolution never grants authority, and an unresolvable input
falls back to the identity-independent path, never an invented adaptation.

## 5. Q3 — The participant dependency matrix

### 5.1 Matrix

| Participant class | Produces | Consumes | May authorize | Structurally independent of |
| --- | --- | --- | --- | --- |
| **Human principal** (root, `github:JasonHuang3D`) | intent; grants H1-H4; ratifications; preferences | reports; review deltas; checkpoints | ALL authority — it is the source | nothing; it IS the governance root |
| **Canonical role** (spec) | authority semantics as data | — | nothing by itself; exercised only through an actor | every instance of every kind |
| **Session designation** | process-surface widening | an owner grant | ∅ (invariant: authority delta = ∅) | — |
| **LLM instance** | proposals; transactions; reviews; attribution-bearing commits | ContextBundle (floors + constraints + candidates; `authorization: not_granted`) | nothing — reading never grants | device / human / client tool (R5 blindness) |
| **Client tool** (capability class) | transport; permission surface; relay | — | nothing — the permission surface can BLOCK (safety) but cannot GRANT project authority; relay is not authority | cognition content |
| **Device** (local, e.g. JasonPC) | execution; storage materializations; gate runs | specs; artifacts | nothing — possession ≠ authority (constitution §12) | governance |
| **Remote device** (GitHub + CI) | canonical published state; CI evidence | pushes; dispatches | authenticates the root principal; remote refs answer "what is published now" (question-scoped, constitution §10) — an authentication and published-state channel, not an independent authority source | cognition semantics |
| **Store / CAS** | snapshots; receipts | transactions | admission only (fencing, quarantine — mechanical safety), never content authority | participants |

Read the matrix column-wise for the two invariants it encodes: authority has
exactly one source (the human principal, exercised through policy), and
every other participant class is a capability, a channel, or a safety
control.

### 5.2 Artifacts in the matrix (non-participant producers/consumers)

- **`RoleSpec`s** — produced by owner ratification of an (often LLM-authored)
  proposal; consumed by the policy table at admission time.
- **Workflow profiles** — co-produced by humans and agents; consumed by
  session resolution at read time.
- **View declarations / bindings** — produced by view transactions; consumed by
  resolution; create no authority (DR-008, ADR-0035).
- **Checkpoints** — produced by the loop; consumed by continuity; never
  canonical (constitution §5).

### 5.3 Checkable invariants (I-PM set)

Provenance and evidence surfaces LAWFULLY name instances; normative surfaces
must not.

- **I-PM1** No NORMATIVE canonical surface (contracts, role specs, policy,
  workflow profiles, requirements) names a concrete model instance, tool
  product, or device. Evidence, audits, qualification records, and state
  are exempt. *[repo lint, scoped]*
- **I-PM2** Every authority grant traces to principal + policy table
  (DR-003 corollary). *[admission-time check]*
- **I-PM3** Authority delta of any session designation = ∅. *[designation
  construction test]*
- **I-PM4** Workflow profiles reference capability classes and environment
  classes, never tool/model instances. *[repo lint, scoped like I-PM1]*
- **I-PM5** Serving-model disclosure completeness: every LLM-exercised
  transaction carries declared-vs-served disclosure (ADR-0035 r4). *[transaction schema + lint]*
- **I-PM6** LLM-side blindness (R5): the LLM-facing bundle schema contains
  no device/human/client fields. *[bundle schema test]*

## 6. What this resolves and what it deliberately does not

Resolves: the ontology gap that made `jason-extended-cognition`
unrepresentable; the creation-authority question (propose vs ratify); the
workflow dependency question (layered projections, declared homes, forbidden
edges); the audit question (matrix + I-PM set with honest scoping).

Deliberately does not: widen the canonical role set; touch the
chatgpt-jason / zcode-jason views; change ADR-0035/0036/0037 semantics.

## 7. Phase 5 typed landing (historical status)

The original Phase-5 landing list (RoleSpec registry, SessionDesignation
sidecar type with the authority-delta-∅ construction test, workflow profile
layer metadata, I-PM scoped lints) was accepted as DIRECTION 2026-09-20 and
the draft froze before the full typed landing; what exists in headers today
is the compiled contract the validation report pins at `4cbc995`. Further
typed landing, if any, is Runtime-program work under a fresh decision — not
an open draft batch.
