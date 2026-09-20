# Pit Regression Map — The Scar Ledger

**Status: candidate design input (ADR-0037).** Every recorded pit from the
canonical project memory, incident audits, constitution articles and ADR
lessons, mapped to the type, gate or test that now guards it. **Scars compile:**
a refactor that removes a guard fails a test named after the incident that
motivated it.

Status legend: **v2** = encoded in draft v2 today · **v3** = specified in this
corpus, lands with the v3 phases · **judgment** = deliberately not compiled;
pinned by decision records / prompts (`invariant-inventory.md` §4) · **v4** =
activation-layer encoding per the cognitive boundary model; the operational
guard lives in the devkit/context repositories (see the v4 activation ledger).

Failure classes: `AUTH` authority/identity · `EVID` evidence/acceptance ·
`RETR` retrieval/economy · `TRAN` transport/durability · `ENV` environment ·
`PROC` process/economics · `EPIST` epistemics/lifecycle.

---

## Authority and identity

| ID | Pit (what happened) | Canonical evidence | Class | Encoding | Test name | Status |
| --- | --- | --- | --- | --- | --- | --- |
| P-01 | Two conversational flows concurrently used one local execution capability; no caller identity existed to fence them | 2026-09-16 split-brain incident; MEM-20260915T163500Z-5E7A91; ADR-0026 | AUTH | `ExecutionGrant` acquisition + fail-closed refusal + quarantine (DR-003) | `pit.split_brain_second_flow_refused` | v3 |
| P-02 | Authority asserted by holding a capability or passing a pointer ("I am the only executor") | ADR-0026 authority rules | AUTH | `AuthenticatedActor` from live identity port, re-checked at commit; participants hold `const` handles (DR-001/003) | `pit.unverified_actor_refused` | v3 |
| P-03 | Authority state silently reverted when a request was rejected | MEM-20260916T040700Z-D8A4C2 | AUTH | quarantine as a state machine; refusals never de-fence a held grant | `pit.rejected_flow_stays_fenced` | v3 |
| P-04 | Session identity client-selectable on the wire; copied identity outranked host-owned identity | MEM-20260916T042800Z-B71E3C; MEM-20260916T045200Z-C8F219 | AUTH | grant minted only by the authority port; client-supplied identity is a claim, verified never trusted | `pit.grant_is_port_minted` | v3 |
| P-05 | A passing handoff was treated as review; exact remote review of the exact delta still required | MEM-20260913T162546Z-E73124; ADR-0036 H2 | EVID | `H2Evidence.reviewedDeltaDigest` content binding (DR-004) | `pit.h2_evidence_rebind_refused` | v3 |
| P-07 | Verbal attempt to skip a mandatory typed handoff | ADR-0036 no-verbal-waiver protocol | AUTH | policy table in cognition; no code path waives; attempt is recordable | `pit.handoff_has_no_waiver_path` | v3 |
| P-08 | Unattended automation attempting mutation | ADR-0036 | AUTH | WorkMode gate (v2) + unattended grants carry no write right | `pit.unattended_mutation_refused` | v2 |
| P-09 | Reviewer self-certification risk: the reviewing instance upgrading its own qualification | ADR-0035; local-supervised-agent workflow | AUTH | `ReviewRecord` carries reviewing binding; gate refuses same-binding review for H2 rows | `pit.self_review_refused` | v3 |
| P-10 | Serving-model substitution left silent under provider failure | ADR-0035 rule 4; v8 session record; 2026-09-19 quota exhaustion | AUTH | `ServingDisclosure` mandatory on actor and checkpoint | `pit.undisclosed_substitution_flagged` | v3 |
| P-33 | Restored artifact silently gaining write/governance authority | context-handoff contract failure conditions; ADR-0033 §10 | AUTH | quarantine state machine; `PromoteAuthority` is the only edge to `Promoted` | `pit.restored_never_self_promotes` | v3 |

## Evidence, transactions, durability

