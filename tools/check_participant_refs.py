#!/usr/bin/env python3
"""Scoped participant-reference lint (I-PM1 / I-PM4, DR-019).

Scans a target repository's NORMATIVE surfaces for concrete participant
INSTANCE names (model products, client tools, devices). Scoping is the whole
point (participant-model.md 5.3): evidence, audits, qualification records,
state and view bindings lawfully name instances — they record what happened
or declare a participant adaptation. Normative rules must not.

Default: report mode (exit 0, findings listed). --strict exits non-zero on
any non-exempt finding, for future gate wiring.

Usage: python tools/check_participant_refs.py <repo-root> [--strict]
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# I-PM4 scope: surfaces that prescribe behavior. Deliberately excludes
# memory/, evidence/, sessions/, state/ (records of what happened) and
# views/bindings/, views/humans/, views/environments/ (participant
# declarations that exist to name participants).
NORMATIVE_DIRS = ("collaboration", "governance", "views/workflows")

# I-PM4 + I-PM1 tokens: concrete instances, not classes or roles.
INSTANCE_TOKENS = ("ZCode", "ChatGPT", "GPT-", "GLM-", "Claude", "Gemini",
                   "JasonPC")

# Per-path exemptions with reasons (the reclassification ledger: a finding
# here is a sentence judged LAWFUL under DR-018 layering — L3 ergonomics or
# explicit evidence citation inside a contract). Keep this list SHORT; every
# entry is reviewed debt.
EXEMPTIONS = {
    # operating-contract discusses serving-model/binding mechanics and the
    # JasonPC reference producer topology: these are ABOUT participants by
    # subject matter (binding law, H1 producer role), not participant-
    # dependent rules. Tracked for reclassification into participant-model
    # vocabulary.
    "collaboration/operating-contract.md",
    "collaboration/dcr-operational-contract.md",
    "collaboration/context-validation.md",
    # handoff boundary names the K4 producer topology (H1 reference example)
    "collaboration/human-handoff-boundary.md",
    # workflows are L1/L3 documents; instance names inside them are DR-018
    # layer-3 ergonomics sentences pending per-sentence classification.
    "views/workflows/supervised-agent.md",
    "views/workflows/chatgpt-jason-local-execution.md",
    "views/workflows/long-running.md",
}


def scan(repo: Path, strict: bool) -> int:
    findings: list[str] = []
    scanned = 0
    for dirname in NORMATIVE_DIRS:
        root = repo / dirname
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix not in (".md", ".yaml", ".yml"):
                continue
            scanned += 1
            rel = path.relative_to(repo).as_posix()
            exempt = any(rel.startswith(prefix) for prefix in EXEMPTIONS)
            text = path.read_text(encoding="utf-8", errors="replace")
            for lineno, line in enumerate(text.splitlines(), 1):
                for token in INSTANCE_TOKENS:
                    if token in line:
                        findings.append(
                            f"{rel}:{lineno}: '{token}'"
                            f"{' [EXEMPT]' if exempt else ''}: {line.strip()[:90]}"
                        )
    print(f"scanned {scanned} normative files under {', '.join(NORMATIVE_DIRS)}")
    hard = [f for f in findings if "[EXEMPT]" not in f]
    for f in findings:
        print(f)
    print(f"{len(findings)} findings ({len(findings) - len(hard)} exempt, {len(hard)} hard)")
    if strict and hard:
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repo", type=Path)
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    if not args.repo.is_dir():
        print(f"ERROR: {args.repo} is not a directory", file=sys.stderr)
        return 2
    return scan(args.repo, args.strict)


if __name__ == "__main__":
    raise SystemExit(main())
