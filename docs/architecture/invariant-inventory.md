# Invariant Inventory — Compiled, Compilable, Judgment-Only

**Status: candidate design input (ADR-0037).** The project's validation authority
already enforces a large invariant set over the canonical repository (record
schemas, lifecycle coherence, legacy write bans, session separation, view
reference integrity, governance presence…). The draft's purpose is to give the
mechanical subset a compiled home — and to state honestly which invariants will
*never* compile. This inventory is the master list; it is also the working
checklist for `OBL-20260918T215000Z-B4D6A8`.

Classes:
- **C** — compiled: a type, gate or test exists (or lands with a v3 phase) and
  its absence is a compile/test failure.
- **K** — compilable: mechanically expressible; not yet scheduled in detail
  (design debt, explicitly tracked).
- **J** — judgment-only: requires evaluation the type system cannot perform;
  pinned by decision records, mandated prompts and process gates instead.

---

## 1. Constitution articles (18)

| # | Article | Class | Compiled form |
| --- | --- | --- | --- |
| 1 | Cognition must outlive its participants | C | snapshot chain + store contract; K4 trial topologies (P-13) |
| 2 | Capture conservatively, canonicalize deliberately, retrieve selectively | C | transaction gates + bundle floors (DR-007) |
| 3 | Evidence and interpretation are different | C | `EvidenceRecord` immutable; `EvidenceObject` vs records; epistemic axis (P-35) |
| 4 | History is not overwritten | C | append-only snapshots; lifecycle ops only (P-36) |
| 5 | Session independence is mandatory | C | checkpoint sidecar never canonical; durable-only-in-session detected by review rule (P-17); full detection is K (below) |
| 6 | Unfinished cognition is first-class | C | `Obligation` with trigger kind **and value**; three-valued evaluation |
| 7 | Negative knowledge is first-class | C | `MemoryRecord::NegativeKnowledge`; bundle floors surface relevant rejections |
| 8 | Epistemic types must not be collapsed | C | `EpistemicType` axis; gates separate evidence from acceptance (P-35) |
| 9 | Provenance is required | C | typed `Provenance`; gate refuses unprovenanced records (P-37) |
| 10 | Authority is question-scoped | C | the four planes; policy table per operation class |
| 11 | Conflict is transient epistemic state | C | `Conflict` lifecycle + acceptance-blocking gate (DR-006); "genuinely insufficient" boundary is J |
| 12 | Governance authority is explicit | C | identity port + root principal + policy table (DR-003/005) |
| 13 | Derived indexes are never canonical | C | bundle rehydrates from snapshot; index lag is diagnostic (P-34) |
| 14 | Private does not mean secret store | C | export omits credentials by construction; artifact requirements forbid secrets (K4 §producer 9) |
| 15 | Simplicity must be falsifiable | J | deferral quality is judgment; the *form* (failure signals, revisit triggers) is K via obligation records |
| 16 | Retrieval reliability includes invocation | C | `RetrievalTrigger` tracked state (P-18) |
| 17 | Validation proves a candidate, not an apprenticeship loop | C/J | known-hazard → design review is a recovery row (C); recognizing "known" is J |
| 18 | Continuity is testable | C | K1–K5 acceptance properties; trial topologies typed (P-13) |

## 2. Canonical-repository validation invariants (operating model §Validation)

| Invariant | Class | Compiled form |
| --- | --- | --- |
| Record schemas and required fields | C | typed records; deserialize validation; golden vectors |
| Legacy write bans (ledger/events, evidence/ci…, sessions/legacy) | K | v3 tree simply has no legacy surfaces; the *ban* stays repo-level until kernel import exists |
| Collaboration vs session separation | C | `SessionCheckpoint` sidecar; durable-cognition-only-in-session review rule (K for full detection) |
| Repository-inventory purity (no live refs) | C | `StateView` typed; `pit.no_live_refs_in_state` (P-25) |
| Active-session continuity (checkpoint exists and is current) | C | checkpoint auto-write triggers; staleness check at boot |
| Canonical lifecycle coherence (supersession reciprocal, acyclic) | C | `SupersedeDecision` gate checks reciprocity + acyclicity |
| Governance presence | C | snapshot requires governance; identity port requires principal |
| ContextView reference integrity | C | `ViewSpec` profile refs resolve in-snapshot (DR-008, P-40) |
| Continuity/handoff/succession contracts exist and are referenced | K | K1 import maps contract files to `CanonicalRef`s; presence check compiles then |

## 3. Kernel acceptance properties (K1–K5)

| Property | Class | Compiled form |
| --- | --- | --- |
| K1 versioned serialization, golden vectors, absent/null distinction, round-trip fidelity | C | DR-009 §10 of cognition-model; golden-vector suite |
| K2 atomicity, CAS, idempotency, race/failure injection, durable receipts | C | DR-011; restart-capable adapter is the only durable-receipt claimant |
| K3 bundle floors, protected constraints, unknown semantics, negative fixtures | C | ContextBundle rules; `pit.bundle_floor_survives_budget` |
| K4 causal artifact handoff, isolation, quarantine, integrity | C | quarantine state machine; typed restore errors; trial topology types; the *trial itself* remains a real topology (J by definition — cannot be simulated) |
| K5 lossless machine round-trip + pinned-tokenizer measurement + paired cognitive trials | C/J | machine gates compile (corruption/resource/fail-closed); measurement is a defined procedure; cognitive equivalence trials are real topologies (J by definition) |

## 4. Judgment-only register (with reasons)

These are recorded, gated by process, and pinned by decision records — never
faked as types:

1. **Known-hazard recognition** (§17): deciding that repeated failures are a
   design problem rather than a patching queue. Reason: requires engineering
   judgment over failure *meaning*, not failure *shape*. Pin: recovery row
   routes `InvariantFailed` to review; review quality is the human gate.
2. **Insufficient-evidence conflict** (§11): when a conflict may remain
   unresolved as an explicit open question. Reason: epistemic judgment about
   evidence sufficiency. Pin: `Conflict` status transitions require a resolution
   record; staying open requires justification text (schema-enforced, content
   judged).
3. **Relevance and ranking quality** (§2): what the candidate set should
   contain. Reason: inherently semantic, measured not compiled. Pin: bundle
   floors are compiled; candidate quality is benchmarked (candidate-acceptance
   schema in the canonical repo).
4. **Material-transaction judgment** (operating-model triggers): whether a
   conversational turn changed durable cognition. Reason: judgment about
   meaning of changes. Pin: triggers are typed as a checklist the loop must
   evaluate; the evaluation is prompted, the checklist is compiled.
5. **Spec-readiness for worker handoff** (feature-spec §16 checklist): the
   checklist is compiled as a schema; the readiness call is CTO judgment.
6. **Acceptance trials themselves** (K4 consumer, K5 paired trials, Human
   Succession): require genuinely independent fresh sessions by contract; a
   compiled simulation would be the P-13 mistake. Pin: topology types +
   isolation contracts; execution is always real.
7. **Review quality** (H2 substance): the digest binding proves *that* the
   delta was reviewed; *how well* is judgment. Pin: `ReviewRecord` findings are
   structured; depth is the reviewer's responsibility and the delegated-review
   escalation's subject.

## 5. Rule for future invariants

New invariants enter this inventory with a class before implementation. A "C"
without a test name is a defect; a "J" without a pin (decision record or
process gate) is a defect. The inventory itself is validated: every `P-*` row in
`pit-regression-map.md` must reference an inventory row or a named test, and
vice versa.
