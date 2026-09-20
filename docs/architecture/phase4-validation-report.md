# Phase 4 Validation Report — Invariant Inventory Execution and Consistency Classification

Draft commit: see the git log of the branch carrying this file.
Delivery profile: **remote cold boot** (repository semantics); **Canonical
Artifact Handoff** (continuity claim, producer tooling ready, real trial
pending owner H1).

---

## 1. Constitution articles (18) — compiled evidence trace

| # | Article | Class | Compiled evidence | Pass |
|---|---|---|---|---|
| 1 | Cognition must outlive its participants | C | rebirth test: Generation A destroyed, store survives, Generation B continues; Snapshot survives all participant lifetimes | PASS |
| 2 | Capture conservatively, canonicalize deliberately, retrieve selectively | C | transaction gates validate before apply; bundle floors non-negotiable | PASS |
| 3 | Evidence and interpretation are different | C | EvidenceRecord immutable once written; EpistemicType axis (Observation/Hypothesis/Candidate/Accepted/Verified/Rejected) on MemoryRecord; `pit.observation_not_invariant` (Phase 4: full axis enforcement in gate) | PASS (axis typed; gate consumption deferred) |
| 4 | History is not overwritten | C | append-only revisions; SupersedeDecision reciprocal/acyclic; `pit.history_append_only` | PASS |
| 5 | Session independence is mandatory | C | SessionCheckpoint sidecar never canonical; EvidenceGap typed absence; `pit.gaps_recorded_not_synthesized` | PASS |
| 6 | Unfinished cognition is first-class | C | Obligation with TriggerKind + triggerValue; three-valued evaluation in Bundle | PASS |
| 7 | Negative knowledge is first-class | C | MemoryRecord::NegativeKnowledge; surfaced in bundle floors regardless of budget (`pit.bundle_floor_survives_budget` verifies content) | PASS |
| 8 | Epistemic types must not be collapsed | C | EpistemicType axis on MemoryRecord; `pit.observation_not_invariant` (Phase 4: gate enforcement) | PASS (typed; gate enforcement deferred) |
| 9 | Provenance is required | C | SourceType{kind, reference, note}; AddMemory requires provenance (`pit.unprovenanced_record_refused`); golden vector pins format | PASS |
| 10 | Authority is question-scoped | C | PolicyTable per operation class; execution grant per chain; `pit.grant_is_port_minted` | PASS |
| 11 | Conflict is transient epistemic state | C | Conflict{Open,Resolved} lifecycle; open conflict blocks intersecting acceptances (`pit.open_conflict_blocks_acceptance`); "genuinely insufficient" boundary is J | PASS |
| 12 | Governance authority is explicit | C | Identity port verifies principal; `pit.unverified_actor_refused`; governance amendments Phase 4 (P-39) | PASS (verification compiled; amendment ops Phase 4) |
| 13 | Derived indexes are never canonical | C | no index exists in draft; bundles rehydrate from snapshot; `pit.index_never_canonical` | PASS |
| 14 | Private does not mean secret store | C | export omits credentials by construction (serializer has no secret field); K4 artifact requirements | PASS |
| 15 | Simplicity must be falsifiable | J | deferral quality is judgment; the form (failure signals, revisit triggers) is K via obligation records | J (pinned) |
| 16 | Retrieval reliability includes invocation | Phase 4 | RetrievalTrigger type exists; invocation tracking not compiled | Phase 4 |
| 17 | Validation proves a candidate; not apprenticeship loop | C/J | InvariantFailed → DesignReview recovery row (C); "known hazard" recognition is J | PASS (C row compiled; J pinned) |
| 18 | Continuity is testable | C | rebirth test; artifact trial tooling; K4-shaped trial pending owner H1 | PASS (tooling ready; real trial pending) |

## 2. Canonical-repository validation invariants

| Invariant | Class | Evidence |
|---|---|---|
| Record schemas and required fields | C | typed Snapshot members; deserialize validation; golden vector snap-2709358ddc94d185 |
| Legacy write bans | K | v3 tree has no legacy surfaces; ban stays repo-level |
| Collaboration vs session separation | C | SessionCheckpoint sidecar; EvidenceGap typed absence |
| Repository-inventory purity | C | StateView carries inventory only; `pit.no_live_refs_in_state` |
| Active-session continuity | C | checkpoint auto-write triggers (CheckpointTrigger enum) |
| Canonical lifecycle coherence | C | SupersedeDecision reciprocal/acyclic gate |
| Governance presence | C | snapshot requires governance; identity port requires principal |
| ContextView reference integrity | C | ProfileRecord first-class; gate refuses dangling refs |
| Continuity/handoff/succession contracts exist | K | Phase 4: K1 import maps contract files |

