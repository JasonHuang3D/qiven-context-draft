# qiven-context-draft

An **executable C++ specification** of the qiven-context cognition architecture.
This repository is the semantic bridge between human and LLM: natural language
always produces mutual interpretive drift, while pointer/value choices, types and
call graphs state ownership, lifetime and causality exactly. Here, architecture
rules compile.

**Status: executable specification through v4.6, post-closeout v4
stabilization (V4S) complete.** v3 persistence/authority and v4
cognition-activation semantics are accepted design material. This repository
is still a specification/reference implementation, NOT the production Context
runtime: the runnable cognition_loop demonstrates the v3 causal loop; v4
Cognitive Control has executable semantic proofs but is not yet wired into a
production work/execution loop (Runtime ADL owns that integration, the real
symbol index, the live verifier, and the production tool registry). The next
boundary is Runtime ADL against the frozen v4 semantic contract.

## Why C++ as the bridge

Any LLM maintaining project context in prose will drift — prose boundaries erode
with every edit. Types do not drift. A value member versus a pointer member is a
machine-checkable statement about ownership and lifetime:

- `Snapshot` is a **pure value tree** — zero pointers to runtime participants.
  That single rule makes it snapshot-able, restorable and compressible (K4/K5),
  and doubles as the drift detector: any repo surface that cannot be pointed at
  as a member of `Snapshot` is by definition not context (session bindings,
  device profiles and resolved views are runtime parameters; DURABLE view and
  profile declarations ARE context — DR-008).
- **Three orthogonal identities** (review §3): `SnapshotDigest` (integrity of
  the canonical bytes), `RevisionId` (storage history identity — a GitStore
  mints commit OIDs), `GrantId` (runtime authority capability). Never shared.
- `Materialization` is **immutable** (review §2): a write mints a successor;
  a pinned handle is a consistent world for a whole Work cycle.
- `ExecutionGrant` is an **unforgeable minted capability** (review §4):
  private constructor, move-only; forgery is not expressible.
- `WriteToCognition` returns a **typed `Verdict`** whose refusal reason maps to
  a recovery rule stored IN cognition (recovery-as-data). The store returns
  typed receipts: a CAS failure and a lost acknowledgement are different worlds.
- The `ContextBundle` carries **semantic floors** (constitution articles,
  governance, open obligations, negative knowledge, boundaries) that survive any
  token budget; the budget shrinks typed candidates only, with omissions
  explained, and every bundle is stamped `authorization: not_granted`.

## The persistence question (why git, and why git is optional)

The repository stores the **serialized `Snapshot` itself**. The entire
persistence requirement is one small contract — any data storage with
compare-and-swap on revisions suffices:

```cpp
struct ICognitionStore {
    StoreReceipt compareAndSwap(const RevisionId& base, const Bytes& state); // typed CAS
    Bytes       materialize(const RevisionId& revision) const;
    bool        verify(const RevisionId& revision) const;
    RevisionId  head() const;
};
```

`qiven-context` is **state-replication, not event-sourcing**: recovery reads the
current canonical snapshot and never replays diffs or commit graphs.

Git is today's transport implementation, nothing more:

| Store contract | git implementation |
| --- | --- |
| `compareAndSwap(base, state)` | commit with parent = base (RevisionId = commit OID); CompareFailed = non-fast-forward |
| `materialize(revision)` | checkout / cat-file at that OID |
| `verify(revision)` | cat-file -e |
| `head()` | rev-parse HEAD |

Layered **transport ceremonies** are not part of the store contract: push/pull =
replica sync; PR + review + merge = the H2 acceptance ceremony over the human-AI
channel; **GitHub is a second device** (a cloud device running CI) that
validates appended states, with the evidence flowing back into cognition through
ordinary gated transactions.

## Architecture rules (R1–R6)

1. `Snapshot` is a pure value tree — zero pointers to runtime (drift detector).
2. Cognition swap is atomic with respect to `Work()` — the writer lease and the
   base revision fence; lock the admission, never the work.
3. Participant change (model / tool / device / human) is a pointer rebind — O(1),
   never a cognition write — and the runtime authority layer validates the
   rebound graph (edges + disclosure) without changing cognition.
4. Authority **rules** live in cognition as data (`PolicyTable`: operation class
   → required handoff; refusal reason → mandated recovery); identities are
   session parameters verified against the governance principal.
5. `Work()` side effects = result + gated transaction + tool calls. Nothing else.
6. Restore is pure materialization + runtime rebind; only the delta write is
   gated, and a restored instance is quarantined until a governed cutover.

## Layout

```
include/qiven/context/cognition.hpp     value tree: records, governance, policy, views
include/qiven/context/persistence.hpp   ICognitionStore, Materialization, ExecutionGrant,
                                        QivenContext (grants, gates, bundles)
include/qiven/context/runtime.hpp       Device, RemoteDevice, LLM, LLMClientTool, Human
src/                                    persistence (serialization, gates) + runtime
apps/cognition_loop.cpp                 runnable demo of the whole loop
tests/                                  value-tree checks, fencing/pit suite, store contract
docs/architecture/                      the design corpus (see its README)
```

## Build (Windows / VS2022, mirrors qiven-math conventions)

```cmd
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug
ctest --preset vs2022-x64-debug
apps\cognition_loop Debug\qiven-context-draft-loop.exe   % demo of the full loop
```

Requires a sibling checkout of `qiven-foundation` (same rule as `qiven-math`;
override with `QIVEN_FOUNDATION_ROOT`).

## Related

- Canonical project cognition: `JasonHuang3D/qiven-context`
- Conventions: `qiven-devkit` templates as applied by `qiven-foundation` / `qiven-math`
- Design lineage: draft v0 (Jason), v1–v2 (ZCode+Jason pairing), v3 phases 1–2C

## Agent notes

AGENTS.md is Devkit-managed. Draft discipline: semantics live in headers with
short normative comments; `GitStore` stays a documented sketch (the store
contract is proven by `MemoryStore`); do not introduce serialization libraries,
networking or a YAML parser — this draft proves the architecture, not the
transport. The pit regression suite is sacred: every recorded scar keeps its
named test ("scars compile").
