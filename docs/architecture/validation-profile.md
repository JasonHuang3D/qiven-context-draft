# Validation Profile — OBL-20260918T215000Z-B4D6A8

**Status: candidate design input (ADR-0037).** This document is the validation
profile the obligation requires before any draft semantics feed kernel or K5
design as accepted material. It names the property to prove, the delivery
profile of the proof, and how evidence binds to an exact draft commit.

## The property under validation

**P — the draft's compiled semantics are consistent with the accepted
qiven-context contracts** (ADR-0033 kernel architecture, ADR-0034 delivery
profiles, ADR-0036 typed handoffs, the record-lifecycle and read contracts),
and every recorded pit in `pit-regression-map.md` is mechanically guarded.

Consistency here means: for each contract obligation, the draft either encodes
it (type/gate/test — see `invariant-inventory.md` classes C and K) or
explicitly scopes it out with a recorded reason (class J). Silent divergence
in either direction fails.

## Delivery profile

Named per the ADR-0034 rule that every proof declares its profile:

1. **Repository-semantics proof — remote cold boot profile.** The draft
   repository's own validation (Devkit gate: configure, build Debug/Release,
   full test suite, diff-check, clean-tree) executed at an exact draft commit,
   published and verifiable from the canonical GitHub remote. This proves the
   compiled invariants and the pit regression suite.
2. **Continuity claim — Canonical Artifact Handoff profile.** Any claim that
   draft snapshots survive transport (K4-shaped export/restore semantics)
   requires a real isolated trial: independent producer, artifact-only
   consumer, Phase B live verification. A remote cold boot never substitutes
   (pit P-13). Scheduled after Phases 1–3 land the snapshot/export machinery
   the trial needs.

## Phased acceptance map

| Phase (v3-roadmap) | Proves | Evidence |
| --- | --- | --- |
| 0 — design corpus | the demand/supply model is written and reviewable | owner review of `docs/architecture/` (done 2026-09-19) |
| 1 — authority, verdicts, corruption-safety | P0 pit tests green in Debug and Release: split-brain refusal, unverified actor, fail-closed grant state, content-bound H2, no-retry on handoff refusal, reader-boot preserves writer, corruption/resource fail-closed, golden vectors | Devkit gate PASS at exact head |
| 2 — transactions, conflicts, views, bundles | atomicity, idempotency, conflict gate, view integrity, bundle floors | same, plus K2-shaped negative fixtures |
| 3 — session economics, process types | checkpoint triggers, liveness, self-certification gate, promotion stub | same |
| 4 — validation closeout | inventory executed (every C row traced, every J row pinned), inconsistencies classified and resolved or recorded | dated report in canonical `evidence/audits/` bound to the exact draft commit |

## Execution rules

- Every pit test is named after its recorded pit; a failing pit test blocks
  the phase, and weakening a pit test to pass a phase is a validation failure.
- Evidence binds to the exact draft commit (candidate SHA) and the gate
  transcript; the final exact head re-runs the full gate before any acceptance
  claim (ADR-0021 discipline).
- The acceptance-topology roles (H1 producer, isolated fresh consumer) for the
  Phase-4 continuity trial cannot be simulated by the authoring session
  (context-validation.md acceptance-role clarification).
- Outcome: the report either upgrades draft semantics into kernel design
  material (ADR-0037 step 2) or returns concrete deltas. Until then, current
  context capabilities remain authoritative and no cutover occurs.
