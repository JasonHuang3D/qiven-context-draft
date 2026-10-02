from __future__ import annotations

"""B5 carrier fixture suite (ADR-0060 D3; P0 repair batch B5, 2026-10-02).

Proves THIS repository's build/test/configure task rows (register
draft-task-*), the exact-head/diff-check/clean-tree/merge-proof builtin
rows and the draft-op-exec-start twin ride the four-element carriers of
the CANONICAL devkit operator (the WR-6 same-operator-build basis: the
suite drives the real sibling operator - the same file tools/qiven.py
imports - against a disposable fixture repository whose operator.json
declares THIS repository's real task names):

  B5-0     twin basis: tools/qiven.py is the WR-6 launcher importing the
           canonical operator; the repo operator.json declares every
           in-scope row
  B5-1.<row>  configure/build-debug/build-release/test-debug/test-release
           FAIL: WHAT line + rule + retained-evidence locator + NEXT
           DIAGNOSE with the evidence-read handle and re-run command
  B5-2     big-output FAIL stays bounded (head/tail excerpt + omitted
           count) with the retained evidence locator
  B5-3     exact-head mismatch: rule + compared values + FIX route
  B5-4     merge-proof missing receipt: the B7a four-element carrier
  B5-5     diff-check whitespace: the B7a carrier (bounded findings + FIX)
  B5-6     clean-tree dirty: the B7a carrier (bounded listing + FIX)
  B5-7     PASS carrier byte-stable (no teaching lines)
  B5-8     exec-start 124 twin: WHY (supervision budget, run continues
           under lease) + the status re-attach route; the run is stopped
           before exit (testing law: no leaked fixture processes)

Each case id rides in the assertion message. Disposable temp fixtures
only (testing law: tests never touch developer repositories).
"""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEVKIT_CHECKOUT = Path(os.environ.get("QIVEN_DEVKIT_CHECKOUT", ROOT.parent / "qiven-devkit"))
OPERATOR = DEVKIT_CHECKOUT / "tools" / "qiven_operator.py"
CHECKS = 0

# the in-scope B5 rows of this repository (register draft-task-*): the
# argv rows are driven with fast failing commands at their REAL names,
# the builtin rows with their real specs
ARGV_TASK_ROWS = ("configure", "build-debug", "build-release",
                  "test-debug", "test-release")


def check(condition: bool, label: str, detail: str = "") -> None:
    global CHECKS
    CHECKS += 1
    if not condition:
        raise AssertionError(f"[{label}] {detail}" if detail else f"[{label}] assertion failed")


def run(argv: list[str], *, cwd: Path, expect: int | None = 0) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        argv, cwd=cwd, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=False,
    )
    if expect is not None and completed.returncode != expect:
        raise AssertionError(
            f"unexpected exit {completed.returncode}, expected {expect}: {argv}\n{completed.stdout}"
        )
    return completed


def git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return run(["git", "-C", str(repo), *args], cwd=repo)


def failing_argv(name: str) -> list[str]:
    return [sys.executable, "-c", f"print('{name}-fixture-head'); raise SystemExit(3)"]


def fixture_tasks() -> dict:
    tasks = {name: {"argv": failing_argv(name)} for name in ARGV_TASK_ROWS}
    tasks["exact-head"] = {"builtin": "exact_head", "expected": "0" * 40}
    tasks["diff-check"] = {"builtin": "git_diff_check", "base": "origin/main"}
    tasks["clean-tree"] = {"builtin": "git_clean_tree"}
    tasks["merge-proof"] = {"builtin": "gate_proof", "gate": "local"}
    tasks["b5-big-fail"] = {"argv": [sys.executable, "-c",
                                     "print('b5-big-head ' + 'x' * 6000); "
                                     "print('b5-big-tail'); raise SystemExit(4)"]}
    tasks["b5-pass-ok"] = {"argv": [sys.executable, "-c", "print('b5-ok')"]}
    return tasks


