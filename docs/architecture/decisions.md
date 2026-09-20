# Design Decisions — with Rejected Alternatives (Negative Knowledge)

**Status: candidate design input (ADR-0037).** Negative knowledge is first-class
(constitution §7): every decision here records what was rejected and why, so a
future session does not re-dig a filled pit. Each decision names the test that
pins it. Draft-level IDs (`DR-*`); canonical recording back to the ADR process
happens through `OBL-20260918T215000Z-B4D6A8` when validated.

---

## DR-001 — Snapshots are immutable; mutation produces a successor

**Context.** v2 mutates the tree in place and hands participants a mutable
handle; the README's "single-writer is a type signature" claim is false at the
type level, and `contentId` is a self-referential mutable member.

**Decision.** `Snapshot` (pure value, no epoch/contentId members) +
`Materialization { shared_ptr<const Snapshot>, contentId, epoch, quarantine }`.
Only the service performs transitions; participants hold `const`.

**Rejected alternatives.**
- *Keep in-place mutation, add a const wrapper.* Rejected: const-correctness
  through `const_cast` discipline is policy, not type; the pit (P-02) is that
  policy erodes.
- *Deep-freeze by convention (code review only).* Rejected: this corpus exists
  because convention erodes.

**Consequences.** K4/K5 snapshot semantics become trivial (a snapshot *is* the
unit); copy-assignability restored (no const member); transaction atomicity is
expressible as "produce the successor or nothing".

**Pinned by.** `pit.participant_handle_is_const` (compile-time: LLM holds
`shared_ptr<const Snapshot>`).

## DR-002 — The write gate returns a typed verdict; recovery is data

**Context.** v2's `bool` erases the refusal reason; the recovery rules ("park,
re-read, re-think, never retry a handoff-missing write") live in comments.

**Decision.** `Verdict { Applied | Refused{Reason} | OutcomeUnknown }`; the
reason → mandated-next-action table is a cognition member (`PolicyTable`),
consumed by the Work cycle.

**Rejected alternatives.**
- *Enum return + separate switch in the caller.* Rejected: the process rule
  would live in runtime code again — R4 requires rules in cognition.
- *Exceptions.* Rejected: refusal is a normal, expected outcome with mandated
  next actions, not an exceptional failure; the draft's engineering law keeps
  exceptions out of non-throwing contracts.

**Consequences.** Every failure-response rule in `process-model.md` §3 becomes
gate-enforced; a refactor that drops a recovery rule fails the corresponding
pit test.

**Pinned by.** `pit.refusal_reason_carries_recovery`, `pit.handoff_missing_never_retried`.

## DR-003 — Identity comes from a port; capability never implies authority

**Context.** v2's writer asserts identity by passing the very pointer it wants
validated — the exact anti-pattern ADR-0026 names ("I am the only executor").

**Decision.** `AuthenticatedActor` is produced by a live identity port and
re-checked at commit; the Work cycle begins with an `ExecutionGrant` acquired
from the authority port; competing flows are refused fail-closed.

**Rejected alternatives.**
- *Pass actor in the transaction struct as plain data.* Rejected: caller-
  constructed identity is caller-asserted identity; the split-brain incident is
  precisely two flows each believing their own assertion.
- *Rely on the admission mutex only.* Rejected: the mutex serializes writes but
  does not identify writers; two sessions alternating cleanly still violate
  single-writer intent.

**Consequences.** Authority transitions persist on refusal (quarantine is a
state machine, not a flag); transport reachability confers nothing.

**Pinned by.** `pit.split_brain_second_flow_refused`, `pit.unverified_actor_refused`,
`pit.rejected_flow_stays_fenced`.

## DR-004 — Review evidence is content-bound

**Context.** v2's `handoffEvidenceRef` is a free string; evidence survives
unnoticed payload changes; ADR-0036 requires H2 over "the exact delta".

**Decision.** `H2Evidence { reviewedDeltaDigest, reviewer, reviewRef }`; the
gate compares the digest against the serialized ordered operations of this
transaction.

**Rejected alternatives.**
- *Bind to the base contentId only.* Rejected: many deltas share a base; the
  review covers the delta, not the starting point.
- *Trust the PR record's existence.* Rejected: the pointer-vs-content mistake
  is MEM-20260913T162546Z-E73124's whole lesson.

**Consequences.** A payload edited after review fails the gate as
`HandoffInvalid`; "review then silently rebase" is structurally dead.

**Pinned by.** `pit.h2_evidence_rebind_refused`.

## DR-005 — The handoff policy table is cognition data (R4 made true)

**Context.** v2 hardcodes "AppendDecision → H2" and "unattended → read-only" in
service code while claiming "authority rules live in cognition".

