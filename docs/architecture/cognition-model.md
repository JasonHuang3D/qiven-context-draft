# Cognition Model — v3 Semantics

**Status: candidate design input (ADR-0037).** Code blocks are design sketches
that state semantics; they are not the implemented draft API until the roadmap
phases land them. Prose contracts in the canonical repository remain normative.

Draft v2 proved the data model compiles. This document specifies the machine
around it: what the planes are, how state changes, who may change it, what
happens when a change is refused, and how a consumer reads without ever being
granted authority by the act of reading.

---

## 1. The four planes

Draft v1 conflated lifetime and authority; v2 split them. The deeper truth is
that the architecture has four planes with different physics:

| Plane | Question | Physics | Draft v2 home | v3 home |
| --- | --- | --- | --- | --- |
| **Content** | What is true? | immutable per state; pure values only | `LLMCognition` (mutable in place) | `Snapshot` — immutable value tree |
| **Authority** | Who may change it, when? | leases, fencing, fail-closed transitions | `epoch` + admission mutex | `ExecutionGrant` + policy table + actor port |
| **Presentation** | How does a consumer see it? | derived at read; never authority | full serialization | `ContextBundle` |
| **Durability** | Where does it live? | content-addressed, CAS, receipts | `ICognitionStore` | same contract + typed receipts |

The one-sentence architecture: **the Content plane is a chain of immutable
snapshots; every change is an authorized, atomic transaction; every refusal
names its recovery; reading produces evidence, never permission.**

## 2. Snapshot semantics (fixes v2's mutable handle)

v2 hands every participant a `shared_ptr<LLMCognition>` — a *mutable* handle to
canonical truth — and relies on convention not to use it. v2's own README claims
"single-writer is a type signature, not a policy"; the type signature says
otherwise, and the v2 test suite itself mutates cognition around the gate
(`persistence_fencing.cpp` setup code). v3 makes the claim true:

```cpp
// The value tree is immutable once minted. contentId and epoch are NOT tree
// members — they identify the materialization, so the tree stays a pure value
// (copyable, assignable, compressible) and no self-referential digest exists.
struct Snapshot {                 // pure value tree, v3 member set below
    Governance      governance;
    Constitution    constitution;
    PolicyTable     policy;       // authority rules as data (see §5)
    StateView       state;        // typed, coherent (see §7)
    std::vector<Decision>    decisions;
    std::vector<MemoryRecord> memory;
    std::vector<Obligation>   obligations;
    std::vector<Conflict>     conflicts;   // first-class epistemic state
    std::vector<EvidenceRecord> evidence;
    std::vector<ViewSpec>     views;       // durable participant adaptation
};

struct Materialization {          // runtime identity/authority metadata
    std::shared_ptr<const Snapshot> state;  // participants hold this — const
    ContentId contentId;                    // digest of the snapshot's bytes
    Epoch     epoch;                        // runtime fencing token
    QuarantineState quarantine;             // Isolated -> ... -> Promoted
};
```

Consequences, each traceable to a recorded pit:

- `LLM` holds `shared_ptr<const Snapshot>`; the mutable transition exists only
  inside the service. *"The LLM never holds a mutable reference"* becomes true at
  the type level, not by discipline. *(pit P-02, P-37 tests)*
- `contentId` stops being a mutable member the write path assigns into; it is
  derived and carried by the materialization. *(pit P-25)*
- `epoch` leaves the tree; the tree no longer carries runtime authority state.
  The const-member that broke assignability disappears.

## 3. The value tree member set (delta vs v2)

Kept from v2: decisions, memory, obligations, evidence, constitution,
governance, state, collaborations (now folded into `PolicyTable`).

Changed:

1. **`PolicyTable policy`** — the operation-class → required-handoff mapping and
   the refusal-reason → mandated-next-action mapping live here as data. R4
   ("authority rules live in cognition") stops being aspirational. *(§5)*
2. **`std::vector<Conflict> conflicts`** — conflict is first-class epistemic
   state with a lifecycle, not prose. *(§6)*
3. **`StateView state`** — typed instead of four opaque YAML strings:
   `objective`, `checkpointRef`, `candidateRef`, `nextBoundaryRef`, plus the
   repository inventory. Coherence invariants are gate-checkable: every
   reference must resolve inside the same snapshot. *(pit P-15)*
4. **`std::vector<ViewSpec> views`** — durable participant adaptation is
   canonical context (export must carry it; reference integrity must be
   checkable). The v2 claim "views are runtime parameters" is corrected:
   *bindings* are runtime, *view declarations* are context. *(§8; pit P-40)*
