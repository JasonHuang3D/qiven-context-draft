# qiven-context-draft

An **executable C++ specification** of the qiven-context cognition architecture.
This repository is the semantic bridge between human and LLM: natural language
always produces mutual interpretive drift, while pointer/value choices, types and
call graphs state ownership, lifetime and causality exactly. Here, architecture
rules compile.

**Status: draft.** This is not the production context engine (roadmap order 2,
`qiven-context.exe` / ContextKernel). It is the design substrate that the K4/K5
continuity work and the future kernel will be judged against.

## Why C++ as the bridge

Any LLM maintaining project context in prose will drift — prose boundaries erode
with every edit. Types do not drift. A value member versus a pointer member is a
machine-checkable statement about ownership and lifetime:

- `LLMCognition` is a **pure value tree** — zero pointers to runtime participants.
  That single rule makes it snapshot-able, restorable and compressible (K4/K5),
  and doubles as the drift detector: any repo surface that cannot be pointed at
  as a member of `LLMCognition` is by definition not context (views, device
  profiles, human preferences, participant bindings are runtime parameters).
- `std::shared_ptr` answers *lifetime* (who may hold cognition while it is
  restored); `std::atomic<Epoch>` answers *authority* (which instance may still
  be written). Conflating them was draft v1's mistake; the split is v2's core.
- `ReadFromCognition` returns a snapshot **by value**; `WriteToCognition` takes a
  **delta** and returns `bool`. The LLM never holds a mutable reference to
  cognition — single-writer is a type signature, not a policy.

## The persistence question (why git, and why git is optional)

The repository stores the **serialized `LLMCognition` itself**. Therefore the
entire persistence requirement is one small contract — any data storage with
incremental read/write suffices:

```cpp
struct ICognitionStore {
    ContentId   append(const Bytes& stateBytes, const ContentId& base); // incremental write
    Bytes       materialize(const ContentId& id) const;                 // read state at content id
    Bytes       readDelta(const ContentId& base, const ContentId& target) const; // incremental read
    bool        verify(const ContentId& id) const;                      // existence
};
```

`qiven-context` is **state-replication, not event-sourcing**: recovery reads the
current canonical snapshot and never replays diffs or commit graphs.

Git is today's transport implementation, nothing more:

| Store contract | git implementation |
| --- | --- |
| `append(state, base)` | commit (parent = head; `ContentId` = commit SHA) |
| `materialize(id)` | checkout / cat-file at that SHA |
| `readDelta(base, target)` | fetch/pack between two SHAs (transport optimization) |
| `verify(id)` | cat-file -e |

Layered **transport ceremonies** are not part of the store contract: push/pull =
replica sync; PR + review + merge = the H2 acceptance ceremony over the
human-AI channel; **GitHub is a second device** (a cloud device running CI) that
validates appended states, with the resulting evidence flowing back into
cognition through ordinary gated transactions.

`ContentId` is content-addressed identity (SHA-256 in production; a draft FNV-1a
here). A git commit SHA is one minter of `ContentId`; a K4 handoff-artifact
digest is another. Provenance records may cite SHAs as historical transport
facts — verifiable live (Phase B), never required for semantic reconstruction
(Phase A).

## Architecture rules (R1–R6)

1. `LLMCognition` is a pure value tree — zero pointers to runtime (drift detector).
2. Cognition swap is atomic with respect to `Work()` — fencing epoch, rebind
   before retire; lock the admission, never the work.
3. Participant change (model / tool / device / human) is a pointer rebind — O(1),
   never a cognition write.
4. Authority **rules** live in cognition; identities are session parameters
   verified against the governance principal.
5. `Work()` side effects = result + gated delta write + tool calls. Nothing else.
6. Restore is pure materialization + runtime rebind; only the delta write is gated.

## Layout

```
include/qiven/context/cognition.hpp     value tree: records, governance, state
include/qiven/context/persistence.hpp   ICognitionStore, MemoryStore, GitStore sketch,
                                        QivenContext (fencing + admission + gates)
include/qiven/context/runtime.hpp       Device, RemoteDevice, LLM, LLMClientTool, Human
include/qiven/context/context.hpp       umbrella header
src/                                    persistence (serialization, gates) + runtime
apps/cognition_loop.cpp                 runnable demo of the whole loop
tests/                                  value-tree checks, fencing gates, store contract
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
- Design lineage: draft v0 (Jason), v1–v2 (ZCode+Jason pairing), this repository