**Decision.** `PolicyTable::handoff` holds the operation-class → required-
handoff mapping (from ADR-0036's classification table, including the root-
principal governance class); the gate interprets it.

**Rejected alternatives.**
- *Hardcode per kind with a comment citing ADR-0036.* Rejected: that is v2
  status quo; changing policy would recompile the spec — the spec is not the
  spec.
- *Per-operation virtual hooks.* Rejected: virtual dispatch inside the gate
  multiplies authority surfaces; a table is auditable in one place.

**Consequences.** Governance evolution (S10) becomes an ordinary gated row
amendment; the no-verbal-waiver property has no code path to weaken.

**Pinned by.** `pit.merge_class_without_h2_refused`, `pit.handoff_has_no_waiver_path`.

## DR-006 — Open conflicts block acceptance-class operations

**Context.** K4 trial 1 failed because a resolvable canonical wording conflict
was discovered in Phase B — after the producer gate had passed. The rule
"Project Continuity forbids accepting a known resolvable canonical conflict"
had no mechanical form.

**Decision.** `Conflict` is a first-class record with an Open/Resolved
lifecycle; the write gate refuses acceptance-class operations whose scope
intersects an open conflict, unless the transaction performs the resolution.

**Rejected alternatives.**
- *Warn only.* Rejected: warnings are how the conflict reached Phase B.
- *Global freeze on any open conflict.* Rejected: scope-intersection is the
  actual semantic; a global freeze would let one unrelated conflict stop all
  work — a denial-of-service the other direction.

**Consequences.** Conflict must be representable everywhere acceptance is;
`ResolveConflict` becomes an ordinary operation with its own provenance.

**Pinned by.** `pit.open_conflict_blocks_acceptance`.

## DR-007 — Completeness lives in the tree; economy lives in the Bundle

**Context.** v2 stores constitution titles only ("full text stays canonical"),
which by the value-tree rule places the full text outside context; and v2's
`Query.tokenBudget` comment assigns budget to K5 — conflating lossless
transport with lossy selection.

**Decision.** The tree carries full semantic texts (bounded, stable); the
Bundle performs selection under budget with non-negotiable floors; K5 is
transport-only.

**Rejected alternatives.**
- *Titles + external references for full texts.* Rejected: reintroduces
  "canonical-by-nowhere"; the draft repo itself was titled-only for exactly the
  drift reasons this corpus exists.
- *Budget-aware tree reads.* Rejected: two selection mechanisms (tree read and
  bundle) would drift apart; selection is the Bundle's single job.

**Consequences.** K4's export carries everything semantically needed; token
economy is a read-time property, measurable and tunable without touching truth.

**Pinned by.** `pit.bundle_floor_survives_budget`, `pit.k5_transport_only`.

## DR-008 — View declarations are context; bindings are runtime

**Context.** v2 claims views "are runtime parameters"; the accepted contracts
make `views/` an active write surface, require exports to carry view specs, and
validate view reference integrity.

**Decision.** `ViewSpec` (durable, in tree, reference-integrity-checked,
exported) vs `ParticipantBinding` (live session instance, runtime only).
Resolution is a read-time pure transformation with identity-independent
fallback.

**Rejected alternatives.**
- *Everything runtime (v2).* Rejected: contradicts the export contract; the
  kernel would be unable to express what K4 must deliver.
- *Everything canonical.* Rejected: live serving-model identity and runtime
  capabilities are session facts; freezing them into truth recreates the
  stale-profile problem.

**Consequences.** "Verify live" stays live; durable adaptation becomes
exportable and integrity-checkable; view bootstrap (S1-R2: never invent a
participant combination) becomes a resolution rule, not a protocol paragraph.

**Pinned by.** `pit.export_carries_view_specs`, `pit.view_refs_resolve`.

## DR-009 — Untrusted deserialization fails closed, typed

**Context.** v2 guards the deserializer with `QIVEN_ASSERT`, which compiles to
nothing in Release; a corrupted artifact path (exactly the K4 restore path)
would read out of bounds or reserve attacker-controlled memory.

**Decision.** `expected<Snapshot, DeserializeError>` with typed kinds;
pre-reserve bound checks; digest verification before construction; golden
vectors.

**Rejected alternatives.**
- *Keep asserts, add a Release test build.* Rejected: the production restore
  path is Release; the failure class must be unreachable there, not observed
  there.
- *Exceptions.* Rejected: corruption is an expected environmental input with a
  mandated response (fail closed), not a control-flow surprise.