5. **`Constitution`** carries full article texts, not titles-only. The
   completeness principle (§9, DR-007) is: **completeness lives in the tree;
   economy lives in the Bundle.** v2's "full text stays canonical [somewhere
   else]" was the old ambiguity — under the value-tree rule, anything outside
   the tree is by definition not context, so titles-only made the constitution
   not-context.
6. **Records gain typed provenance and epistemic axes** *(§6)*.

## 4. Transactions: the only write shape

v2's single-kind `TransactionDelta` cannot express a real material transaction
(the ADR-0037 transaction touched seven surfaces in one commit), and its bool
return throws away the information every recovery rule needs.

```cpp
struct AuthenticatedActor {
    PrincipalId principal;        // verified by the identity port, §5
    Role        role;             // canonical package: owner / brother / worker
    BindingId   binding;          // which model instance fulfills the role
    ServingDisclosure disclosure; // serving model + declared-vs-served (ADR-0035 r4)
};

struct Operation {
    enum class Kind {
        AppendDecision, SupersedeDecision,      // lifecycle: reciprocal, acyclic
        AddMemory, TransitionMemory,            // status moves, history stays
        UpsertObligation, TransitionObligation, // trigger value carried; three-valued eval
        AddEvidence,                            // evidence records reachable at all
        UpdateState,                            // typed state fields
        AmendViewSpec,                          // reference integrity re-checked
        AmendGovernance,                        // authorized by the OLD policy (S10-R1)
        ResolveConflict,                        // the only op allowed beside an open conflict
    };
    Kind kind;
    // one typed payload union per kind; every payload carries provenance
};

struct H2Evidence {               // content-bound review evidence (S4-R1)
    Digest      reviewedDeltaDigest;  // over the serialized ordered operations
    PrincipalId reviewer;             // the delegated reviewer's principal
    std::string reviewRef;            // transport pointer: PR record / audit id
};

struct ContextTransaction {
    ContentId            base;        // durable fencing token (my thinking assumed this)
    AuthenticatedActor   actor;
    std::vector<Operation> operations; // atomic; no partial snapshot (S3-R1)
    IdempotencyKey       key;          // same key + same request = original result
    GovernanceVersion    governance;   // policy version the request assumed
    H2Evidence           h2;           // present iff the policy table requires it
};
```

## 5. The authority plane

Three mechanisms, each answering a different recorded failure:

**(a) The actor port.** `AuthenticatedActor` comes from an identity port whose
implementation verifies session claims against the governance principal *at
commit time* — never from a caller-constructed struct, never from "I am the
writer" pointer arguments. *(S3-R2, S7-R1; pit P-02, P-04)*

**(b) The execution grant.** Work begins by acquiring a grant; the grant is
re-checked at verdict time. A second flow requesting the same chain is refused
fail-closed and quarantined — never queued. Transport reachability and clean
trees confer no authority. Loss of the grant does not release it; stale grants
never regain authority. *(S7-R1, S7-R2; pit P-01, P-03)*

```cpp
struct ExecutionGrant { Epoch epoch; ActorId actor; WorkMode mode; };
// port: IExecutionAuthority::acquire(chainId, actor) -> grant | Refused
```

**(c) The policy table.** The ADR-0036 classification table becomes cognition
data, interpreted by the gate:

```cpp
struct HandoffPolicy {
    OperationClass opClass;         // e.g. merge-class = decision acceptance
    Handoff        required;        // H1..H4 or none
    bool           rootPrincipal;   // governance mutation class
};
```

`AmendGovernance` and `AmendViewSpec` are ordinary rows in this table. The gate
asks the table; nobody edits the if-chain. *(S4-R2, S10-R1; pit P-07)*

## 6. Verdicts: the write gate returns a typed outcome

```cpp
struct Verdict {
    enum class Outcome { Applied, Refused, OutcomeUnknown };
    enum class Reason {           // Refused only; each maps to a mandated action
        GrantRefused,             // -> halt; a competing flow may hold the chain
        UnverifiedActor,          // -> halt; identity is not self-asserted
        StaleBase,                // -> park delta, re-read, re-think
        HandoffMissing,           // -> halt and escalate; no re-attempt, ever
        HandoffInvalid,           // -> halt; evidence not bound to this delta
        UnattendedMutation,       // -> defer to supervised mode
        InvariantFailed,          // -> design review, not mechanical patch
        ConflictUnresolved,       // -> resolve the conflict first
        GovernanceDenied,         // -> stop; policy refuses this actor/op
        StoreDiverged,            // -> fail closed
        OutcomeUnresolved,        // -> stop dependent work; resolve via receipt
    };
    Outcome outcome;
    Reason  reason;               // meaningful when Refused
    ContentId successorId;        // meaningful when Applied
};
```

