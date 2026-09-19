# v3 Roadmap — Order of Landing and Proof

**Status: candidate design input (ADR-0037).** Scope discipline (engineering
philosophy §3, §8): each phase lands the smallest increment that makes a
recorded failure class mechanically impossible, with its pit tests. Nothing
here changes how the canonical repository runs; the draft proves architecture,
not transport.

---

## Phase 0 — Design corpus (this directory) ✅

**Deliverable.** The eight documents in `docs/architecture/`. The review that
produced them (2026-09-19) found: v2's data model sound; authority/identity
caller-asserted; recovery rules comment-only; conflicts unrepresentable; views
misclassified; retrieval absent; deserialization not corruption-safe in
Release.

**Proof.** Owner review. The corpus is the requirements + semantics baseline;
later phases trace to it.

## Phase 1 — Authority, verdicts, corruption-safety (P0)

**Deliverable.** The write path rebuilt per DR-001/002/003/004/009/010:

- `Snapshot` / `Materialization` split; participants hold `const`.
- `Verdict` with typed reasons; recovery table as cognition data.
- `AuthenticatedActor` port + `ExecutionGrant`; admission mutex subsumed.
- `H2Evidence` content binding; policy table replaces the H2 if-chain.
- Typed deserialization (`expected<Snapshot, DeserializeError>`) + golden
  vectors.

**Tests (new).** `pit.split_brain_second_flow_refused`,
`pit.unverified_actor_refused`, `pit.rejected_flow_stays_fenced`,
`pit.h2_evidence_rebind_refused`, `pit.handoff_missing_never_retried`,
`pit.reader_boot_preserves_writer`, `pit.corrupt_artifact_fails_closed`,
`pit.resource_abuse_fails_closed`, `pit.release_deserialize_safe`.

**Acceptance.** All v2 suites still green (behavior preserved where v2 was
right); new gates demonstrated in both Debug and Release; the value-tree audit
extended to the new member set.

## Phase 2 — Transactions, conflicts, views, bundles (P1)

**Deliverable.** DR-005/006/007/008/011 + the v3 member set:

- Multi-op atomic `ContextTransaction` with idempotency keys and receipts.
- `Conflict` records + acceptance-blocking gate; `ResolveConflict` op.
- Lifecycle operations (`SupersedeDecision` reciprocal/acyclic gate,
  `TransitionMemory`, `TransitionObligation` with trigger values,
  `AddEvidence`).
- `ViewSpec` in tree; resolution with identity-independent fallback.
- `ContextBundle` with floors/constraints/candidates/omissions;
  `RetrievalTrigger`; `OutputView` split. K5 stays transport-only.
- Typed `StateView` + coherence invariants.
- K2-shaped fixtures: race two writers on one base; inject pre-commit failure
  and post-commit/timeout; memory adapter upgraded to a restart-capable test
  adapter for receipt durability.

**Tests (new).** `pit.open_conflict_blocks_acceptance`,
`pit.partial_transaction_never_visible`,
`pit.timeout_yields_unknown_not_rollback`,
`pit.same_key_different_content_refused`, `pit.history_append_only`,
`pit.unprovenanced_record_refused`, `pit.export_carries_view_specs`,
`pit.view_refs_resolve`, `pit.bundle_floor_survives_budget`,
`pit.candidate_not_truth`, `pit.state_references_resolve`,
`pit.governance_amendable`, `pit.observation_not_invariant`.

**Acceptance.** Every `v3` row in the pit map that names a Phase 2 test is
green; negative fixtures fail closed as specified.

## Phase 3 — Session economics and process types (P2)

**Deliverable.** DR-012 + process-model §2/§5 types:

- `TurnBudget`, checkpoint auto-write triggers, `EvidenceGap`, serving-model
  disclosure, liveness callback, bounded tool waits.
- `Qualification` / `BatchDesign` / `ExecutionRecord` / `ReviewRecord` with the
  self-certification gate.
- `PromoteAuthority` stub (state machine edge only).
- `ContinuityTrial` topology types (spec-level; execution stays real).

**Tests (new).** `pit.checkpoint_tracks_transactions`,
`pit.checkpoint_survives_turn_loss`, `pit.liveness_reports_observables`,
`pit.self_review_refused`, `pit.restored_never_self_promotes`,
`pit.cold_boot_not_artifact_trial`, `pit.view_never_invented`.

## Phase 4 — Validation per OBL-20260918T215000Z-B4D6A8

**Deliverable.** The formal validation the obligation requires, with its
delivery profile named:

1. **Invariant inventory executed.** Every `C` row in
   `invariant-inventory.md` traced to a passing compiled check; every `J` row
   confirmed pinned.
2. **Pit suite as regression contract.** Every `P-*` row green; the map and the
   inventory cross-validated (§5 rule).
3. **Consistency classification against accepted contracts** — the obligation's
   core: draft semantics vs canonical prose contracts, each inconsistency
   classified and resolved or recorded (the P-40 view correction is the first
   known instance, resolved in this corpus).
4. **Delivery profile.** Repository semantics (types, gates, suites): remote
   cold boot profile. Continuity claims (snapshot export/restore): a real K4-
   shaped artifact trial with an independent producer and an isolated fresh
   consumer — never a self-simulation (P-13).
5. **Outcome record.** Dated validation report in the canonical repository's
   `evidence/audits/`, binding draft commit identity to the executed proof;
   candidate semantics accepted into kernel design or returned with concrete
   deltas.

## Sequencing rules

- A phase lands only after the previous phase's pit tests are green; pit tests
  never weaken to make a phase pass (worker-protocol §15 discipline applies to
  this repository by its own AGENTS.md).
- Headers advance with phases, never ahead: a header comment citing a corpus
  rule the code does not enforce is drift and fails review.
- The `docs/engineering/` Devkit surface is untouched; `AGENTS.md` remains
  binding for implementation work even though this corpus was authored under
  the owner's extended-cognition directive.

## Risks and honest boundaries

- **Judgment-only residue** (inventory §4) means some failures remain possible:
  the system converts them from *silent* to *caught-at-a-named-gate*. That is
  the achievable property, and it is enough — every incident in the ledger was
  silent until it was expensive.
- **Token economics**: the corpus grows input size for any consumer that reads
  it whole. The bundle/phase-2 machinery is the intended antidote; until it
  exists, the README's document map is the cheap entry point.
- **Model substitution risk**: the corpus was authored under a disclosed
  substitution (GLM-5.3-Flash for the declared GLM-5.3 brother binding).
  Phase-4 validation should include a review pass by a different-tier instance
  or the owner before candidate semantics are accepted — the same
  replacement-tier review rule ADR-0035 applies to merge-class actions.
