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

# python3 on macOS/Linux; plain python on Windows (python3 there can be a Store stub).
if python3 -c "" >/dev/null 2>&1; then py=python3; else py=python; fi

# Apple's compiler is clang, whose gcov data needs llvm-cov rather than gcc's gcov.
gcovr_extra=()
if [[ "$(uname -s)" == "Darwin" ]]; then
  gcovr_extra=(--gcov-executable "xcrun llvm-cov gcov")
fi

"$py" -m gcovr ${gcovr_extra[@]+"${gcovr_extra[@]}"} "${common[@]}" --txt --print-summary \
  --fail-under-line 100 --fail-under-branch 100

if [[ "${1:-}" == "html" ]]; then
  mkdir -p out/coverage
  "$py" -m gcovr ${gcovr_extra[@]+"${gcovr_extra[@]}"} "${common[@]}" --html-details out/coverage/index.html
  echo "report: out/coverage/index.html"
fi
