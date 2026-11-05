#!/usr/bin/env bash
# Static analysis of the hand-written C++ with cppcheck (warnings, performance, portability).
# Exits non-zero on any finding, so CI fails on one. To silence a false alarm, put
#   // cppcheck-suppress <id>
# on the line above it, with a comment saying why.
#
# Windows: cppcheck ships with the WinLibs compiler (its mingw64/bin folder).
set -euo pipefail
cd "$(dirname "$0")/.."

exec cppcheck --std=c++17 --language=c++ \
  --enable=warning,performance,portability \
  --inline-suppr --suppress=missingIncludeSystem \
  --error-exitcode=1 --quiet \
  --template='{file}:{line}: ({severity}:{id}) {message}' \
  -DBOARD_WAVESHARE_LCD_128B -DTRANSPORT_DEMO \
  -I lib/core/src -I lib/fakes/src -I lib/softcanvas/src -I src \
  lib/core/src lib/fakes/src lib/softcanvas/src src/device src/sim src/main.cpp