def make_fixture(temp: Path, name: str, tasks: dict) -> Path:
    repo = temp / name
    repo.mkdir(parents=True)
    (repo / ".gitignore").write_text(".generated-temp/\n", encoding="utf-8")
    (repo / ".qiven").mkdir()
    config = {
        "schema_version": 1,
        "repository_name": name,
        "default_gate": "local",
        "tasks": tasks,
        "gates": {"local": []},
        "ci": {},
    }
    (repo / ".qiven" / "operator.json").write_text(
        json.dumps(config, indent=2) + "\n", encoding="utf-8")
    git(repo, "init", "-b", "main")
    git(repo, "config", "core.autocrlf", "false")
    git(repo, "config", "user.name", "B5Carrier")
    git(repo, "config", "user.email", "b5@example.invalid")
    (repo / "seed.txt").write_text("seed\n", encoding="utf-8")
    git(repo, "add", "--all")
    git(repo, "commit", "-m", "baseline")
    return repo


def op_run(repo: Path, *args: str, expect: int = 0) -> subprocess.CompletedProcess[str]:
    env = {**os.environ, "QIVEN_TARGET_ROOT": str(repo)}
    completed = subprocess.run(
        [sys.executable, str(OPERATOR), "--no-color", *args],
        cwd=repo, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=False,
    )
    if completed.returncode != expect:
        raise AssertionError(
            f"unexpected exit {completed.returncode}, expected {expect}: {args}\n{completed.stdout}"
        )
    return completed


def twin_basis_checks() -> None:
    check(OPERATOR.is_file(), "B5-0.canonical-operator",
          f"the canonical operator is missing at {OPERATOR}; set QIVEN_DEVKIT_CHECKOUT")
    launcher = (ROOT / "tools" / "qiven.py").read_text(encoding="utf-8")
    check("WR-6 launcher" in launcher, "B5-0.launcher-wr6",
          "tools/qiven.py must stay the WR-6 launcher (the twin basis)")
    check("qiven_operator" in launcher, "B5-0.launcher-imports-operator",
          "the launcher imports the canonical operator by module name")
    real = json.loads((ROOT / ".qiven" / "operator.json").read_text(encoding="utf-8"))
    tasks = real.get("tasks", {})
    for name in ARGV_TASK_ROWS:
        spec = tasks.get(name)
        check(isinstance(spec, dict) and isinstance(spec.get("argv"), list),
              f"B5-0.declares-{name}", f"operator.json must declare task {name}")
    check(tasks.get("exact-head", {}).get("builtin") == "exact_head",
          "B5-0.exact-head-builtin")
    check(tasks.get("diff-check", {}).get("builtin") == "git_diff_check",
          "B5-0.diff-check-builtin")
    check(tasks.get("clean-tree", {}).get("builtin") == "git_clean_tree",
          "B5-0.clean-tree-builtin")
    check(tasks.get("merge-proof", {}).get("builtin") == "gate_proof",
          "B5-0.merge-proof-builtin")


