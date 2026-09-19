# Requirements — What qiven-context Actually Needs When It Is Used

**Status: candidate design input (ADR-0037).**

These requirements are not invented. Each one is derived from observed operation
of `JasonHuang3D/qiven-context` — the contracts it enforces daily, the incidents
it survived, and the obligations it tracks. Every requirement cites its
operational evidence. The document ends with a coverage verdict: where draft v2
stands today, where the v3 design lands, and which residue is judgment-only.

Evidence citations use the canonical repository's identifiers: `MEM-*` (memory
records), `ADR-*` (decisions), `OBL-*` (obligations), contract file paths, and
dated incidents recorded in `evidence/audits/`.

---

## S1 — Cold boot: a fresh consumer reconstructs the project

The defining scenario. A fresh agent and/or fresh human, with no prior
conversation and no model-native memory, reconstructs enough project cognition to
work correctly (BOOTSTRAP protocol, 17 steps).

- **S1-R1** Boot must read an exact canonical ref; local copies are working
  materializations, never authority. *(Constitution §10, authority.yaml)*
- **S1-R2** Boot must resolve the applicable participant view or fall back to the
  identity-independent context — never invent or assume a participant combination.
  *(BOOTSTRAP step 10 rule, added 2026-09-18)*
- **S1-R3** Boot must load a mandatory input set: governance, constitution,
  operating contracts, compact state, active work, roadmap, latest session
  checkpoint, open obligations. *(BOOTSTRAP steps 2–11; R1 mandatory-input rules)*
- **S1-R4** Live facts (remote refs, CI, tool versions, VPN, serving model) must
  be verified at use time; durable profiles may record families, never cached
  values. *(ADR-0035 verify_live; environment profile verification policy)*
- **S1-R5** Inconsistencies discovered during boot must be reported and
  classified before action — not silently reconciled and not silently worked
  around. *(BOOTSTRAP step 16; constitution §11)*

**Observed failure modes this scenario already produced:** stale `current.md`
"next boundary" paragraph lagging accepted merges (found 2026-09-19 boot); session
checkpoint lagging two canonical merges; a canonical wording conflict between
`state/current.md` and the Human Manual Mode workflow that blocked K4 trial 1.

## S2 — Selective retrieval for a task

Before using project-history facts, a task-specific retrieval runs; its results
are candidate evidence, not truth (constitution §2, §16).

- **S2-R1** Retrieval is invoked at every material domain/task transition — a
  correct engine that is not invoked is a miss. *(constitution §16;
  OBL-20260914T124259Z-7A4D13; the retrieval-reliability branch series)*
- **S2-R2** The bundle carries mandatory inputs and protected constraints
  regardless of budget; budget pressure may shrink only the candidate set.
  *(ADR-0033 §6–8; context-read-contract)*
- **S2-R3** Candidates are typed as candidates, rehydrated from the exact
  snapshot; ranking never becomes truth; derived indexes never become canonical.
  *(constitution §13; ADR-0033 §8)*
- **S2-R4** Unresolved evaluation is three-valued: known / unknown / explicitly
  abstained — never silently guessed. *(ADR-0033 §6)*

## S3 — Material transaction: writing canonical truth

Durable cognition changes land as one coherent transaction per trigger class
(trigger list 1–9 in the operating model).

- **S3-R1** A transaction is atomic across all records it touches; no partial
  snapshot is ever visible. *(ADR-0033 §4; observed: the ADR-0037 transaction
  touched decisions + obligations + view + state + session in one commit)*
- **S3-R2** Writes carry an authenticated actor — never a caller-asserted
  "I am the writer" statement. *(ADR-0026 authority rule; MEM-20260916T042800Z-
  B71E3C)*
- **S3-R3** Commits compare-and-swap on the declared base; a stale base refuses
  with a reason that tells the writer what to do next. *(ADR-0033 §4; K2)*
- **S3-R4** Same-key retries resolve to one durable outcome; a timeout after a
  possibly-successful commit is `outcome_unknown`, never assumed rollback.
  *(ADR-0033 §4; K2 acceptance suite)*
- **S3-R5** Lifecycle transitions (supersede, close, reconcile) are explicit
  operations with reciprocal, acyclic relations — history is never overwritten.
  *(constitution §4; record-lifecycle contract)*
- **S3-R6** Every record carries typed provenance; recording a lesson with empty
  provenance must be refused. *(constitution §9)*

## S4 — Acceptance and merge-class publication

- **S4-R1** Merge-class operations require an H2 review **of the exact delta**;
  evidence must be bound to the delta's content, not a free-text reference.
  *(ADR-0036 H2; MEM-20260913T162546Z-E73124)*
- **S4-R2** The operation-class → required-handoff mapping is policy data,
  interpretable by the gate — not an if-chain in the service. *(ADR-0036
  classification table; R4 "authority rules live in cognition")*
- **S4-R3** Mandatory handoffs have no verbal-waiver path; the waiver attempt
  itself is recordable. *(ADR-0036 no-verbal-waiver protocol)*
- **S4-R4** Reviewer self-certification is structurally impossible: a delegated
  reviewer cannot review a delta authored by the same instance. *(ADR-0035;
  local-supervised-agent workflow, delegated-review section)*
- **S4-R5** Acceptance topologies (H1 producer, isolated fresh consumer) cannot
  be simulated by the authoring session or a child process. *(context-handoff
  contract; ADR-0034)*

## S5 — Session continuity and rollover

- **S5-R1** No fact that must survive the session may exist only in a session
  record; checkpoints are continuity evidence only. *(constitution §5)*
- **S5-R2** Checkpoints are written at turn boundaries, after material
  transactions, at async exit, and at session close — the observed 26-minute
  Chat tool-turn boundary is the budget that makes this non-optional.
  *(MEM-20260915T135800Z-6B0D8A; operating-model trigger list 5–8)*
