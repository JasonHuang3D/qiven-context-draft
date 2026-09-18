# AGENTS.md — qiven-context-draft

Executable C++ specification of the qiven-context cognition architecture.
The canonical project cognition lives in `JasonHuang3D/qiven-context`; this
repository expresses its architecture as compilable types and must stay in sync
with the accepted contracts there (ADR-0030/0033/0035/0036,
`collaboration/human-handoff-boundary.md`).

Conventions follow `qiven-devkit` templates as applied by `qiven-foundation`
and `qiven-math`: namespace `qiven::context`, C++20, `/W4 /permissive- /utf-8`
on MSVC, flat assert-based test executables, VS2022 CMake presets.

Build and test:

```cmd
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug
ctest --preset vs2022-x64-debug
```

Draft discipline: semantics live in headers with short normative comments;
`GitStore` stays a documented sketch (the store contract is proven by
`MemoryStore`); do not introduce serialization libraries, networking or a YAML
parser — this draft proves the architecture, not the transport.