The **recovery table is cognition data** (`PolicyTable::recovery`), consumed by
the Work cycle: a `HandoffMissing` refusal *cannot* be retried because the rule
that forbids retrying it is the same record the gate consults. The v2 comment
"hold, re-read, re-think (not: retry the write)" becomes enforced semantics.
*(S9 all; pits P-11, P-37)*

`OutcomeUnknown` is a first-class outcome, not an error: a timeout after a
possibly-successful commit means dependent mutations stop until a receipt lookup
resolves the key. *(S3-R4; pit P-31)*

## 7. Conflicts and epistemics

```cpp
struct Conflict {
    ContentId id;
    enum class Status { Open, Resolved };
    std::string scope;                    // surfaces/records in contradiction
    std::string description;
    std::vector<RecordRef> contradicting; // reciprocal, acyclic
    std::optional<Resolution> resolution; // required when Resolved
};

enum class EpistemicType {   // constitution §8: these never collapse
    Observation, Hypothesis, Candidate, Accepted, Verified, Rejected
};

struct Provenance {          // constitution §9, typed (v2: vector<string>)
    std::vector<Source> sources;   // sourceType + reference + optional digest
    std::optional<ActorAssertion> recordedBy;  // explicit unknown ≠ synthesized
};
```

Gate rule: an acceptance-class operation is **refused while an open conflict
intersects its scope**, unless the transaction itself performs
`ResolveConflict`. This is the K4-trial-1 lesson (a resolvable canonical
wording conflict blocked acceptance) as a compiled gate. *(S8-R1; pit P-14)*

`MemoryRecord` keeps its `Kind` (genre: fact/lesson/risk/invariant/negative
knowledge) and gains `EpistemicType` — an observation is typed as an observation
forever until a transaction promotes it. *(pit P-35)*

## 8. Views: durable declarations vs live bindings

The v2 README claims views "are runtime parameters". The accepted contracts
disagree: `views/` is an active write surface, canonical exports must carry view
specifications, and view reference integrity is validated. v3 separates:

```cpp
struct ViewSpec {                 // IN the tree: durable, exported, integrity-checked
    ViewId id;                    // "zcode-jason"
    AgentFamily agent; HumanId human;
    std::vector<ProfileRef> preferenceProfiles;   // must resolve in-snapshot
    std::vector<ProfileRef> environmentProfiles;  // families only; exact values live
    std::vector<ProfileRef> workflowProfiles;
    std::vector<BindingSpec> bindings;            // role -> model + qualification evidence
};
struct ParticipantBinding {       // RUNTIME: the live instance of one session
    ViewId view; Role role; BindingId binding; ServingDisclosure disclosure;
};
```

View *resolution* (ProjectSnapshot + ViewSpec + live capability assertions →
ResolvedContextView) is a read-time pure transformation; an absent or
non-resolving view falls back to the identity-independent context, never an
invented adaptation. *(S1-R2; pit P-40)*

## 9. The read plane: ContextBundle

Reading is where token economics and correctness meet: drift correlates with
*economy pressure* — models that skip re-reading, or re-read stale surfaces. The
bundle makes correct reading cheap and floors non-negotiable.

```cpp
struct ContextQuery { TaskScope scope; HistoryMode history; TokenBudget budget; };

struct ContextBundle {
    SnapshotId   snapshot;                  // one snapshot; never mixed
    std::vector<Section> mandatoryInputs;   // S1-R3 floors: governance, constitution,
                                            // state, active work, open obligations,
                                            // relevant contracts, recent checkpoint
    std::vector<ProtectedConstraint> constraints; // handoff table, waiver ban,
                                            // DCR suspension — verbatim, never elided
    std::vector<Candidate> candidates;      // typed candidates; rehydrated payloads;
                                            // epistemic type preserved end-to-end
    std::vector<ObligationEvaluation> obligations; // three-valued: met/unknown/unmet
    std::vector<Omission>  omissions;       // what was left out and why
    Diagnostics diagnostics;                // index lag, unknowns, abstentions
    Authorization authorization = Authorization::NotGranted;  // always
};
```

