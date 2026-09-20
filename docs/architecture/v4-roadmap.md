# v4 Roadmap — Activation: Order of Landing and Proof

**Status: candidate design input.** Derives from
`cognitive-boundary-model.md` §42 (landing discipline) and the ranked
backlog in `activation-failure-inventory.md` §5. Nothing changes how the
canonical repository runs until each phase's guards and proofs land with
it. Standing rules: deterministic triggers before semantic machinery (no
recorded scar has yet demanded semantic retrieval); every guard lands WITH
its pit row; publication boundaries follow P-53 merge-proof.

---

## Phase v4.0 — Boundary model frozen ✅

**Deliverable.** The owner-authored seed (`cognitive-boundary-model.md`)
adopted verbatim; the first narrower seed withdrawn in review and retained
as superseded evidence.

**Proof.** Owner review 2026-09-20; corpus adoption (PR #19 `a087a46`).

## Phase v4.1 — First deterministic triggers ✅

**Deliverable.** The two highest-recurrence guards from the inventory:
P-51 attribution subject-position lint; P-52 reference-integrity sweep; the
pit map's v4 activation ledger born with them.

**Proof.** P-51 selftest + HEAD-ancestry scan clean on all repositories;
P-52 vacuity-proven (the exact A4 path fails the gate); both wired into
real gates (devkit `local`; context `context-local`/`context-docs`).

## Phase v4.2 — Publish-boundary guard ✅

**Deliverable.** P-53: operator gate receipts for exact heads + the
`gate_proof` builtin; the supervised-agent publish-boundary rule (missing
receipt = stop).

**Proof.** Fail-closed vacuity proven in devkit AND context: a new head
without a receipt refuses merge-proof; a fresh gate PASS at that head
unblocks it. Devkit self-host (146bc63) + context snapshot (4f5cdef).

## Phase v4.3 — Control-plane types ✅

**Deliverable.** The seed's §27 vocabulary as draft C++ types with
serialization: `ClaimClass`, `ActionKind`, `ActionIntent`,
`CognitiveRequirement`/`RequirementKind`, `InvocationPolicy` (as cognition
data, PolicyTable-adjacent), `PreparationPacket`; the discretionary
`CognitiveNeed` request path. Minimal wiring: an intent → policy lookup →
requirements derivation demo covering the naming-convention scenario
(seed §29).

**Proof.** LANDED 2026-09-20 (`03abf0d`, gate + merge-proof PASS):
`tests/control_plane.cpp` executes the §29 naming scenario from the
snapshot only; the §41 transport miniature (restored generation derives
identical preparation); fail-closed unresolved recall; listed action-class
demands; purity. `default_invocation_policy()` encodes P-51/52/53 and the
structural boundaries as policy data. Serialization v7, golden re-pinned.

## Phase v4.4 — Remaining high-value triggers ✅

**Deliverable.** A6 tool-contract invocation routing (Operator-declared
argv drives invocation; guessing forbidden where a contract exists);
A7 failure-fingerprint + batched evidence (first material failure captures
fingerprint + retrieves related pits; repeated identical signature refuses
blind retry).

**Proof.** LANDED 2026-09-20 (`tests/tool_and_retry.cpp`): A6 — the
contract constructs the argv; three guessed-variant shapes (wrong layout,
typo flag, renamed operation) all FAIL validation. A7 — normalization
strips timestamps/temp paths; the fingerprint recalls the recorded
odyssey pit; the equivalent retry without new evidence is REFUSED
(V4-R3), with evidence or a different operation permitted.

## Phase v4.5 — Creation-boundary activation ✅

**Deliverable.** A2 residual: naming-policy activation at identifier
creation inside working sessions (not just repo entry); A8: eligible-
lower-layer search before reusable-primitive authoring (dependency graph +
symbol search machinery — the largest item).

**Proof.** LANDED 2026-09-20: scenario 1 shipped with v4.3
(`control_plane.cpp` naming case + transport miniature); scenario 2 in
miniature now too — `primitive_judgment_authorized` refuses an
IntroducePrimitive judgment whose search never ran, and the report
surfaces the existing `qiven::fnv1a64` for judgment without deciding
reuse. The OPERATIONAL search machinery (real symbol/dependency index)
remains execution-time mechanism work outside the draft, per seed §36.

## Phase v4.6 — v4 validation (compiled proofs complete; H1 boundary reached)

**Deliverable.** The §41 nine proof obligations executed and recorded;
delivery profile named per obligation. H1 boundaries are EXPECTED here
(fresh-consumer trials, any acceptance-producer roles) — the standing
long-running window stops for them by definition.

**Proof.** Dated validation report binding draft commit identity to the
executed scenarios; canonical recording through the normal transaction
path; only then do v4 semantics feed K5's transport design.

---

## Sequencing rules

- Deterministic before semantic: no semantic-retrieval machinery is built
  until a recorded scar demands it (none has).
- Every guard lands with its pit row (check_pit_map consistency).
- Every phase's final exact head carries a full-gate PASS receipt
  (P-53 applies to v4's own publication).
- The hidden interior of reasoning stays opaque; v4 types only the
  boundaries (seed §23) — any proposal that types thought interiors is
  rejected at design review, not implemented.