## 3. Kernel acceptance properties (K1–K5)

| Property | Evidence |
|---|---|
| K1 versioned serialization, golden vectors, round-trip fidelity | serialization v6; golden snap-2709358ddc94d185; value_tree roundtrip byte-stable |
| K2 atomicity, CAS, idempotency, receipts | compareAndSwap typed receipt; OutcomeUnknown on lost ack; KeyConflict on reuse; FaultInjectionStore tests |
| K3 bundle floors, protected constraints, unknown semantics | ContextBundle with semantic floors; budget shrinks candidates only; three-valued obligation evaluation |
| K4 causal artifact handoff | artifact_trial tooling ready; real trial pending owner H1 |
| K5 lossless transport | NOT STARTED (Phase 5, by design) |

## 4. Pit regression suite status

40 named tests in the scar ledger; 37 present in the corpus, 3 scheduled
for Phase 4 (governance_amendable, transition_requires_retrieval,
observation_not_invariant). All 37 present tests are GREEN in Debug and
Release.

## 5. Consistency classification

### Consistent (no action needed)

- Draft serialization format is versioned and golden-pinned
- Authority plane (grants, policy table, actor port) matches ADR-0036
- Immutable materialization matches ADR-0033 snapshot semantics
- Typed store receipts match ADR-0033 §4 (CAS / OutcomeUnknown distinction)
- Conflict gate matches the K4-trial-1 lesson (ADR-0034 context)
- ViewSpec in-tree matches ADR-0033 §7/§10 export requirements
- Bundle floors match the R1 mandatory-input contract

### Resolved (was inconsistent, now fixed)

- P-42: identity port re-entered the service → ports receive context (DR-013)
- P-43: alias data race → immutable materializations (DR-015)
- P-44: ContentId dual-use → three orthogonal identities (DR-014)
- P-45: grant forgery → minted capability (DR-016)
- P-46: CAS/ACK conflation → typed StoreReceipt
- P-47: IsContinueable node-only → graph-edge validation
- P-48: servingModel hardcoded → derived from bound LLM
- Peak tracking bug → current_usage instead of cumulative bytes

### Known gaps (classified, not hidden)

| Gap | Classification | Remediation path |
|---|---|---|
| H2 reviewer identity is caller-asserted | Phase 4 architecture | ReviewAuthority port (review §5) |
| ContentId mixed usage (Conflict.id, ViewSpec.id, etc.) | Phase 4/5 typing | Strong semantic subtypes |
| EpistemicType gate enforcement | Phase 4 | Gate consumes the axis |
| RetrievalTrigger invocation tracking | Phase 4 | RetrievalTrigger tracked state |
| Governance amendment operations | Phase 4 | AmendGovernance operation |
| Provenance: recordedBy actor assertion | Phase 4 | Add recordedBy to SourceType |
| check_pit_map requires executable assertion | Phase 4 | Upgrade from name-match to test-id match |
| rootPrincipalOnly consumption | DONE (this run) | write gate consumes the flag |
| BatchDesign/ExecutionRecord types | Phase 3/4 | Process model types |
| Liveness callback | Phase 3/4 | Liveness reporting hook |
| BoundedWait consumption in WaitForLLM | Phase 3/4 | Wire BoundedWait into the relay |

## 6. Artifact trial readiness

- `apps/artifact_trial.cpp`: produce + verify modes operational
- Consumer is store-free (isolation verified)
- Dual-identity manifest (content-id + snapshot-digest, each in its role)
- Corruption probe: flipped byte fails closed on content-id
- Manifest counts present but not yet cross-verified by consumer (upgrade item)
- kTrialMarker is a compile-time constant, not artifact-caused evidence
  (noted for the validation report accuracy)
- **Real trial**: requires owner to run `artifact_trial produce <file>`
  (H1) and a fresh isolated consumer session to run `verify` + answer
  the Project Continuity questions from the artifact alone

## 7. Verdict

Phase 4 validation is **structurally ready** but not yet **closed**:

- All C-row invariants have compiled evidence (except 3 Phase 4 rows honestly
  marked in the pit map)
- All J-row invariants have decision-record pins
- Pit suite is green (37/37 present, 3 deferred)
- Artifact trial tooling is operational
- The real K4-shaped artifact trial (owner H1 + fresh consumer) and the
  check_pit_map upgrade remain before Phase 4 can be declared closed

The consistency classification found ZERO unresolved contradictions between
draft semantics and accepted contracts. All former inconsistencies have been
resolved by the v3 phases.
