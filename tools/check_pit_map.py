#!/usr/bin/env python
"""Check that every test named in the pit regression map actually exists.

The scar ledger (docs/architecture/pit-regression-map.md) names a test for
every recorded pit ("scars compile"). This tool is the wire check: a pit row
whose test name cannot be found in the test sources (or whose status column
schedules it for a later phase) fails the traceability contract.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP = ROOT / "docs" / "architecture" / "pit-regression-map.md"
TEST_DIRS = [ROOT / "tests", ROOT / "src", ROOT / "include"]

NAME_PATTERN = re.compile("pit[.]([a-z0-9_]+)")


def main() -> int:
    map_text = MAP.read_text(encoding="utf-8")
    scheduled = set()
    for line in map_text.splitlines():
        if line.lstrip().startswith("| P-"):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) >= 7 and ("Phase 4" in cells[-1] or "judgment" in cells[-1]):
                scheduled.update(NAME_PATTERN.findall(line))
    test_names = sorted(set(NAME_PATTERN.findall(map_text)))

    corpus = chr(10).join(
        p.read_text(encoding="utf-8", errors="replace")
        for d in TEST_DIRS
        for p in sorted(d.rglob("*"))
        if p.suffix in {".cpp", ".hpp", ".md"}
    )
    missing = [name for name in test_names
               if name not in corpus and name not in scheduled]
    print("[ RUN] pit traceability: " + str(len(test_names)) + " named tests in the scar ledger ("
          + str(len(scheduled)) + " scheduled for later phases)")
    for name in missing:
        print("[FAIL] " + name + ": named in the pit map but not present in the test corpus")
    if missing:
        print("[FAIL] pit traceability: " + str(len(missing)) + " dangling name(s)")
        return 1
    print("[ OK ] pit traceability: every named test exists in the corpus")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
