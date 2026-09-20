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

The compiled proofs establish the SEMANTICS. What they cannot establish
alone is **artifact-causal activation**: that a fresh, isolated consumer
receiving only a handoff artifact (no repository, no prior session)
reconstructs the invocation semantics and demonstrates obligations 1, 6
and 8 from the artifact — the K4-shaped producer → artifact → isolated
consumer topology (seed §41's "without relying on the participant's prior
session memory", at full strength).

**The standing window stops here by definition**: the trial needs an
independent producer run through the owner's trusted local path (H1 —
the K4 `k4-handoff-acceptance` producer pattern, exporting a snapshot
that now carries the invocation policy), and an isolated fresh-LLM
consumer session (owner-launched).

Proposed trial (for owner authorization):
1. Owner runs the JasonPC producer gate against the published v4 candidate
   (exact head after this branch merges); the K4 artifact machinery
   already serializes `invocation` (v7) — no producer change needed.
2. Owner launches a fresh LLM session receiving only the artifact plus
   the generic K4 Phase A instruction, with three added Phase A tasks:
   (a) state the naming-policy requirement for CreateCppSymbol;
   (b) state whether a Publish action may proceed without a full-gate
   receipt (it may not — P-53 rule travels in the policy);
   (c) state the retry rule for a repeated identical failure.
3. Phase B live verification confirms the artifact identity.

PASS upgrades v4 semantics to accepted kernel design material per
`OBL-20260918T215000Z-B4D6A8`'s validation path; the trial record lands in
`evidence/audits/`.
