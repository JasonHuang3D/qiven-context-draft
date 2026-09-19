# Memory Infrastructure Analysis — should the draft adopt Foundation memory primitives?

Status: analysis (2026-09-20, overnight run). Question posed by the project
owner: the draft uses `shared_ptr` and raw pointers heavily, which loses
precise semantics; Foundation has memory infrastructure — is it necessary to
use it directly?

## 1. The three pointer populations in the draft

| Population | Type today | Meaning |
| --- | --- | --- |
| Pinned cognition handles | `shared_ptr<const Materialization>` / `shared_ptr<const Snapshot>` | Shared, IMMUTABLE, long-lived; multiple participants pin the same world for a whole Work cycle |
| Participant references | raw `Human*`, `LLM*`, `Device*`, `LLMClientTool*` (function params) | Non-owning observers passed into the whole-graph predicate; never own, never outlive the graph |
| In-flight objects | `shared_ptr<LLM>`, `shared_ptr<Device>` in the runtime graph | Owning edges of the participant pointer graph (R3 rebinds swap these) |

## 2. Verdict per population

### 2.1 Pinned cognition handles — keep `shared_ptr<const T>`

The semantics are genuinely SHARED ownership of an immutable value: several
participants (and the service registry) pin the same materialization for the
duration of a Work cycle, and after a write the old world must stay alive for
every pin issued before it. Foundation's `owned_*` primitives model EXCLUSIVE
ownership with explicit destruction; an arena RESET would invalidate every
pinned view — which is precisely the alias bug class the v3 phase 2C fix
removed (pit P-43). `shared_ptr<const T>` is the accurate statement of this
contract: shared, immutable, lifetime pinned by use.

### 2.2 Participant references — the real imprecision; Foundation candidate F3

The raw `T*` parameters are non-owning observers, but nothing in the type says
so: a `T*` could own, could alias, could be dangling. This is the owner's
"loses precise semantics" observation, and it is correct. The draft already
validates edges at runtime (`IsContinueable` graph-edge checks, review §9),
but the TYPE could say it first.

**Foundation candidate F3** (deferred, one consumer today): a non-owning
observer vocabulary — `qiven::observer<T>` (nullable, no implicit ownership,
no arithmetic) — so predicate signatures read
`IsContinueable(observer<const Human>, observer<const LLMClientTool>, ...)`.
Trigger for landing: a second consumer (the Operator/Host layer passing
participant graphs) or the K3 view pipeline, whichever first. Until then the
raw pointers stay, with the runtime edge checks carrying the weight.

### 2.3 Arena scratch for Bundle construction — adopt at K3

`ContextBundle` construction allocates many short-lived strings and candidate
structures per query, with no sharing after the query completes. This matches
Foundation's `memory::LinearArena` design (bulk allocation, wholesale reset).
The draft does not adopt it now (the toy bundle is allocation-light), but the
kernel's K3 bundle pipeline should: arena-per-query, reset on completion.
Recorded as design input to the kernel batch, not a draft change.

## 3. What Foundation must NOT absorb from this analysis

`Snapshot`/`Materialization`/`Verdict`/`PolicyTable` are cognition-domain
semantics. The memory question was about the POINTER VOCABULARY, not the
types: shared-immutable, non-owning-observer, and arena-scratch are the three
memory disciplines; each maps to one Foundation or domain mechanism and none
is interchangeable with another.

## 4. Summary of decisions

| Decision | Where |
| --- | --- |
| Keep `shared_ptr<const T>` for pinned cognition | draft (unchanged) |
| Observer vocabulary F3: deferred, trigger = second consumer / K3 | foundation distillation doc |
| Arena-per-query for bundle construction: adopt at K3 | kernel design input |
| Runtime graph-edge checks carry correctness until F3 | draft `IsContinueable` (landed, review §9) |