| ID | Pit | Canonical evidence | Class | Encoding | Test name | Status |
| --- | --- | --- | --- | --- | --- | --- |
| P-11 | Connector refusal euphemized as an engineering objection; semantics weakened to pass the connector | MEM-20260915T203423Z-C4A912 | TRAN | typed `TransportRefused` reason; authoring-path ladder preserves semantics | `pit.connector_refusal_switches_path` | v3 |
| P-12 | High-level contents-API mutation used for critical writes | MEM-20260916T095000Z-7C4E91 | TRAN | publication path types: named-branch push / `gh pr create` only | `pit.merge_surrogate_refused` | v3 |
| P-14 | Resolvable canonical conflict discovered only at Phase B, blocking K4 trial 1 | `evidence/audits/context-k4-handoff-trial-1-2026-09-18.md`; constitution §11 | EPIST | `Conflict` record + acceptance-blocking gate (DR-006) | `pit.open_conflict_blocks_acceptance` | v3 |
| P-15 | Stale canonical state paragraph (Next boundary listing merged work as upcoming) contradicting the same file's body and the obligations index | 2026-09-19 cold boot findings | EPIST | typed `StateView` + reference-coherence invariants | `pit.state_references_resolve` | v3 |
| P-16 | Session checkpoint lagging two canonical merges (described accepted work as pending) | sessions/2026-09-18-qiven-v8 vs remote main e3dfe3b | PROC | checkpoint auto-write triggers (DR-012) | `pit.checkpoint_tracks_transactions` | v3 |
| P-17 | Missing session records (v2–v5) risk of synthesis | session README; constitution §5 | PROC | `EvidenceGap` typed absence | `pit.gaps_recorded_not_synthesized` | v3 |
| P-23 | Corruption/truncation accepted during restore | context-handoff contract | TRAN | typed deserialize `expected<Snapshot, DeserializeError>` (DR-009) | `pit.corrupt_artifact_fails_closed` | v3 |
| P-24 | Release-build assert compiles away; side-effect-in-assert incident | 0.1.4 roll Release-only assert bug; v2 source comment | TRAN | bounds checks are typed failures, never asserts; assert-free invariant tests | `pit.release_deserialize_safe` | v3 |
| P-25 | Canonical state self-pinning a live main SHA | MEM-20260913T172118Z-71F0AC | EPIST | `StateView` carries inventory only; live refs resolved at use time | `pit.no_live_refs_in_state` | v3 |
| P-29 | Event-sourcing replay temptation on recovery | ADR-0033 alt. 4; negative-knowledge record | TRAN | recovery materializes snapshots; readDelta is transport-only | `pit.recovery_never_replays` | v2 (partial), v3 |
| P-30 | Partial transaction visible mid-application | ADR-0033 §4 | TRAN | atomic multi-op transaction; successor-or-nothing | `pit.partial_transaction_never_visible` | v3 |
| P-31 | Timeout after possibly-successful commit treated as rollback | ADR-0033 §4; K2 suite | TRAN | `OutcomeUnknown` first-class; dependent work stops | `pit.timeout_yields_unknown_not_rollback` | v3 |
| P-32 | Same idempotency key reused with different content | K2 | TRAN | key bound to request digest; different content refused | `pit.same_key_different_content_refused` | v3 |
| P-36 | History overwritten in place | constitution §4; record-lifecycle | EPIST | lifecycle operations only; snapshots append-only | `pit.history_append_only` | v3 |
| P-37 | Lessons recorded with empty provenance; evidence records unreachable by any write path | constitution §9; v2 `AddMemoryRecord` apply case | EVID | typed provenance required by gate; `AddEvidence` operation exists | `pit.unprovenanced_record_refused` | v3 |

## Retrieval and economy

