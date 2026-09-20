# Activation-Failure Inventory — v4 Landing Steps 2-3

**Status: candidate design input.** Executes the cognitive-boundary-model's
landing discipline (§42 steps 2-3): inventory the recurring development
failures that were **persistence-success / activation-failure**, and derive
the minimal `ActionKind` vocabulary from real scars. Every entry cites
verifiable evidence (commit, MEM, audit, pit row, or code comment).

---

## 1. Method

A case qualifies only if BOTH hold:

1. **persistence-success** — the cognition that would have prevented the
   failure was already recorded and accepted (rule, convention, contract,
   pit, or pinned guard) at the time of the failure;
2. **activation-failure** — the acting participant's process did not bring
   that cognition to bear at the action boundary.

Adjacent families are deliberately excluded (§3): authority failures
(fencing, split-brain), canonical-conflict failures (two active truths),
and ordinary bugs caught by compiled mechanisms before review.

## 2. The inventory

### A1 — Attribution-position regression (TWICE) — flagship

- **What the project knew**: the commit-attribution convention with the
  block in trailer position, accepted 2026-09-19
  (`MEM-20260919T113238Z-B2F4D8`), enforced by a same-day rewrite of
  fourteen published commits (`MEM-20260919T135930Z-F1C2A9`).
- **What happened**: the 2026-09-20 merge generator reintroduced the
  pre-subject layout in eighteen commits across five repositories; a full
  second rewrite was required (`MEM-20260920T100100Z-A7D3E9`).
- **Missing activation**: merge/commit-message authoring never recalled the
  convention — twice, AFTER acceptance AND after one complete rewrite.
- **Kind**: `Commit`. **Trigger**: LLM-authored commit or merge-message
  authoring. **Guard**: subject-position lint (fail when the first message
  line starts with `role:`); trivially mechanical, not yet compiled.

### A2 — Conventions invisible across repositories

- **What the project knew**: engineering conventions recorded in
  `qiven-context` (canonical, accepted).
