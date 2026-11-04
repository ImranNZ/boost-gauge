#!/usr/bin/env python3
"""Format the C++ sources with clang-format, or check that they already are.

  python tools/format.py           # rewrite the files in place
  python tools/format.py --check   # change nothing; exit 1 and list the files that differ

The style is in .clang-format (Google, 100 columns). Generated files are skipped: the font
tables (src/looks/fonts/), and the dial and spec headers (*Dial.h, *Spec.h) are written by
tools/make_font.py and tools/make_look.py, so formatting them would only be undone.

Needs:  pip install -r requirements.txt   (clang-format is pinned there)
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATED_DIRS = ("src/looks/fonts/",)
GENERATED_SUFFIXES = ("Dial.h", "Spec.h")


def sources() -> list[str]:
    """Tracked C++ files we write by hand."""
    listed = subprocess.run(["git", "ls-files", "lib", "src", "test"], cwd=ROOT, capture_output=True,
                            text=True, check=True).stdout.split()
    return [f for f in listed
            if f.endswith((".h", ".cpp"))
            and not f.startswith(GENERATED_DIRS)
            and not f.endswith(GENERATED_SUFFIXES)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="only report files that need formatting")
    args = parser.parse_args()

    exe = shutil.which("clang-format")
    if exe is None:
        sys.exit("clang-format not found: pip install -r requirements.txt")

    files = sources()
    flags = ["--dry-run", "--Werror"] if args.check else ["-i"]
    failed = False
    for start in range(0, len(files), 40):  # keep command lines short on Windows
        batch = files[start:start + 40]
        result = subprocess.run([exe, "--style=file", *flags, *batch], cwd=ROOT,
                                capture_output=True, text=True)
        if result.returncode != 0:
            failed = True
            sys.stderr.write(result.stderr)

    if args.check:
        if failed:
            print("\nformat: files above need formatting. Run: python tools/format.py", file=sys.stderr)
            return 1
        print(f"format: ok ({len(files)} files)")
        return 0
    print(f"format: formatted {len(files)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
