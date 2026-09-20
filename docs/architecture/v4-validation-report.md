# v4 Validation Report — FINAL CLOSEOUT

**Status: final (post-stabilization).** Binds the v4 feature closeout, the
artifact-causal activation trial, and the post-closeout stabilization
(V4S) into one record.

- Feature closeout: v4.0-v4.6 on draft main lineage through `b15e41f`.
- Canonical acceptance: `qiven-context` `evidence/audits/context-v4-activation-trial-2026-09-20.md`
  (PASS with finding F1, fixed in canonical `6cd93db` lineage).
- Stabilization closeout: this branch (V4S-01..07); exact head recorded by
  the V4S-07 validation run below.
- Distinction preserved: **compiled repo-semantics proofs** (bind to exact
  draft heads) versus **artifact-causal activation proof** (the trial).
- Post-stabilization deltas: S0-01..S1-06 remediations changed executable
  semantics in the ways the stabilization plan prescribed (policy presence,
  fail-closed control, readiness split, resolver closure, claim matching,
  reference-state honesty); the re-validation is the V4S gate run at the
  final exact head - every original v4.6 proof remains green under the new,
  stricter semantics.

## The nine §41 obligations

| # | Obligation | Status | Evidence (compiled proof) | Profile |
| --- | --- | --- | --- | --- |
| 1 | Known naming rule is activated | **compiled** | `control_plane.cpp` naming case: CreateCppSymbol intent → packet carries the naming policy from the snapshot only; transport miniature proves a restored generation derives identical preparation | repo-semantics |
| 2 | Lower-layer duplication is intercepted | **compiled (miniature)** | `control_plane.cpp` A8 case: search-mandatory rule + `primitive_judgment_precondition_met` refuses unsearched judgment; `qiven::fnv1a64` surfaced for judgment. The OPERATIONAL symbol/dependency search remains execution-time mechanism (seed §36) | repo-semantics |
| 3 | Tool syntax is not guessed | **compiled** | `tool_and_retry.cpp` A6: contract constructs argv; three guessed-variant shapes fail validation | repo-semantics |
| 4 | First failure changes the reasoning state | **compiled** | A7: normalization + `find_related_records` recalls the odyssey pit from a fingerprint; default policy lists InspectKnownPit for RetryFailure | repo-semantics |
| 5 | Repeated blind retry is refused | **compiled** | A7: `retry_permitted` refuses the equivalent retry without new evidence (V4-R3) | repo-semantics |
| 6 | Canonical claim retrieves canonical truth | **compiled** | `control_plane.cpp` fail-closed case: VerifyCanonical unresolved → packet not ready; default policy carries the rule | repo-semantics |
| 7 | Live claim verifies live state | **compiled (boundary)** | MakeLiveClaim → VerifyLive is a listed, execution-time demand by default policy; the draft deliberately hosts no live ports (they are runtime mechanisms) | repo-semantics |
| 8 | Participant replacement preserves invocation semantics | **compiled** | InvocationPolicy is cognition data (snapshot member, serialization v7); policy lookup never reads participant fields; transport miniature | repo-semantics |
| 9 | Retrieval backend replacement preserves semantics | **compiled (by absence)** | The ontology depends on no retrieval backend (seed §36); v1 resolution is deterministic string matching — no semantic machinery exists to replace | repo-semantics |

## Artifact-causal activation acceptance

The artifact-causal activation trial was executed on 2026-09-20.

Verdict: **PASS with finding F1.**

- Producer: the deterministic k4 producer run against published main
  `e9a9aef` (agent-executed per the refined H1 isolation-boundary
  definition, owner-directed); handoff digest
  `sha256:c5be7d1e3898a8db3f77b4ffa94b20a75836785372f915f43f8e5173ba51fc7f`.
- Fresh isolated consumer: GLM-5.3-Flash (reasoning max), owner-launched,
  artifact-only during Phase A; the sealed reply was graded against the
  prepared rubric.
- Finding F1: cross-repository guard landing (P-51 in qiven-devkit) was
  absent from carried canonical cognition. The consumer correctly
  ABSTAINED rather than inventing the missing fact. F1 was subsequently
  closed in canonical cognition (MEM-20260920T153000Z-D4F8A2 landing
  inventory + the cross-repo recording rule).
- A first consumer attempt was invalidated for an isolation breach by
  artifact PLACEMENT (artifact attached from inside the repository tree);
  the relocation rule now lives in the generated-temp convention.
- Full record: `qiven-context evidence/audits/context-v4-activation-trial-2026-09-20.md`.

This trial supplies the causal activation evidence required beyond the
compiled repo-semantics proofs.

## V4S-07 — final stabilization validation record

Validated exact head: `55a8fa9` (draft local gate PASS + merge-proof PASS,
debug + release, 8/8 suites green). Verified per plan sec 33:

- all original v4.6 proofs remain green under the stricter V4S semantics
  (naming scenario, transport-stable activation, lower-layer interception,
  tool-contract guess rejection, blind-retry refusal);
- all new stabilization regressions green (genesis policy, World A/B,
  legacy no-silent-synthesis, readiness split matrix, decoy recall,
  fingerprint-connected pit recall, claim axis, needs-cannot-waive);
- serialization golden intentionally re-pinned at v8
  (`snap-aea527846a299800`); artifact corruption still fails closed;
- pit traceability green (40 named tests, P-54..P-59 compiled).

**v4 semantics freeze from this head.** Runtime ADL may replace mechanisms
(string lookup, reference bool, fixed argv vector, in-memory miniature)
but must preserve the semantic obligations (plan sec 35).
