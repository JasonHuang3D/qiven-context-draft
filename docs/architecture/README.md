# qiven-context-draft — Architecture Documentation

This directory is the design corpus for the **frozen executable C++
specification** of the qiven-context cognition architecture. It is the
written half of the semantic bridge: the C++ headers are the compiled half.
Where a rule can be expressed as a type, a gate or a test, it lives in code;
where a rule requires human judgment, it is pinned here as a decision record
with its rejected alternatives.

**Status: final accepted semantic contract.** The v3/v4 design and
stabilization phases are COMPLETE (V4S + freeze closeout); the last
executable semantic change is `4cbc995`, pinned by
[v4-validation-report.md](v4-validation-report.md) — read it as the **dated
final proof** (with its `4cbc995` semantic pin and cited test receipts), not
as a current roadmap. Later commits on this branch are non-semantic
(doc/maintenance) unless a new accepted decision says otherwise. This
repository is a specification/reference implementation, NOT the production
Context runtime: qiven-runtime owns the production path (Component ADL,
RuntimeHost, the MVP program). Canonical authority — contracts, ADRs,
memory, obligations — lives in `JasonHuang3D/qiven-context`; this corpus
cites it as evidence and never redefines it.

## Current map (active documents)

| Document | Question it answers |
| --- | --- |
| [requirements.md](requirements.md) | What does qiven-context actually need when it is *used*? Derived from observed operation. |
| [cognition-model.md](cognition-model.md) | The v3 semantics: planes, snapshot model, value tree, transactions, verdicts, authority, conflicts, bundles, store, runtime. |
| [process-model.md](process-model.md) | How the loop runs — causal order, execution discipline (ADR-0044 single-session reading), the failure-response ladder, retrieval discipline, session economics, Git/CI discipline. |
| [participant-model.md](participant-model.md) | Participants and authority: roles vs designations vs bindings; workflow dependency layering; the participant dependency matrix and I-PM invariants. |
| [cognitive-boundary-model.md](cognitive-boundary-model.md) | The v4 seed: Judgment / Cognitive Control / Mechanism separated by specification closure; ActionIntent-driven cognition activation, invocation policy, PreparationPacket, V4-R invariants. |
| [activation-failure-inventory.md](activation-failure-inventory.md) | The real persistence-success/activation-failure scars, the minimal ActionKind vocabulary they ground, the ranked trigger/guard backlog. |
| [v4-validation-report.md](v4-validation-report.md) | **Dated final proof** of the frozen v4 semantic contract (`4cbc995` pin, test receipts). |
| [decisions.md](decisions.md) | Which design decisions were made and — equally — which alternatives were rejected, and why (negative knowledge, first-class). |
| [pit-regression-map.md](pit-regression-map.md) | Where is every recorded pit encoded? The scar ledger: pit → evidence → type/gate/test → status. |
| [invariant-inventory.md](invariant-inventory.md) | Which invariants are compiled, which are compilable, and which are judgment-only forever — stated honestly. |
| [validation-profile.md](validation-profile.md) | The validation profile sketch (FULL/FOCUSED semantics). |

## Historical area

[legacy/](legacy/) preserves the completed plans, reviews and original
design-input versions verbatim (moved 2026-09-26 PR4 doc-repair):
`v3-roadmap.md`, `v4-roadmap.md`, `v4-stabilization-plan.md`,
`v4-freeze-cleanup-plan.md`, `v4-freeze-review.md`,
`phase4-validation-report.md`, and the original role-staged
`process-model-v3-design-input.md` / `participant-model-v3-design-input.md`
(their live paths carry the current revised subsets). These are historical
evidence — do not treat them as current instructions.

## Reading order (current entry)

1. `requirements.md` — the demand side, with evidence.
2. `cognition-model.md` — the supply side.
3. `process-model.md` — how the two meet in a running loop.
4. `participant-model.md` — who is in the loop, and what each may do.
5. `cognitive-boundary-model.md` — the v4 seed: when cognition must become active.
6. `decisions.md` — why it is this way and not the other ways.
7. `pit-regression-map.md`, `invariant-inventory.md` and
   `activation-failure-inventory.md` — the audit surfaces.
8. `v4-validation-report.md` — the dated final proof and freeze record.

## Relationship to other surfaces

- Engineering conventions and standards are canonical in the Devkit
  (`JasonHuang3D/qiven-devkit`: `docs/conventions/README.md` and
  `docs/engineering/README.md`, ADR-0046). This repository carries no local
  `docs/engineering/` copy; earlier corpus text pointing at one is
  superseded.
- `include/qiven/context/*.hpp` — the compiled specification (frozen v4
  semantics per the validation report). Where this corpus refines or
  corrects older semantics, the header comment stays and the corpus rules;
  headers advance only under a new accepted decision.
- `JasonHuang3D/qiven-context` — canonical prose contracts, ADRs, memory,
  obligations (including the ADR-0044 single-session engineering law).

## Authorship note

Corpus authored 2026-09-19/20 in owner-directed design sessions
(`ContextView<ZCode, Jason>`; models disclosed per ADR-0035 rule 4). Owner
authorized free architectural reasoning across this corpus; the owner's
acceptance role and the canonical recording path are unaffected.