Rules: budget pressure shrinks only `candidates` — floors and constraints are
non-negotiable *(pit P-20)*. Candidates never become truth by being retrieved
*(P-19, P-34)*. Rejected alternatives relevant to scope are part of the floors —
negative knowledge must be surfaced where a pit is about to be re-dug *(S2-R2)*.
Retrieval invocation is itself typed: `RetrievalTrigger { ColdBoot,
TaskTransition, DomainChange, MaterialTransaction, SessionRollover }` —
constitution §16 compiles *(pit P-18)*.

**K5 stays a transport concern.** `CognitionSourceKind::CompressedStream`
remains the only place compression exists; `tokenBudget` moves from Query to
Bundle semantics. v2's comment "budget enforcement is K5's domain" conflated
lossless transport with lossy task-bounded selection — the exact confusion
ADR-0034 forbids. *(pit P-21)*

Human/machine dual views: bundle rendering has `OutputView::Human | Machine` —
separate views of one execution result, no scraping across audiences; the
owner-run K4 producer's human-view requirement is a downstream instance of this
rule.

## 10. The durability plane

`ICognitionStore` keeps v2's shape and gains typed outcomes:

```cpp
struct Receipt { enum class State { Committed, OutcomeUnknown }; ContentId id; };
[[nodiscard]] virtual Receipt compareAndSwap(const ContentId& base,
                                             const Bytes& successor,
                                             const IdempotencyKey& key) = 0;
```

- Recovery never replays: reads materialize a snapshot; diffs are transport
  optimizations only. *(state-replication; pit P-29)*
- Same key + same request → the original committed result; same key + different
  request → refused. Keys and receipts are durable for retained history. *(K2)*
- Deserialization of untrusted bytes returns
  `expected<Snapshot, DeserializeError>` with kinds `Truncated, BadVersion,
  BadEnum, LengthOverflow, DigestMismatch, ResourceAbuse` — never an assert that
  compiles away in Release. Bounds are checked before `reserve`; resource abuse
  fails closed. *(S6-R3; pits P-23, P-24)*
- Golden vectors pin the serialization; absent/empty are distinguished; Unicode
  and numeric rules are explicit — the K1 serialization contract, inherited.
- `GitStore` remains a documented sketch; `MemoryStore` remains the contract
  proof; a restart-capable test adapter is the only thing allowed to claim
  durable receipts. *(K2 lesson)*

## 11. The runtime plane: participants and session economics

Participants stay a pure pointer graph (v2 got this right): every participant
change is an O(1) rebind. The Work cycle is typed end to end:

```cpp
struct WorkReport {
    enum class Phase { Booted, Read, Thought, Acted, Wrote, Parked, Refused, Halted };
    Verdict verdict;                  // when a write was attempted
    std::optional<TurnUsage> usage;   // real observables only
};
// LLM::Work acquires grant -> bundle read -> think -> propose transaction
//   -> verdict -> per-recovery-table next action -> checkpoint update
```

Session economics — the process pits, compiled:

- `TurnBudget { soft, hard }` — the observed ~26-minute Chat tool-turn boundary
  is the budget that makes checkpoint discipline non-optional. *(pit P-27)*
- Checkpoint auto-writes at: turn boundary, material transaction, async exit,
  session close. The checkpoint gains `std::vector<EvidenceGap>` (typed absence;
  never synthesized) and a required `ServingDisclosure`. *(pit P-16, P-17)*
- Liveness reports observable state only — no invented percentages or ETAs;
  heartbeat cadence scales with expected duration. *(pit P-27; MEM-8F2C41)*
- Tool waits are bounded by contract (dispatch ≠ completion; three observations
  / 60 seconds default; no sleep/poll loops). *(pit P-05 adjacent; session-ci
  handoff contract)*

## 12. Promotion: the quarantine state machine

```cpp
enum class QuarantineState { Isolated, Verified, AuthorityPending, Promoted };
```

A restored instance may advance `Isolated → Verified` by passing integrity and
completeness validation, and may advance `AuthorityPending → Promoted` only
through a governed `PromoteAuthority` operation naming the cutover snapshot —
never by checksum success, successful import, or uptime. *(S6-R2, S10-R2; pit
P-33)*

## 13. Deliberately out of scope

No daemon, no networking, no serialization libraries, no YAML parser, no
storage-product commitment, no distributed consensus, no plugin framework. The
draft proves architecture, not transport. The judgment-only invariants are
listed honestly in `invariant-inventory.md` §4 — they are pinned by decision
records and mandated prompts, not faked as types.
