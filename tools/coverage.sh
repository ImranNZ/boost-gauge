#!/usr/bin/env bash
# Run the unit tests instrumented for gcov and enforce the coverage gate.
#
#   tools/coverage.sh          # text summary, fails if the gate is missed
#   tools/coverage.sh html     # also writes out/coverage/index.html
#
# Scope: lib/core and lib/softcanvas (all hardware-independent production code).
# lib/fakes is test support, and src/device/** is hardware glue that cannot run
# on a PC, so neither is measured.
set -euo pipefail
cd "$(dirname "$0")/.."

# The gate is defined for GCC. Apple's "g++" is really clang, which counts default
# member initialisers and branches differently. On macOS build with Homebrew's GCC
# (brew install gcc) when it is there; without it we still run, but only report.
gcov_args=()
gate=(--fail-under-line 100 --fail-under-branch 100)
if [[ "$(uname -s)" == "Darwin" ]]; then
  gcc_version=""
  for v in 15 14 13 12; do
    if command -v "g++-$v" >/dev/null; then gcc_version=$v; break; fi
  done
  if [[ -n "$gcc_version" ]]; then
    export GAUGE_GCC_VERSION=$gcc_version   # read by tools/link_coverage.py
    gcov_args=(--gcov-executable "gcov-$gcc_version")
  else
    gcov_args=(--gcov-executable "xcrun llvm-cov gcov")
    gate=()
    echo "NOTE: Apple clang counts coverage differently, so the 100% gate is not enforced." >&2
    echo "      Run 'brew install gcc' and re-run for the real gate." >&2
  fi
fi

rm -rf .pio/build/coverage
log=$(mktemp)
pio test -e coverage -vv >"$log" 2>&1 || true

# PlatformIO's doctest runner mislabels results, so judge by doctest's own summary.
grep -E "^\[doctest\] (test cases|assertions|Status)" "$log" || { cat "$log"; exit 1; }
grep -q "Status: SUCCESS" "$log" || { cat "$log"; echo "TESTS FAILED"; exit 1; }

common=(-r . --object-directory .pio/build/coverage
        --filter 'lib/core' --filter 'lib/softcanvas'
        --exclude-throw-branches --exclude-unreachable-branches
        # A lone "}" is where the compiler hangs destructor/unwind cleanup; no logic.
        --exclude-lines-by-pattern '^\s*}\s*$')

# gcovr on PATH (venv or pipx) wins; else python3 (macOS/Linux) or python (Windows,
# where python3 can be a Store stub).
if command -v gcovr >/dev/null; then gcovr_cmd=(gcovr)
elif python3 -c "" >/dev/null 2>&1; then gcovr_cmd=(python3 -m gcovr)
else gcovr_cmd=(python -m gcovr); fi

# ${arr[@]+...} keeps empty arrays safe under `set -u` on bash 3.2 (macOS).
run_gcovr() {
  "${gcovr_cmd[@]}" ${gcov_args[@]+"${gcov_args[@]}"} "${common[@]}" "$@"
}

run_gcovr --txt --print-summary ${gate[@]+"${gate[@]}"}

if [[ "${1:-}" == "html" ]]; then
  mkdir -p out/coverage
  run_gcovr --html-details out/coverage/index.html
  echo "report: out/coverage/index.html"
fi
