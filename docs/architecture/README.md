# qiven-context-draft — Architecture Documentation

This directory is the **design corpus** for the executable C++ specification of the
qiven-context cognition architecture. It is the written half of the semantic bridge:
the C++ headers are the compiled half. Where a rule can be expressed as a type, a
gate or a test, it lives in code; where a rule requires human judgment, it is pinned
here as a decision record with its rejected alternatives, not silently left to prose
erosion.

**Status: candidate design input** (ADR-0037 in `JasonHuang3D/qiven-context`).
Nothing here changes how qiven-context runs today. The prose contracts, the
Operator, and the K4/K5 acceptance paths in the canonical repository remain
authoritative until formal validation completes (`OBL-20260918T215000Z-B4D6A8`).
Canonical recording of accepted outcomes from this corpus flows back to
`JasonHuang3D/qiven-context` through the normal transaction process.

## Why this corpus exists

The project consumed frontier-LLM effort measured in tens of billions of tokens
maintaining project context in prose, and still produced drift: stale state
surfaces, competing active truth, evidence decoupled from the deltas it reviewed,
authority asserted by whoever held a capability, retrieval skipped at exactly the
moments it mattered. The failure ledger shows these are overwhelmingly
**authority, identity, epistemics and process** failures — not data-structure
failures. Draft v2 proved the data model compiles. This corpus specifies the rest
of the machine: the failure channels, the recovery rules, the acceptance
boundaries, and the mapping from every recorded scar to the type, gate or test
that now guards it.

Design principle: **scars compile.** Every recorded pit in the canonical memory
corpus gets a named encoding. A future refactor that removes a guard fails a test
named after the incident that motivated it.

## Document map

| Document | Question it answers |
| --- | --- |
| [requirements.md](requirements.md) | What does qiven-context actually need when it is *used*? Derived from observed operation, with a coverage verdict on draft v2. |
| [cognition-model.md](cognition-model.md) | What are the v3 semantics? Planes, snapshot model, value tree, transactions, verdicts, authority, conflicts, bundles, store, runtime. |
| [process-model.md](process-model.md) | How does the loop actually run — two-layer execution, failure-response ladder, retrieval discipline, session economics, Git/CI discipline? |
| [decisions.md](decisions.md) | Which design decisions were made and — equally — which alternatives were rejected, and why (negative knowledge, first-class). |
| [pit-regression-map.md](pit-regression-map.md) | Where is every recorded pit encoded? The scar ledger: pit → evidence → type/gate/test → status. |
| [invariant-inventory.md](invariant-inventory.md) | Which invariants are compiled, which are compilable, and which are judgment-only forever — stated honestly. |
| [v3-roadmap.md](v3-roadmap.md) | In what order do the semantics land, and what proves each phase? Includes the validation-profile sketch for `OBL-...-B4D6A8`. |

## Reading order

1. `requirements.md` — the demand side, with evidence.
2. `cognition-model.md` — the supply side.
3. `process-model.md` — how the two meet in a running loop.
4. `decisions.md` — why it is this way and not the other ways.
5. `pit-regression-map.md` and `invariant-inventory.md` — the audit surfaces.
6. `v3-roadmap.md` — the plan.

## Relationship to other surfaces

- `docs/engineering/` — Devkit-managed process templates for worker/brother
  collaboration on this repository. Unchanged by this corpus; the two directories
  answer different questions (how we build vs. what we are building).
- `include/qiven/context/*.hpp` — draft v2 headers. Where this corpus refines or
  corrects v2 semantics, the header comment stays and the corpus rules; headers
  advance with the v3 phases, not ahead of them.
- `JasonHuang3D/qiven-context` — canonical prose contracts, ADRs, memory,
  obligations. This corpus cites them as evidence and never redefines them.

## Authorship note

Produced 2026-09-19 in an owner-directed extended-cognition design session
(`ContextView<ZCode, Jason>`, serving model GLM-5.3-Flash, reasoning highest,
disclosed per ADR-0035 rule 4). Owner authorized free architectural reasoning
across this corpus; the owner's acceptance role and the canonical recording path
are unaffected.