- **S5-R3** Missing history is recorded as explicit absence, never synthesized.
  *(sessions v2–v5 are missing and must stay missing; checkpoint knownGaps)*

## S6 — Isolated handoff (K4) and transport compression (K5)

- **S6-R1** Artifact handoff is causal: producer → self-describing artifact →
  isolated consumer; a remote cold boot never substitutes for it. *(ADR-0034;
  the pre-correction K4 challenge was rejected for exactly this reason)*
- **S6-R2** Restored instances are quarantined and non-authoritative; promotion
  is a separately governed operation, never a side effect. *(ADR-0033 §10–11)*
- **S6-R3** Corruption, truncation, version mismatch and resource abuse fail
  closed during restore. *(context-handoff contract failure conditions)*
- **S6-R4** K5 compression is lossless at the semantic-closure level; byte
  compression is not token compression; decoder material travels with the
  artifact; measurement names the tokenizer. *(ADR-0034 K5 section)*

## S7 — Authority and participant change

- **S7-R1** Single-writer over the canonical chain is established by an
  authority mechanism (lease/broker), not by conversational singularity or
  transport reachability; a second competing flow is refused fail-closed, not
  queued. *(ADR-0026; the 2026-09-16 split-brain incident; MEM-20260915T163500Z-
  5E7A91)*
- **S7-R2** Authority transitions persist even when the triggering request is
  rejected. *(MEM-20260916T040700Z-D8A4C2)*
- **S7-R3** Participant change (model, tool, device, human) is an O(1) runtime
  rebind, never a cognition write; the binding's qualification evidence enters
  cognition only through normal transactions. *(R3; ADR-0035)*
- **S7-R4** Serving-model substitution is disclosed, never silent. *(ADR-0035
  rule 4; observed twice: GLM-5.3 provider failures during the v8 session, again
  on 2026-09-19)*
- **S7-R5** Unattended automation is read-only by default. *(ADR-0036)*

## S8 — Conflict and lifecycle

- **S8-R1** A discovered contradiction is preserved, then reconciled; while
  unresolved it is represented explicitly — and **acceptance-class operations
  are refused while a resolvable open conflict intersects their scope**.
  *(constitution §11; K4 trial 1 was blocked for exactly this)*
- **S8-R2** State surfaces must be mutually coherent (objective, checkpoint,
  candidate, next boundary reference real records). *(observed drift class)*

## S9 — Failure and recovery

Every failure class names its recovery path — the LLM's natural failure response
(retry / silently weaken / summarize away) is precisely what the system must
prevent.

- **S9-R1** Stale thinking → park the delta, re-read, re-think; never
  blind-retry the write. *(runtime.cpp v2 comment made normative)*
- **S9-R2** Missing handoff evidence → halt and escalate; no re-attempt loop.
- **S9-R3** Connector/transport refusal → state the boundary, preserve intended
  semantics, switch authoring path (operator-first → trusted local path →
  exact generated patch). *(MEM-20260915T203423Z-C4A912)*
- **S9-R4** Gate failure after authoring → a handoff trigger to the authoring
  role, not a worker-side fix opportunity. *(execution-stage granularity,
  2026-09-18 owner direction)*
- **S9-R5** Provider unavailability → bounded wait, disclosure, no retry loops.
  *(bounded-wait rule; v8 session record)*
- **S9-R6** Invariant failure in a known hazard class → design review, not
  mechanical patch/CI repetition. *(constitution §17)*

## S10 — Governance evolution: rules change under rules

- **S10-R1** Governance and policy amendments are themselves gated operations
  authorized by the *old* policy. *(ADR-0033 §4)*
- **S10-R2** A working mechanism is not an accepted mechanism; candidate
  semantics stay quarantined from production until formal validation.
  *(ADR-0037; the K4 lesson general)*

---

## Coverage verdict

| Surface | Draft v2 today | v3 design (this corpus) | Residue |
| --- | --- | --- | --- |
| S1 cold boot | partial: genesis/restore, no view resolution, no mandatory floors | typed mandatory inputs, view compilation, boot-time consistency classification | relevance judgment stays human/LLM judgment |
| S2 retrieval | **absent** (query ignored, full-tree read) | ContextBundle with floors/constraints/candidates/omissions | ranking quality stays a measured property |
| S3 transactions | partial: single-op deltas, CAS, no atomicity, no idempotency | atomic multi-op transactions, receipts, outcome_unknown | — |
| S4 acceptance | partial: H2 string evidence, if-chain policy | content-bound evidence, policy table, self-certification-safe review records | review *quality* stays judgment |
| S5 sessions | sidecar exists, no triggers, no gaps typing | checkpoint triggers, EvidenceGap, turn budget | — |
| S6 handoff/K4/K5 | partial: quarantine, inline artifact restore | restore fail-closed typed errors, promotion state machine, K5 transport stub with measurement contract | trial execution stays a real topology |
| S7 authority | **wrong where present** (caller-asserted writes, epoch over-fencing) | actor port, grants/leases, fail-closed refusal reasons, disclosure types | — |
| S8 conflict | **absent** | Conflict records + acceptance-blocking gate | "evidence genuinely insufficient" stays judgment |
| S9 recovery | one path (parked deltas) | typed refusal reason → mandated next action table | — |
| S10 governance | genesis bakes identity; no amendment ops | governed amendment operations; policy versioning | — |

**Honest verdict:** draft v2 encodes roughly the data model plus the three
easiest gates (~20% of the observed requirement surface, weighted by incident
cost). The v3 design in this corpus closes the structural gaps; what remains is
explicitly listed as judgment-only in `invariant-inventory.md` rather than
pretended away.