| ID | Pit | Canonical evidence | Class | Encoding | Test name | Status |
| --- | --- | --- | --- | --- | --- | --- |
| P-18 | Retrieval engine not invoked at material task transitions — operationally a miss | constitution §16; OBL-20260914T124259Z-7A4D13 | RETR | `RetrievalTrigger` typed; invocation tracked | `pit.transition_requires_retrieval` | Phase 4 |
| P-19 | Retrieval candidates treated as truth | context-read-contract; retrieval-candidate-acceptance schema | RETR | `Candidate` type; rehydration from snapshot; epistemic type preserved | `pit.candidate_not_truth` | v3 |
| P-20 | Budget pressure silently dropping mandatory inputs | ADR-0033 §6; R1 mandatory-input rules | RETR | floors non-negotiable; budget shrinks candidates only (DR-007) | `pit.bundle_floor_survives_budget` | v3 |
| P-21 | Byte compression claimed as token compression; hidden external dictionaries | ADR-0034 K5 section | RETR | K5 transport-only; decoder travels with artifact; pinned-tokenizer measurement contract | `pit.k5_transport_only` | v3 |
| P-27 | Silent long-running work mistaken for a hang; invented progress | MEM-20260913T194500Z-8F2C41; MEM-20260915T135800Z-6B0D8A | PROC | `TurnBudget`; liveness observable-state-only | `pit.liveness_reports_observables` | v3 |
| P-34 | Derived index promoted to truth; query silently reading a different snapshot | constitution §13; ADR-0033 §8 | RETR | bundle bound to one snapshot id; index lag is a diagnostic | `pit.index_never_canonical` | v3 |
| P-35 | Epistemic types collapsing (observation → invariant; CI green → acceptance) | constitution §8; MEM-20260916T020500Z-A27C91 | EPIST | `EpistemicType` axis; gates distinguish evidence from acceptance | `pit.observation_not_invariant` | Phase 4 |

## Environment, views, governance

| ID | Pit | Canonical evidence | Class | Encoding | Test name | Status |
| --- | --- | --- | --- | --- | --- | --- |
| P-13 | Remote cold boot mistaken for artifact-handoff proof | ADR-0034; pre-correction K4 challenge | EVID | trial topologies typed distinct (`ContinuityTrial` kinds); a cold boot cannot satisfy the artifact profile | `pit.cold_boot_not_artifact_trial` | v3 (spec-level) |
| P-38 | Durable profiles treated as live snapshots (paths, versions, VPN) | ADR-0035 verify_live; environment profile policy | ENV | profiles carry families; exact values verified via ports; never cached in tree | `pit.profiles_family_only` | v3 |
| P-39 | Genesis baking the governance principal into the tree as identity | R4; authority.yaml evolution rule | AUTH | governance amendments are gated operations; principal-as-identity verified live | `pit.governance_amendable` | Phase 4 |
| P-40 | View declarations excluded from context/export ("views are runtime parameters") | ADR-0033 §7/§10; operating model views/ surface; ContextView reference-integrity validation | EPIST | `ViewSpec` in tree; resolution at read; integrity checked (DR-008) | `pit.export_carries_view_specs` | v3 |
| P-41 | A participant combination assumed when the applicable view was uncertain | BOOTSTRAP step-10 rule (2026-09-18) | AUTH | view resolution falls back to identity-independent context; no invention | `pit.view_never_invented` | v3 |
| P-42 | The identity port re-entered the service while the admission mutex was held: same-thread re-lock of a non-recursive `std::mutex` threw `resource_deadlock_would_occur`; uncaught, it terminated silently (exit 3) and the CRT abort dialog paused unattended runs for human clicks | 2026-09-19 incident during v3 Phase 1: Debug ctest "hangs" were operator click latency on the MSVC abort-report dialog; stderr was empty, proving no QIVEN_ASSERT fired | AUTH | ports are called WITH the governance snapshot as context (`IIdentityVerifier::verify(actor, governanceSource)`); the service resolves context under its own lock via the locked helper; `RecordingVerifier` test | `pit.port_never_reenters_service` | v3 |