- **What happened**: sessions working in other repositories never saw them;
  the devkit conventions README records this as the founding failure mode of
  the conventions rollout; the draft carried PascalCase/k-prefix deviations
  through Phase 4 close (nine-repository audit finding 6; remediation
  landed 2026-09-20, PR #20 `fd006e5`).
- **Missing activation**: repo-entry recall — a session entering draft/
  foundation had no boundary that surfaced the conventions.
- **Kind**: `CreateCppSymbol` (identifier/file creation). **Trigger**:
  creation of project identifiers/files. **Guard**: preparation packet
  (naming policy + neighboring examples). Partially landed: AGENTS.md
  pointers activate at repo ENTRY (2026-09-20, PR #40/#4/#1); the residual
  is mid-session creation after context has grown stale.

### A3 — Hardcoded dependent path survived a rename

- **What the project knew**: the views restructure (candidate `01dbacf`)
  updated the machine declaration `collaboration/context-inputs.yaml` to
  the new binding paths — the authoritative record WAS current.
- **What happened**: `tools/test_context_r1.py` hardcoded
  `views/chatgpt-jason.yaml`; the morning merges ran only the scoped
  `context-docs` gate, masking the breakage; the first full-gate run caught
  it (fix `46db19a`, PR #39).
- **Missing activation**: the rename did not trigger a reverse-dependency
  sweep; the gate selection did not trigger the full-gate rule (see A5).
- **Kind**: `ModifyReferencedContract` (proposed addition). **Trigger**:
  rename/move of any path or contract with known dependents. **Guard**:
  mechanical reference-integrity sweep over canonical surfaces AND tooling
  (grep-level; prose references included).

### A4 — "All references fixed" claim without mechanical verification

- **What the project knew**: the restructure commit itself claimed
  "All stale path references updated".
- **What happened**: six stale references remained on canonical main
  (cold-boot finding; seven replacements landed in the reconciliation
  transaction, PR #39).
- **Missing activation**: a completeness CLAIM was committed without a
  mechanical check behind it.
- **Kind**: `ModifyReferencedContract` + `MakeCanonicalClaim`. **Trigger**:
  making a completeness claim in a commit/PR. **Guard**: the A3 sweep; a
  completeness claim must cite the check's output, not assert it.

### A5 — Merge-class publication without the full gate

- **What the project knew**: the supervised-agent workflow's scoped-
  iteration rule — full default gate required on the final exact head
  before merge-class publication.
- **What happened**: the three morning merges (pre-rewrite `869c36e`,
  `775a453`, `296b5e4`) ran `context-docs` only (their own merge messages
  record it); the A3 breakage survived all three publications.
- **Missing activation**: merge-time recall of the gate-class rule.
- **Kind**: `Publish` (merge-class granularity). **Trigger**: merge-class
  publication. **Guard**: Operator-level merge proof — a merge-class PR
  carries a recorded full-gate PASS at its exact head or is refused. Same
  boundary as P-50 (publication sweep), different failure mode: P-50 is
  delta identity, A5 is gate class.

### A6 — Tool-contract guessing and CLI retry

- **What the project knew**: `.qiven/operator.json` declares every task/
  gate argv; the workflow doc's "Discover before invoking" rule exists
  precisely because guessing happened.
- **What happened (recorded instances)**: the 2026-09-19 gate odyssey —
  one-assert-at-a-time test failures each cost a full ~3-minute gate cycle
  (`tools/test_cold_boot_contract.py:148` comment); this session's
  near-miss: `gh pr create` resolved its head from the current branch and
  failed on main (adapted in one step; window log).
- **Missing activation**: invocation-time recall of the declared contract.
- **Kind**: `InvokeTool`. **Trigger**: tool invocation where a declared
  contract exists. **Guard**: contract-driven argv construction (Operator
  tasks already are this surface; the residual is direct CLI bypasses).

### A7 — Retry without new evidence (the odyssey, generalized)

- **What the project knew** (after the fact): the cold-boot contract test
  now reports every checkpoint gap in ONE failure — the fix compiled.
- **What happened before it**: repeated gate cycles each producing a single
  bit of evidence.
- **Missing activation**: first-failure → full-evidence retrieval was not a
  rule; it became one only as a code comment.
- **Kind**: `RetryFailure`. **Trigger**: first material failure → capture
  fingerprint + all related evidence; repeated identical signature → blind
  retry refused. **Guard**: batched-failure reporting is compiled in ONE
  test; the general rule is not.

### A8 — Lower-layer duplication pressure (standing class)

- **What the project knew**: engineering-philosophy §1 exists as accepted
  law naming the pattern; the Foundation inventory and conventions exist.
- **What happened**: the law's own text records that duplication is the
  recurring tendency it exists to prevent; no fresh incident SHA this
  quarter — honestly classified as a standing class grounded in accepted
  law rather than a dated incident.
- **Kind**: `IntroducePrimitive`. **Trigger**: authoring a reusable
  primitive. **Guard**: eligible-lower-layer search before implementation
  (dependency graph + symbol search), result surfaced for judgment.

### A9 — POSITIVE CONTROL: the compiled guard made forgetting impossible

- **Case**: the serialization v6 bump (this session, `216d119`→`e768ec5`
  window) initially missed two dependents — the pinned genesis golden
  digest (`pit.golden_vector_pinned`, P-44) and the corruption test's
  version byte.
- **What happened**: BOTH were caught immediately by compiled mechanisms
  (tests) before any human review; cost: minutes, not rewrites.
- **Meaning**: this is exactly A1's shape (change with known dependents,
  author forgot) — but the dependents were guarded by compiled regression,
  so the activation failure could not become a published failure. This is
  the v4 thesis in one case: **where a scar has compiled into a mechanism,
  forgetting stops being an engineering failure mode.**
- **Kind**: `ModifyReferencedContract`. **Guard**: already compiled; the
  inventory's target state for A1-A8.

## 3. Adjacent families (excluded, and why)

- **Authority/fencing** (2026-09-16 split-brain; P-01; ADR-0026): the
  cognition was not missing — the EXECUTION AUTHORITY plane was. v4 does
  not own it; v3's grant/fencing already does.
- **Canonical conflict** (K4 trial-1 `--json` vs `--verbose` wording):
  two active truths; the conflict lifecycle owns it. Activation would have
  surfaced BOTH, which argues for v4, but the failure class is conflict,
  not recall.
- **Ordinary bugs caught by build/tests** (e.g., a dangling pointer in a
  new test, caught at compile): mechanisms working as designed.

## 4. Minimal ActionKind vocabulary (derived)

Grounded kinds (evidence → mandatory requirement → testable property):

| Kind | Grounded by | Mandatory requirement at the boundary | Testable property |
| --- | --- | --- | --- |
| `Commit` | A1 (×2) | attribution format check on LLM-authored messages | no published subject starts with `role:` |
| `Publish` | A5, P-50 | full-gate PASS at exact head + reviewed-delta identity | merge-class PR carries gate proof |
| `CreateCppSymbol` | A2 | repo naming policy + neighboring examples in preparation | identifier creation has convention in packet |
| `IntroducePrimitive` | A8 | eligible-lower-layer search before implementation | search result precedes authoring |
| `InvokeTool` | A6 | declared contract drives argv; no guessing | invocation constructed from contract |
| `RetryFailure` | A7 | fingerprint + evidence batch before equivalent retry | repeated signature without new evidence refused |
| `MakeCanonicalClaim` | A4 | claim cites mechanical check or canonical record | completeness claims carry check output |
| `MakeLiveClaim` | P-38 | verify from live source; profiles carry families only | live claims resolved via live port |
| `BeginTask` / transitions | P-18, constitution §16 | retrieval invoked at structural boundaries | bundle freshness at task boundary |
| `AcceptCandidate` | reviewer self-cert ban (zcode-jason view) | acceptance role ≠ authoring instance where policy requires | H2 evidence binds reviewer identity |
| `ModifyReferencedContract` | A3, A4, A9 | dependent sweep on rename/version bump of referenced things | sweep output precedes the change |

**Proposed delta vs the seed §27 enum**: ADD `ModifyReferencedContract`
(three same-week cases ground it; the seed enum lacks it). KEEP
`EnterDomain`, `ModifyArchitecture`, `ModifyPublicAPI` as
**evidence-pending** — carried from the seed, not yet grounded in a
recorded activation failure; do not implement their triggers until a scar
demands them.

## 5. Trigger/guard backlog (ranked by recurrence × cost)

1. **Attribution subject lint** (A1) — happened twice; each recurrence cost
   a multi-repo history rewrite. **COMPILED 2026-09-20**:
   devkit `tools/check_commit_subjects.py`, selftested, wired into the
   devkit local gate; other repositories adopt via the next operator roll.
2. **Reference-integrity sweep** (A3+A4) — two cases the same day. **COMPILED
   2026-09-20**: context `tools/check_references.py`, vacuity-proven,
   wired into the context-local and context-docs gates.
3. **Merge-class full-gate proof** (A5, P-50 boundary) — Operator refuses
   merge-class publication without recorded exact-head full-gate PASS.
4. **Tool-contract invocation** (A6) — route through Operator tasks; flag
   direct-CLI bypasses of declared contracts.
5. **Failure-fingerprint + batched evidence** (A7) — generalize the
   compiled one-test fix into the control rule.
6. **Lower-layer search** (A8) — needs dependency-graph/symbol machinery;
   largest item, last.

## 6. Pit-ledger second dimension (proposal, not yet landed)

**Landed 2026-09-20**: the pit map gains a v4 activation ledger section
(`pit-regression-map.md`) carrying `activation_trigger` and `compiled_guard`
per row; P-51 (attribution lint) and P-52 (ref sweep) were born WITH their
guards; P-53 (merge-class gate proof) follows when backlog item 3 lands.
Original sequencing rule kept:
`tools/check_pit_map.py` validates map↔encoding consistency, so adding
uncompiled pits would fail the traceability gate. The map edit lands in
the same transaction as the first guards.

## 7. What this inventory does not claim

- It is not a retrieval engine, and it does not argue for semantic
  retrieval: every case here is a **deterministic-contract** case (a rule,
  a path, an argv, a version pin existed). This directly supports the
  seed's §42 ordering — deterministic triggers first; semantic machinery
  only if a scar demands it.
- It does not weaken v3: authority, persistence and transaction semantics
  are untouched; v4 adds the activation layer on top.