def task_row_cases(temp: Path) -> Path:
    repo = make_fixture(temp, "b5-task-rows", fixture_tasks())

    # -------- B5-1: every argv task row fails four-element ------------
    for name in ARGV_TASK_ROWS:
        done = op_run(repo, "run", name, expect=1)
        check(f"[FAIL] {name}: exit 3" in done.stdout, f"B5-1.{name}.what", done.stdout)
        check("rule: operator/task-failed" in done.stdout, f"B5-1.{name}.rule", done.stdout)
        check("full output retained at:" in done.stdout, f"B5-1.{name}.evidence", done.stdout)
        check("NEXT: DIAGNOSE -" in done.stdout and "qiven evidence-read" in done.stdout,
              f"B5-1.{name}.next", done.stdout)
        check(f"re-run `qiven run {name}`" in done.stdout, f"B5-1.{name}.rerun", done.stdout)
    # the composed run-level selector line rides the same carrier
    done = op_run(repo, "run", "build-debug", expect=1)
    summary = next((ln for ln in done.stdout.splitlines() if "run: FAIL" in ln), "")
    check("NEXT action: DIAGNOSE" in summary and " - evidence: " in summary,
          "B5-1.run-selector", summary)

    # -------- B5-2: big-output FAIL stays bounded ---------------------
    done = op_run(repo, "run", "b5-big-fail", expect=1)
    check("bytes omitted" in done.stdout, "B5-2.omitted-marker", done.stdout)
    check("x" * 3000 not in done.stdout, "B5-2.middle-omitted",
          "the 6000-byte run must not reach the console unbounded")
    check("b5-big-head" in done.stdout and "b5-big-tail" in done.stdout,
          "B5-2.head-tail-kept", done.stdout)
    check("full output retained at:" in done.stdout, "B5-2.evidence-locator", done.stdout)

    # -------- B5-3: exact-head mismatch four-element ------------------
    done = op_run(repo, "run", "exact-head", expect=1)
    check("rule: operator/exact-head-mismatch" in done.stdout, "B5-3.rule", done.stdout)
    check("evidence: expected" in done.stdout and "git rev-parse HEAD" in done.stdout,
          "B5-3.evidence-values", done.stdout)
    check("NEXT: FIX - verify the intended full sha" in done.stdout, "B5-3.fix-route",
          done.stdout)

    # -------- B5-4: merge-proof missing receipt (B7a carrier twin) ----
    done = op_run(repo, "run", "merge-proof", expect=1)
    check("rule: operator/gate-proof-missing-receipt" in done.stdout, "B5-4.rule", done.stdout)
    check("NEXT: FIX - run `qiven gate --name local`" in done.stdout, "B5-4.fix-route",
          done.stdout)

    # -------- B5-5: diff-check whitespace (B7a carrier twin) ----------
    head = git(repo, "rev-parse", "HEAD").stdout.strip()
    git(repo, "update-ref", "refs/remotes/origin/main", head)
    (repo / "ws.txt").write_text("trailing whitespace   \n", encoding="utf-8")
    git(repo, "add", "--all")
    git(repo, "commit", "-m", "introduce whitespace error")
    done = op_run(repo, "run", "diff-check", expect=1)
    check("rule: operator/diff-check" in done.stdout, "B5-5.rule", done.stdout)
    check("ws.txt" in done.stdout, "B5-5.evidence-listing", done.stdout)
    check("NEXT: FIX - fix the whitespace/conflict markers" in done.stdout,
          "B5-5.fix-route", done.stdout)

    # -------- B5-6: clean-tree dirty (B7a carrier twin) ---------------
    (repo / "untracked.txt").write_text("dirty\n", encoding="utf-8")
    done = op_run(repo, "run", "clean-tree", expect=1)
    check("rule: operator/clean-tree" in done.stdout, "B5-6.rule", done.stdout)
    check("?? untracked.txt" in done.stdout, "B5-6.evidence-listing", done.stdout)
    check("NEXT: FIX - commit or stash" in done.stdout, "B5-6.fix-route", done.stdout)
    return repo


def pass_stability_case(repo: Path) -> None:
    # -------- B5-7: PASS carrier byte-stable ---------------------------
    done = op_run(repo, "run", "b5-pass-ok")
    check(any(ln.strip() == "[ OK ] run: PASS" for ln in done.stdout.splitlines()),
          "B5-7.pass-line-byte-stable", done.stdout)
    check(not any("NEXT" in ln for ln in done.stdout.splitlines()),
          "B5-7.pass-no-teaching", "PASS carrier must not grow teaching lines")


def exec_twin_case(temp: Path) -> None:
    # -------- B5-8: the exec-start 124 twin (draft-op-exec-start) -----
    repo = make_fixture(temp, "b5-exec-twin",
                        {"noop": {"argv": [sys.executable, "-c", "pass"]}})
    done = op_run(repo, "exec", "start", "--timeout", "1", "--max-lifetime", "30",
                  "--", sys.executable, "-c", "import time; time.sleep(8)", expect=124)
    wait_line = next((ln for ln in done.stdout.splitlines()
                      if "still running after" in ln), "")
    check(bool(wait_line), "B5-8.wait-line", done.stdout)
    check("why: exit 124 is the operator supervision budget" in done.stdout,
          "B5-8.why", done.stdout)
    twin_id = wait_line.split()[2].rstrip(":") if wait_line else ""
    check(f"NEXT action: re-attach with `qiven exec status {twin_id}`" in done.stdout,
          "B5-8.next-status-handle", done.stdout)
    check("key on the payload status field" in done.stdout, "B5-8.status-field-law",
          done.stdout)
    stopped = op_run(repo, "--json", "exec", "stop", twin_id)
    check(json.loads(stopped.stdout).get("status") == "stopped", "B5-8.cleaned-up",
          stopped.stdout)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="qiven-b5-carrier-") as temp_name:
        temp = Path(temp_name)
        twin_basis_checks()
        repo = task_row_cases(temp)
        pass_stability_case(repo)
        exec_twin_case(temp)
    print(f"[ OK ] b5-carrier: {CHECKS} checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