| P-43 | The v2 alias data race moved up a layer: `shared_ptr<const Materialization>` aliased a service-mutated object; `ReadFromCognition` raced the publish path | 2026-09-20 review §2 | AUTH | IMMUTABLE materializations: a write mints a SUCCESSOR materialization; the registry advances, published objects never mutate | `pit.reader_boot_preserves_writer` + pinned-world semantics | v3-2C |
| P-44 | `ContentId` doubled as snapshot-integrity hash AND storage-history token — no real storage (git) can chain on a content hash | 2026-09-20 review §3 | TRAN | `SnapshotDigest` / `RevisionId` / `GrantId` as distinct types; CAS on revisions, integrity on digests | `pit.golden_vector_pinned` + store contract tests | v3-2C |
| P-45 | `ExecutionGrant` was a public value type: a value-equal forgery borrowed the holder's verified identity | 2026-09-20 review §4 | AUTH | private constructor + move-only + minted `GrantId`; forgery is not expressible (static_asserts) | compile-time in `persistence_fencing.cpp` | v3-2C |
| P-46 | A store CAS failure and a lost acknowledgement were conflated — every CAS rejection would claim "possibly committed" | 2026-09-20 review §6 | TRAN | `StoreReceipt{Committed, CompareFailed, OutcomeUnknown}`; CompareFailed → StoreDiverged, lost ack → OutcomeUnknown receipt | `pit.timeout_yields_unknown_not_rollback` | v3-2C |
| P-47 | `IsContinueable` checked node health, not graph EDGES: mismatched client/llm/device bindings passed | 2026-09-20 review §9 | AUTH | edge validation: client→llm, client→device, llm→handle, binding-modelId == llm name | runtime rebirth test (generation B + mismatched-graph refusals) | v3-2C |
| P-48 | `Human::UseLLMToWork` hardcoded the serving model: a runtime rebind could drift the ADR-0035 disclosure while cognition stayed silent | 2026-09-20 review §10 | AUTH | disclosure derived from the ACTUALLY bound LLM (`pClient->pCurrentLLM->name`); `IsContinueable` validates binding↔LLM consistency | runtime rebirth test | v3-2C |

| P-50 | Publication swept unaccepted preserved commits onto main: the pointer commit sat on top of unpushed local main, and the merge published the whole stack | 2026-09-20 host publication incident (nine-repository audit finding 1) | PROC | pre-publication rule: `git log base..branch` MUST be reviewed and match the H2'd delta exactly | morning-review checklist + audit record | v3-2C |

## Explicitly judgment-only (no test will exist)

Recorded here so their absence from the suite is a decision, not an oversight:
whether a hazard class is "known" (§17); whether evidence is "genuinely
insufficient" (§11); review quality itself; relevance/ranking quality; the
judgment that a transaction is "material" (trigger list); spec-readiness for
worker handoff. See `invariant-inventory.md` §4.

## v4 activation ledger (second dimension)

The second dimension of "scars compile" (cognitive-boundary-model §39):
a remembered scar that is never activated can still be re-lived. Rows here
give pits their activation edge — the future condition that recalls the
lesson — and the compiled guard that makes forgetting fail a gate instead
of surviving to review. Derived from `activation-failure-inventory.md`
(A1-A9). v4-status rows name operational encodings outside this
repository; per-row test names are not used for them (P-50 precedent).

