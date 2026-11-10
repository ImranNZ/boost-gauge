---
name: check
description: Run the full verification for the boost gauge (unit tests, 100% coverage gate, simulator render) and report results. Use before saying any change is done, or when asked to "check", "verify" or "run everything".
---

# Verify the boost gauge

0. Run `python tools/format.py` and `tools/static_analysis.sh` (CI fails on an unformatted file or a
   cppcheck finding).
1. Run `tools/coverage.sh`. It first checks the `lib/core/src` layering (`tools/check_layers.py`),
   then must end with `[doctest] Status: SUCCESS!` and
   line/branch/function coverage all `100.0%`. If PlatformIO prints SKIPPED or
   `0 test cases`, ignore that; read doctest's own summary.
   - If a command isn't found on Windows, prepend the WinLibs `mingw64/bin` and
     the Python `Scripts` directories to `PATH` for that command.
   - If the test binary exits with `3221225785` (`0xC0000139`), prepend WinLibs'
     `mingw64/bin` so its runtime DLLs win over Git Bash's; see `AGENTS.md`.
2. If any gauge drawing code changed (a look, `FrameRenderer`, `SoftCanvas`, `IDrawSurface`), run
   `pio run -e sim -t exec`, then open `out/sheet.png` and `out/no_link.png`
   and look: needle direction, readout not overlapping scale labels, zone
   colours, NO LINK banner. For motion, run `python tools/preview.py --no-browser`
   in the background, open `http://127.0.0.1:8765` in the browser pane, and use
   the page controls (`drive`, `boost <psi>`, `offline`/`online`); stop it afterwards.
3. If any look, font or canvas code changed, run `python tools/check_renders.py` (it compares every
   look against `test/golden/`; on an intended change look at the result, then `--update` and commit).
4. If firmware code (`src/**`) changed, compile the affected environment(s):
   `pio run -e device-demo` always, plus `device-can` / `device-ble` /
   `device-wifi` if their adapters changed.
5. Report: test counts, coverage numbers, which envs compiled, and anything not
   verified (hardware behaviour is never verified here; say so).

If coverage drops below 100%, add tests; do not lower the gate or exclude code
to make it pass.