**Consequences.** "Corruption/truncation is accepted" becomes mechanically
impossible; resource abuse fails closed (K5's gate, inherited early).

**Pinned by.** `pit.corrupt_artifact_fails_closed`, `pit.resource_abuse_fails_closed`.

## DR-010 — The epoch fences writer identity, not materialization count

**Context.** v2 bumps a global epoch on every `CreateCognition`; any read-boot
permanently revokes every prior instance's write authority — a reader kills a
writer. The durable base-CAS already handles stale writes.

**Decision.** The grant (DR-003) fences *who may write the chain now*;
`epoch` becomes the grant's fencing token in the authority plane; re-
materializations for reading or views never touch write authority.

**Rejected alternatives.**
- *Drop epoch, keep only base-CAS.* Rejected: CAS detects divergence after
  thinking; the split-brain cost was paid *during* thinking — the grant stops
  two flows before the wasted turns, which is the incident's actual lesson.
- *Per-instance epochs.* Rejected: fencing must be comparable against a single
  authority plane to detect staleness.

**Consequences.** Cold-booting a ContextView consumer no longer de-authorizes
an Operator session; writer succession is explicit.

**Pinned by.** `pit.reader_boot_preserves_writer`, `pit.stale_grant_never_regains`.

## DR-011 — Outcomes include `OutcomeUnknown`; receipts are durable

**Context.** ADR-0033 §4's hardest-won semantics: a timeout after a possibly-
successful commit is not a rollback; dependent work stops; same-key retry
resolves to one durable outcome.

**Decision.** Store `compareAndSwap` returns `Receipt { Committed,
OutcomeUnknown }`; the Verdict surfaces `OutcomeUnresolved`; a restart-capable
test adapter is the only adapter allowed to claim durable receipts.

**Rejected alternatives.**
- *Treat unknown as failure and retry.* Rejected: retry-after-unknown is how
  double-application happens; this is K2's core negative fixture.
- *MemoryStore-only proof.* Rejected: memory-only tests cannot claim crash
  recovery (K2 acceptance boundary).

**Consequences.** The draft can host K2's race/failure-injection suite later
without redesign.

**Pinned by.** `pit.timeout_yields_unknown_not_rollback`, `pit.same_key_one_outcome`.

## DR-012 — Session economics are part of the contract

**Context.** The observed ~26-minute Chat tool-turn boundary, the turn-economy
reliability constraint, and the truthful-liveness rule are recorded lessons
with no mechanical form in v2; checkpoints lag merges in practice.

**Decision.** `TurnBudget`, checkpoint auto-write triggers (turn boundary,
material transaction, async exit, session close), typed `EvidenceGap`, required
serving-model disclosure, liveness reporting observable state only, bounded
tool waits.

**Rejected alternatives.**
- *Leave checkpoint writes to the model's discretion.* Rejected: discretion
  under token pressure is the failure mode; the v8 checkpoint lag is the
  demonstration.
- *Fake progress percentages to reassure the operator.* Rejected: MEM-8F2C41
  explicitly forbids inventing observables.

**Consequences.** A turn incident loses bounded work by construction; the
checkpoint is written by the loop, not remembered by the model.

**Pinned by.** `pit.checkpoint_survives_turn_loss`, `pit.gaps_recorded_not_synthesized`.

## DR-013 — Ports receive context; they never re-enter the service

**Context.** The 2026-09-19 incident (P-42): the identity port was invoked
while the service held the admission mutex, and the draft verifier re-entered
`CanonicalHead()`, re-locking the same non-recursive mutex on the same thread.
MSVC Debug throws `resource_deadlock_would_occur`; uncaught, the process
terminated silently (exit 3) behind a modal CRT abort dialog that paused
unattended runs for human clicks. MSVC Release silently tolerates the re-lock
(recursion counter) — the same defect, config-divergent manifestation, and
standards-level undefined behavior either way.

**Decision.** Ports are called with the context they need. `IIdentityVerifier::
verify(actor, governanceSource)` receives the governance snapshot the service
resolved under its own lock (internal locked helper); no port signature can
reach back into the service.

**Rejected alternatives.**
- *Make the admission mutex recursive (`std::recursive_mutex`).* Rejected: the
  re-entrancy is the defect; hiding it behind a recursive lock admits arbitrary
  callback cycles and resurrects the split-brain class the mutex exists to
  fence.
- *Catch the system_error in the verifier.* Rejected: catching the symptom of
  re-entrancy keeps the cycle; the throw is the detector, not the disease.

**Consequences.** Every future port (identity, authority, retrieval backends)
gets its context as parameters; a port that needs more must say so in its
signature, which makes the dependency edge reviewable.

**Pinned by.** `pit.port_never_reenters_service` (`RecordingVerifier`).

## DR-014 — Three orthogonal identities

**Context.** `ContentId` doubled as the snapshot-integrity hash and the
storage-history CAS token; no real storage (git) can chain a commit on a
content hash (review §3).

**Decision.** `SnapshotDigest` (integrity), `RevisionId` (storage history),
`GrantId` (authority capability) are distinct types. CAS compares revisions;
integrity compares digests.

**Rejected.** One umbrella `ContentId` with conventions — conventions erode;
the K4 lesson again.

**Pinned by.** store-contract tests; golden digest assertions.

## DR-015 — Materializations are immutable; writes mint successors

**Context.** The v2 alias race survived the v3 split: the service mutated the
same `Materialization` object participants held as `const` (review §2).

**Decision.** A write mints a successor `Materialization`; the registry
advances; published objects never mutate; a pinned handle is a consistent
world for a whole Work cycle.

**Rejected.** Recursive locks or copy-on-read — both hide the race instead of
removing it.

**Pinned by.** reader-boot-preserves-writer + pinned-world assertions; rebirth
test.

## DR-016 — Grants are minted capabilities

**Context.** A value-equal forged grant borrowed the holder's verified
identity (review §4).

**Decision.** Private constructor, move-only, minted `GrantId` validated
against the active lease.

**Rejected.** HMAC-style grant MACs — cryptographic theater for an in-process
capability; the type system suffices at this layer.

**Pinned by.** compile-time static_asserts in `persistence_fencing.cpp`.

---

## DR-017 — Roles, designations, and bindings are three constructs, not one

**Context.** Canonical practice uses canonical roles (owner/brother/worker),
session designations (jason-extended-cognition), and bindings (ADR-0035)
interchangeably under the word "role". The draft types only the first (a
closed enum) and half of the third; designations exist only in prose.
`jason-extended-cognition` does not fit the enum and force-fitting it would
freeze a session-scoped adaptation into permanent authority topology.

**Decision.** Three-tier separation (`participant-model.md` §3):
`RoleSpec` is cognition data — authorities, duties, capability-class floor,
ratification provenance — participant-independent by construction; the
canonical set stays closed at the type level, widened only by governance
transaction + code change. `SessionDesignation` is a runtime sidecar:
owner-granted per session, never self-assigned, with the structural
invariant authority-delta = ∅ (process widening only). Bindings name
implementations and create no authority (per DR-008/ADR-0035). An LLM may
propose a RoleSpec; only the owner ratifies one — a participant cannot
enlarge the space of authorities it may hold (self-certification ban class).

**Rejected.** (a) Runtime-mintable free-form RoleId — authority forgery
with extra steps; the 2026-09-20 publication-sweep incident is the failure
class it multiplies. (b) extended-cognition as a fourth enum member —
violates its own session-scoped definition. (c) Designation as a view-only
concept with no runtime type — grants become unauditable in the loop.

**Pinned by.** designation authority-delta construction test (Phase 5);
RoleSpec provenance integrity checks; I-PM3.

## DR-018 — Workflow profiles are layered projections, not participant-bound documents

**Context.** Workflow docs accreted four layers (canonical law,
capability-class procedure, role choreography, environment ergonomics) in
one file, developing undeclared dependencies on nearly every participant
axis; copies of canonical law inside workflows silently diverge
(stale-reference cleanup class, 2026-09-20).

**Decision.** Four-layer model with lawful homes and a per-sentence
substitution test (`participant-model.md` §4). Workflow profiles are
read-time pure DERIVED artifacts — projections of policy × capability
class × environment class × human preferences — same resolution shape as
views (S1-R2): never authority, never invented adaptation. Forbidden
edges: workflow → concrete model instance; workflow → named human beyond a
preference profile; workflow → authority production (consume policy,
choreograph gates, never mint or waive).

**Rejected.** (a) Splitting every workflow into four physical files —
ceremony without checkability; the classification test is per-sentence,
not per-file. (b) Promoting all ergonomics to canonical surfaces —
device-specific path facts do not belong in identity-independent truth.

**Pinned by.** I-PM4 scoped lint; workflow-resolution purity test.

## DR-019 — Participant dependency is declared and auditable, not assumed

**Context.** Independence existed as slogans ("project truth is
identity-independent") but never as an auditable surface; the owner's 2026-
09-20 question demanded the full produce/consume/authorize audit.

**Decision.** The participant dependency matrix (`participant-model.md` §5)
is corpus law: authority has exactly one source (the human principal
through policy); every other participant class is a capability, a channel,
or a safety control. The I-PM invariant set (I-PM1..6) makes independence
checkable, with explicit SCOPING: evidence, audits, qualification records,
and state lawfully name instances; normative surfaces must not.

**Rejected.** Treating independence as a single bit — it is per-axis; a
role is instance-independent and exercise-dependent at once, and the
workflow layer test is per-sentence. Unscoped "no instance names anywhere"
lints — they over-fire on lawful provenance and get ignored.

**Pinned by.** I-PM scoped lints; I-PM6 bundle blindness test.