| ID | Pit (what happened) | Canonical evidence | Class | Activation trigger | Compiled guard | Status |
| --- | --- | --- | --- | --- | --- | --- |
| P-51 | Attribution block published AHEAD of the conventional subject — twice (fourteen commits 2026-09-19; eighteen across five repositories 2026-09-20), each requiring an owner-directed history rewrite with trees preserved | MEM-20260919T113238Z-B2F4D8; MEM-20260919T135930Z-F1C2A9; MEM-20260920T100100Z-A7D3E9 | PROC | ActionKind Commit: any LLM-authored commit or merge-message authoring | devkit tools/check_commit_subjects.py (selftested; HEAD-ancestry scan; wired into the devkit local gate). Reaches other repositories via the next operator roll — deliberately not a cross-repo callback (managed repos stay independently usable) | v4 |
| P-52 | A rename left dangling path references behind — twice the same day: a hardcoded pre-restructure test path masked by a scoped gate, and six stale refs behind an "all fixed" commit claim | activation-failure-inventory A3/A4; reconciliation PR #39; fix 46db19a | PROC | ActionKind ModifyReferencedContract / MakeCanonicalClaim: rename or move of a referenced path; completeness claims in commits | context tools/check_references.py (extension-bearing path references on normative surfaces must resolve; evidence surfaces and test fixtures scoped out with reasons; cross-repo citations exempt-listed; vacuity-proven by injecting the exact A4 path; wired into context-local and context-docs gates) | v4 |
| P-53 | Merge-class publication proceeded on a scoped gate: three morning merges published a broken test that the full gate would have caught; the full-gate-at-exact-head rule existed but was not recalled at merge time | activation-failure-inventory A5; PR #39 fix 46db19a; supervised-agent scoped-iteration rule | PROC | ActionKind Publish: any merge-class publication | devkit operator gate receipts + gate_proof builtin (devkit 146bc63, self-hosted; context snapshot 4f5cdef with merge-proof task); supervised-agent publish-boundary rule: missing receipt for the exact head is a stop condition. Fail-closed vacuity-proven (new head without receipt refuses) | v4 |

Back-filled activation edges for existing pits (their encodings are
unchanged; the trigger column is the v4 addition):

| ID | Activation trigger (v4) | Existing encoding |
| --- | --- | --- |
| P-18 | ActionKind BeginTask / task-domain transition: structural boundaries fire retrieval regardless of felt confidence | constitution §16; RetrievalTrigger enum |
| P-38 | ActionKind MakeLiveClaim: any live-state claim; durable profiles carry families only | environment-profile family rule; verify_live ports |
| P-44 | ActionKind ModifyReferencedContract: version bump of a pinned contract (the A9 positive control — dependents caught by compiled tests in minutes, not rewrites) | pit.golden_vector_pinned; serialization roundtrip tests |
| P-50 | ActionKind Publish: merge-class publication (delta identity + gate class at the same boundary; the gate-class half is backlog item 3) | morning-review checklist + audit record |

| P-54 | InvocationPolicy existed in types/tests but real genesis booted without it - a v4 chain was born without its activation policy | V4S plan S0-01; V4S-01 | PROC | ActionKind BeginTask/genesis: makeGenesis seeds default_invocation_policy; loaded policy never replaced | genesis policy assertions in persistence_fencing | v4S |
| P-55 | Missing control policy was DOCUMENTED fail-closed while executable behavior failed OPEN (empty derivation, ready()==true) | V4S plan S0-02/S1-05; control_plane World-A test | PROC | ActionKind any (governed): absent policy -> typed PreparationFailure, distinct from valid empty rule set | World-A + World-B tests in control_plane/persistence_fencing | v4S |
| P-56 | PreparationPacket::ready() conflated LISTED requirements with SATISFIED ones - a Publish packet read ready with nothing satisfied | V4S plan S0-03; control_plane readiness matrix | PROC | ActionKind Publish/Commit: derivation vs satisfaction split; readiness split by boundary | readiness-matrix tests in control_plane | v4S |
| P-57 | VerifyCanonical/InspectKnownPit routed through generic substring memory recall; prose similarity could satisfy blocking requirements | V4S plan S0-04; decoy tests | PROC | MandatoryRecall exact-key; InspectKnownPit via FailureFingerprint only; VerifyCanonical explicit-resolver-only | decoy + fingerprint-connection tests in control_plane | v4S |
| P-58 | ClaimClass existed on ActionIntent but never affected policy matching - decorative data | V4S plan S1-01 | EPIST | claim-scoped InvocationRule matching (absent = all classes) | claim-axis test in control_plane | v4S |
| P-59 | ToolContract.allowedFlags advertised flag pass-through the miniature never implemented - dead data with false comments | V4S plan S1-04 | PROC | ToolContract is a fixed argv template; advertise only implemented behavior | tool_and_retry contract tests | v4S |

Backlog (inventory §5): tool-contract invocation routing (A6); failure-
fingerprint batched evidence (A7); lower-layer search (A8). Merge-class
full-gate proof (A5/P-53) compiled 2026-09-20.
