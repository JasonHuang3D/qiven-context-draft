# Process Model — The Loop as It Actually Runs

**Status: candidate design input (ADR-0037).** This document encodes the
*operational* process — how humans, client tools, models, devices and the store
interact in real qiven-context usage — as typed vocabulary. The AI-first
development pitfalls live here more than anywhere: nearly every recorded
incident is a process failure that a data model could not see.

---

## 1. The causal loop (normative order)

```text
human intent
  -> client tool (relay; fallible; no context access)
    -> grant acquisition (authority plane; fail-closed on competition)
      -> bundle read (floors + constraints + candidates; candidate != truth)
        -> live verification of facts whose authority is live (verify_live)
          -> thinking (the only opaque step)
            -> transaction proposal (actor + operations + base + evidence)
              -> gate: verdict -> recovery table -> next action
                -> applied: successor snapshot + receipt + checkpoint update
                -> refused: per-reason mandated action; deltas park, never vanish
```

Properties that make this loop specifiable rather than merely describable:

1. The relay (client tool) has **no context access** — it moves prompts and
   results, never cognition (R5; the UI-send incident class stays a transport
   concern with typed failure, not a context corruption vector).
2. Reading never grants authority (`authorization: not_granted` on every
   bundle, without exception).
3. The only opaque step is thinking; everything around it is typed, gated and
   testable. That boundary is the whole reason an executable specification is
   possible.

## 2. Two-layer execution (brother authors, worker publishes)

The 2026-09-18 owner direction (execution-stage granularity) is process law
that v2 has no representation for. Typed form:

```cpp
struct Qualification {          // ADR-0035: untested / provisional / qualified
    Status status;  EvidenceRefs evidence;  Date qualified;
    // upgrade to qualified is owner-reserved — reviewer self-certification ban
};
struct BatchDesign {            // what brother-side produces
    std::vector<FeatureSpec> queue;   // finite, ordered, with stop conditions
    ValidationProfile profile;        // FULL default; FOCUSED needs exact scope
    std::vector<ArchitecturalBarrier> barriers;
};
struct ExecutionRecord {        // what worker-side produces
    std::vector<GateResult> gates;    // exact commands, exact heads, real codes
    std::vector<ContentId> exactHeads;// validation binds to exact SHAs
    HandoffReport report;             // truthful PASS/NONE/CLEAN or bust
};
struct ReviewRecord {           // what brother-side returns on the exact delta
    Digest reviewedDeltaDigest;       // binds to DR-004's H2Evidence
    std::vector<Finding> findings;    // blocking / non-blocking, with evidence
    bool sameSessionLimitation;       // disclosed when reviewer ≈ author instance
};
```

Rules that compile from these types:

- **Gate failures are handoff triggers.** The worker returns to the authoring
  role with the failing evidence; the author corrects with a new commit; the
  worker revalidates. A `InvariantFailed` verdict maps to "design review",
  never to a worker-side fix attempt (constitution §17).
- **Frozen layers.** Once a batch advances past a feature, that layer is frozen;
  a later defect in it stops and escalates — no autonomous history rewrites.
- **Self-certification ban.** `ReviewRecord` carries the reviewing binding; a
  review of a delta authored by the same binding cannot satisfy the policy
  table's H2 row — the gate refuses, mirroring the delegated-review escalation
  rule.

## 3. The failure-response ladder

Every recorded failure class, its mandated response, and where it is enforced.
This table *is* the recovery policy (`PolicyTable::recovery`), restated for
humans:

| # | Failure | Mandated response | Enforced by | Pit |
| --- | --- | --- | --- | --- |
| 1 | Stale base / stale thinking | park delta → re-read → re-think; never blind-retry | `Verdict::StaleBase` recovery row | P-05 family |
| 2 | Handoff evidence missing/invalid | halt, escalate; no re-attempt exists | policy table + DR-004 digest | P-05, P-07 |
| 3 | Unattended mutation attempted | defer to supervised; park | `WorkMode` gate | P-08 |
| 4 | Invariant failed (known hazard class) | design review, not patch/CI loops | `Verdict::InvariantFailed` → review | P-36 family |
| 5 | Connector/transport refusal | state the boundary; preserve intended semantics; switch authoring path (operator-first → trusted local → exact generated patch); never euphemize | `Verdict` reason `TransportRefused` + authoring-path ladder | P-11 |
| 6 | Provider unavailability / quota | bounded wait; disclose substitution; no retry loops | `ServingDisclosure` mandatory; bounded-wait primitive | P-10 |
| 7 | Competing execution flow | refuse fail-closed + quarantine; never queue | grant port | P-01 |
| 8 | Conflict discovered | preserve → open Conflict → block intersecting acceptance → reconcile when evidence permits; else explicit unresolved | DR-006 gate | P-14 |
| 9 | Missing history | record typed absence (`EvidenceGap`); never synthesize | checkpoint type | P-17 |
| 10 | Store divergence / corruption | fail closed; `OutcomeUnknown` semantics for timeouts | DR-009, DR-011 | P-23, P-31 |

The ladder's unifying rule: **the model never chooses its own recovery** — the
recovery table (cognition data) chooses, and the loop executes it. Discretion
under pressure is the failure mode this whole corpus exists to remove.

## 4. Retrieval discipline (constitution §16 compiles here)

- `RetrievalTrigger` fires at: cold boot, task transition, domain change,
  material transaction, session rollover. A correct engine that is not invoked
  is a miss — so invocation is tracked state, not hope.
- The bundle's floors (mandatory inputs, protected constraints, relevant
  negative knowledge) are non-negotiable under budget; candidates shrink.
- Candidates carry epistemic type end-to-end; retrieval results are candidate
  evidence, never truth; derived indexes rehydrate from the snapshot or fail
  visibly (index lag is a diagnostic, never a silent different-snapshot read).

## 5. Session economics

- `TurnBudget { soft, hard }` around the observed ~26-minute tool-turn
  boundary; the soft limit ends the current safe checkpoint
  (`USAGE_BUDGET_SOFT_STOP`), the hard limit halts.
- Checkpoint auto-writes (turn boundary / material transaction / async exit /
  session close) — written by the loop, not remembered by the model.
- Liveness: start acknowledged promptly, quiet periods heart-beat, milestones
  reported, terminal state explicit; only observable state, ever.
- Checkpoint content: session identity, exact task, accepted refs, parked
  candidates, `EvidenceGap` list (typed absence), serving-model disclosure,
  exact next action. Nothing that must survive lives only here.

## 6. Git and CI discipline

- Publication uses precise low-level paths (named-branch push, `gh pr create`);
  contents-API mutation is not a merge surrogate (P-12).
- Validation binds to exact heads; a changed head revalidates; clean-tree and
  diff-check gates return explicit success markers — a command that merely
  prints state is not a gate.
- CI is asynchronous: dispatch, record run identity, return; no sleep/poll
  loops; bounded-wait primitives (three observations / 60 seconds default);
  completion verified through API tooling before any dependent merge.
- Raw diff review belongs to the reviewer via exact remote review; human-run
  checks use non-paged, non-interactive forms.

## 7. Unattended mode

Read-only by default (retrieval, analysis, bounded derived artifacts).
Unattended runs acquire no mutating grant — the gate refuses before the attempt
can even form a delta (v2's mode gate kept; grant acquisition returns
`WorkMode::Unattended` grants without write rights). Local commits only inside
an explicitly authorized task scope; push/PR/merge are never unattended-
eligible.

## 8. Human and machine output views

`OutputView::Human | Machine` — two renderings of one execution result. Humans
get terminal-quality progressive output with success markers; machines get
stable structured results. Neither audience scrapes the other's representation.
The K4 producer's human-view requirement (`--verbose`) is an instance; the
Operator's `--json` is the other instance.

## 9. What this model deliberately does not encode

Judgment. Which hazard classes are "known", whether evidence is "genuinely
insufficient", whether a spec is ready for worker execution, review quality
itself — these stay human/model judgments, pinned by decision records and the
invariant inventory's judgment-only register rather than faked as types.
Pretending they compile would be the next drift.
