#!/usr/bin/env python3
"""Fail if lib/core/src breaks its layering.

lib/core/src is grouped by domain, and a folder may include only folders ranked below
it, so there are no cycles and "later" code never leaks into "earlier" code:

    base < render, obd < sampling < gauge < looks < app

(render and obd are independent of each other.) Files must live in one of these
folders, never at the root. Run by tools/coverage.sh; also fine to run on its own.
"""
import re
import sys
from pathlib import Path

CORE = Path(__file__).resolve().parent.parent / "lib" / "core" / "src"
RANK = {"base": 0, "render": 1, "obd": 1, "sampling": 2, "gauge": 3, "looks": 4, "app": 5}
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def main() -> int:
    problems = []
    for path in sorted(CORE.rglob("*")):
        if path.suffix not in (".h", ".cpp"):
            continue
        folder = path.relative_to(CORE).parts[0] if path.parent != CORE else None
        if folder not in RANK:
            problems.append(f"{path.relative_to(CORE)}: not inside one of {', '.join(RANK)}")
            continue
        for target in INCLUDE.findall(path.read_text(encoding="utf-8")):
            dest = target.split("/")[0] if "/" in target else None
            if dest is None or dest not in RANK or dest == folder:
                continue  # a system/library header, or the same folder
            if RANK[dest] >= RANK[folder]:
                problems.append(f"{path.relative_to(CORE)}: {folder}/ must not include {target} "
                                f"({dest}/ is not below it)")

    for problem in problems:
        print(f"layering: {problem}", file=sys.stderr)
    if not problems:
        print("layering: ok (base < render, obd < sampling < gauge < looks < app)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
