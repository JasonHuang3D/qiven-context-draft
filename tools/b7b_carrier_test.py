from __future__ import annotations

"""B7b carrier fixture suite (ADR-0060 D3; P0 repair batch B7b, 2026-10-02).

Regression coverage for this repository's four-element carriers:

  B7b-D1  check_pit_map dangling FAIL teaches rule + FIX route
          (behavioral: temp fixture map/corpus via module seams)
  B7b-D2  check_participant_refs --strict FAIL teaches rule + FIX route;
          report mode stays exit-zero with an explicit NOTE (behavioral)
  B7b-D3  SG-6 closure: .github/workflows/ci.yml exists and matches the
          accepted ci.full profile shape the operator.json declaration
          names (workflow_dispatch jobs input, typed plan admission and
          ci-gate FAIL carrying the four-element law - WHAT/WHY/
          EVIDENCE/NEXT (2026-10-03 extension outside the resolve job),
          lock-resolved node checkouts (2026-10-03 pin
          cancellation; no node pins live in the workflow), ci-gate conclusion
          enforcement) - source pins
  B7b-D4  the operator.json ci.full registration resolves: the declared
          workflow file is present in-tree (no dangling registration)

Disposable temp fixtures only (testing law: tests never touch the
developer's repository). Each case id rides in the failure message.
"""

import contextlib
import io
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_participant_refs as cpr  # noqa: E402
import check_pit_map as cpm  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CHECKS = 0


def check(condition: bool, label: str, detail: str = "") -> None:
    global CHECKS
    CHECKS += 1
    if not condition:
        raise AssertionError(f"[{label}] {detail}" if detail else f"[{label}] assertion failed")


def case_d1() -> None:
    with tempfile.TemporaryDirectory(prefix="draft-b7b-pit-") as tmp:
        root = Path(tmp)
        (root / "docs" / "architecture").mkdir(parents=True)
        (root / "docs" / "architecture" / "pit-regression-map.md").write_text(
            "| P-1 | sample scar | pit.missing_test | link | | | active |\n"
            "| P-2 | scheduled scar | pit.later_test | link | | | Phase 4 |\n",
            encoding="utf-8")
        (root / "tests").mkdir()
        (root / "tests" / "present.cpp").write_text(
            "// pit.present_test lives here\n", encoding="utf-8")
        # the map must also name the present test or it counts as dangling
        (root / "docs" / "architecture" / "pit-regression-map.md").write_text(
            "| P-1 | sample scar | pit.missing_test | link | | | active |\n"
            "| P-2 | scheduled scar | pit.later_test | link | | | Phase 4 |\n"
            "| P-3 | ok scar | pit.present_test | link | | | active |\n",
            encoding="utf-8")
        real_map, real_dirs = cpm.MAP, cpm.TEST_DIRS
        captured = io.StringIO()
        try:
            cpm.MAP = root / "docs" / "architecture" / "pit-regression-map.md"
            cpm.TEST_DIRS = [root / "tests"]
            with contextlib.redirect_stdout(captured):
                code = cpm.main()
        finally:
            cpm.MAP, cpm.TEST_DIRS = real_map, real_dirs
        text = captured.getvalue()
        check(code == 1, "B7b-D1", f"expected exit 1, got {code}: {text}")
        check("missing_test: named in the pit map but not present" in text,
              "B7b-D1", "dangling name listed: " + text)
        check("[FAIL] pit traceability: WHY:" in text, "B7b-D1", "WHY present")
        check("rule: draft/pit-traceability" in text, "B7b-D1", "rule present")
        check("NEXT action: FIX" in text, "B7b-D1", "FIX route present")


def case_d2() -> None:
    with tempfile.TemporaryDirectory(prefix="draft-b7b-pref-") as tmp:
        repo = Path(tmp) / "repo"
        (repo / "collaboration").mkdir(parents=True)
        (repo / "collaboration" / "rule.md").write_text(
            "The build must be driven by ZCode sessions.\n", encoding="utf-8")
        strict = io.StringIO()
        with contextlib.redirect_stdout(strict):
            code = cpr.scan(repo, strict=True)
        text = strict.getvalue()
        check(code == 1, "B7b-D2", f"strict exit {code}, expected 1: {text}")
        check("'ZCode'" in text, "B7b-D2", "finding listed: " + text)
        check("[FAIL] participant-refs: WHY:" in text, "B7b-D2", "WHY present")
        check("rule: draft/participant-refs" in text, "B7b-D2", "rule present")
        check("NEXT action: FIX" in text, "B7b-D2", "FIX route present")
        report = io.StringIO()
        with contextlib.redirect_stdout(report):
            code = cpr.scan(repo, strict=False)
        rtext = report.getvalue()
        check(code == 0, "B7b-D2", f"report exit {code}, expected 0: {rtext}")
        check("[NOTE] report mode:" in rtext, "B7b-D2",
              "report mode is explicit, never a silent pass: " + rtext)
        check("[FAIL]" not in rtext, "B7b-D2",
              "no FAIL lines on the exit-zero path: " + rtext)


def case_d3() -> None:
    workflow = ROOT / ".github" / "workflows" / "ci.yml"
    check(workflow.is_file(), "B7b-D3", "SG-6: ci.yml must exist in-tree")
    text = workflow.read_text(encoding="utf-8")
    check("workflow_dispatch:" in text, "B7b-D3", "dispatch-only trigger")
    check("inputs:" in text and "jobs:" in text, "B7b-D3", "jobs input declared")
    check("error: WHAT: unknown validation unit(s):" in text, "B7b-D3",
          "typed plan admission (labeled WHAT)")
    check("error: EVIDENCE: received jobs input string:" in text, "B7b-D3",
          "plan FAIL carriers carry labeled EVIDENCE")
    check("[FAIL] CI Gate: WHAT: a requested validation unit did not succeed"
          in text, "B7b-D3", "ci-gate FAIL carries labeled WHAT")
    check("NEXT action: FIX - correct the unit name" in text, "B7b-D3",
          "plan FAIL carries the FIX route")
    check("JasonHuang3D/qiven-workspace" in text, "B7b-D3",
          "workspace control checkout present")
    check("JasonHuang3D/qiven-context-draft" in text, "B7b-D3",
          "candidate checkout present")
    check("gate-configure" in text, "B7b-D3", "workspace-path configure")
    check("ci-gate" in text, "B7b-D3", "conclusion enforcement job")
    check("NEXT action: DIAGNOSE - open the failed unit's step logs" in text,
          "B7b-D3", "ci-gate FAIL carries the DIAGNOSE route")


def case_d4() -> None:
    import json
    config = json.loads((ROOT / ".qiven" / "operator.json").read_text(encoding="utf-8"))
    declared = config.get("ci", {}).get("full", {}).get("workflow")
    check(declared == "ci.yml", "B7b-D4", f"ci.full declares {declared!r}")
    check((ROOT / ".github" / "workflows" / str(declared)).is_file(), "B7b-D4",
          "the declared workflow resolves in-tree (SG-6 closed)")


def main() -> int:
    case_d1()
    case_d2()
    case_d3()
    case_d4()
    print(f"[ OK ] draft B7b carrier fixtures ({CHECKS} checks)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as failure:
        print(f"[FAIL] b7b-carrier-tests: {failure}", file=sys.stderr)
        print("[FAIL] b7b-carrier-tests: WHY: a pinned four-element carrier "
              "selector broke (rule: draft/b7b-carriers)", file=sys.stderr)
        print("       NEXT action: FIX - the B7b-Dn id above names the carrier; "
              "restore the four-element law, never the check", file=sys.stderr)
        raise SystemExit(1)
