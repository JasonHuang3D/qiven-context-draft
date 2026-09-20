# v4 Validation Report — Activation Proofs and the H1 Boundary

**Status: candidate record.** Binds the v4 phases (v4.0-v4.5) to their
compiled proofs per the seed's §41 obligations, names the delivery profile
of each, and identifies exactly which obligations require the H1 boundary
the standing window stops for. Draft commit identity: branch
`jason-extended-cognition/v4-boundaries` @ `94ec522` (and merged v4.3 @
`afd9d2d`); all gates + merge-proof PASS at exact heads.

Delivery profiles: **repo-semantics proofs** are remote-cold-boot profile
(the draft repository and its compiled tests are the evidence); **the
fresh-consumer activation trial** is artifact-handoff profile (K4-shaped:
independent producer, isolated fresh consumer) and is the H1 boundary.

## The nine §41 obligations

| # | Obligation | Status | Evidence (compiled proof) | Profile |
| --- | --- | --- | --- | --- |
| 1 | Known naming rule is activated | **compiled** | `control_plane.cpp` naming case: CreateCppSymbol intent → packet carries the naming policy from the snapshot only; transport miniature proves a restored generation derives identical preparation | repo-semantics |
| 2 | Lower-layer duplication is intercepted | **compiled (miniature)** | `control_plane.cpp` A8 case: search-mandatory rule + `primitive_judgment_authorized` refuses unsearched judgment; `qiven::fnv1a64` surfaced for judgment. The OPERATIONAL symbol/dependency search remains execution-time mechanism (seed §36) | repo-semantics |
| 3 | Tool syntax is not guessed | **compiled** | `tool_and_retry.cpp` A6: contract constructs argv; three guessed-variant shapes fail validation | repo-semantics |
| 4 | First failure changes the reasoning state | **compiled** | A7: normalization + `find_related_records` recalls the odyssey pit from a fingerprint; default policy lists InspectKnownPit for RetryFailure | repo-semantics |
| 5 | Repeated blind retry is refused | **compiled** | A7: `retry_permitted` refuses the equivalent retry without new evidence (V4-R3) | repo-semantics |
| 6 | Canonical claim retrieves canonical truth | **compiled** | `control_plane.cpp` fail-closed case: VerifyCanonical unresolved → packet not ready; default policy carries the rule | repo-semantics |
| 7 | Live claim verifies live state | **compiled (boundary)** | MakeLiveClaim → VerifyLive is a listed, execution-time demand by default policy; the draft deliberately hosts no live ports (they are runtime mechanisms) | repo-semantics |
| 8 | Participant replacement preserves invocation semantics | **compiled** | InvocationPolicy is cognition data (snapshot member, serialization v7); policy lookup never reads participant fields; transport miniature | repo-semantics |
| 9 | Retrieval backend replacement preserves semantics | **compiled (by absence)** | The ontology depends on no retrieval backend (seed §36); v1 resolution is deterministic string matching — no semantic machinery exists to replace | repo-semantics |

## What remains for v4 acceptance — the H1 boundary

The compiled proofs establish the SEMANTICS. The remaining acceptance
step is **artifact-causal activation**: a fresh, isolated consumer
receiving only a handoff artifact demonstrates that the rules v4 compiled
are PRESENT and FINDABLE in carried cognition — the seed §5 failure class
("a rule is present in context, but the reasoning that needed the rule
proceeds as if it did not") tested at full artifact strength.

**H1 per the refined isolation-boundary definition (2026-09-20,**
`collaboration/human-handoff-boundary.md`): the producer run is
deterministic and digest-verifiable, so the agent executes it (owner
direction) against published main via the parameterized
`k4-handoff-producer-main` task; the OWNER performs the boundary
operations — verify the artifact identity, launch a fresh LLM session,
attach the artifact with the prepared verbatim prompt, and relay the
sealed reply back verbatim. The authoring session grades against the
rubric (evaluator role; the key never precedes the seal).

The prepared kit (artifact path, digest, verbatim consumer prompt, relay
instructions) is recorded in the v11 session checkpoint and the window's
H1 kit file; the trial record lands in `evidence/audits/` on PASS.
