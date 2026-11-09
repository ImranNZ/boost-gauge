# AGENTS.md: working guide for AI agents

Boost Gauge is an ESP32-S3 round-display boost gauge for turbocharged cars, built around an
F56 Mini Cooper S. C++17, PlatformIO and the Arduino framework on the device; the same gauge
logic, OBD protocols and rendering also run on a PC against a simulated ECU, so most work
needs no hardware.

**Read [`CLAUDE.md`](CLAUDE.md) before changing code.** It is the canonical engineering
rulebook and has the full detail behind everything summarised here. [`README.md`](README.md)
is the user-facing documentation. This file is the fast orientation for any agent (Claude,
DSH, Cursor, ...). If the two ever disagree, `CLAUDE.md` wins.

The owner works in .NET/TypeScript. Explain C++-specific idioms (headers, RAII, `constexpr`,
object lifetimes, value vs reference semantics) briefly when they matter; do not assume C++
fluency.

## Commands

| What | Command |
| ---- | ------- |
| Full verification (trust this) | `bash tools/coverage.sh` |
| Quick unit tests | `pio test -e native` |
| Format / check format | `python tools/format.py` / `python tools/format.py --check` |
| Static analysis | `bash tools/static_analysis.sh` |
| Render stills | `pio run -e sim -t exec` (writes `out/sheet.png`, `out/no_link.png`) |
| Live browser preview | `python tools/preview.py` at <http://127.0.0.1:8765> |
| Render regression | `python tools/check_renders.py` (add `--update` after an intended visual change) |
| Layering check | `python tools/check_layers.py` (also run by `coverage.sh`) |
| Firmware build | `pio run -e device-demo` (also `device-can`, `device-ble`, `device-wifi`) |

Run the `.sh` tools through `bash` (Git Bash) on Windows. The tools are already on `PATH` on
this machine: `pio`, `gcovr`, `clang-format` from the Python `Scripts` directory; `g++` and
`cppcheck` from WinLibs.

**Trust `tools/coverage.sh`, not PlatformIO's labels.** PlatformIO may print `SKIPPED` or
`0 test cases` for doctest while the tests actually ran. Judge by the `[doctest] Status:` line
and the script's exit code.

**Windows DLL gotcha.** If `coverage.sh` builds but the test binary dies with
`Program errored with 3221225785` (`0xC0000139`, entry point not found), Git Bash's
`/mingw64/bin` is shadowing the WinLibs runtime with an older `libstdc++-6.dll`. The tests are
fine; prepend WinLibs' `mingw64\bin` inside bash and rerun:

```bash
bash -lc 'export PATH="/c/.../WinLibs.../mingw64/bin:$PATH"; tools/coverage.sh'
```

## Verify before claiming done

1. `python tools/format.py --check` (CI fails on an unformatted file; generated files are
   exempt).
2. `bash tools/static_analysis.sh` (cppcheck must be clean; suppress a false alarm with
   `// cppcheck-suppress <id>` and a reason).
3. `bash tools/coverage.sh`: `[doctest] Status: SUCCESS!` and **100% line and branch**
   coverage on `lib/core` and `lib/softcanvas`.
4. Any visual change (a look, `FrameRenderer`, `SoftCanvas`, `IDrawSurface`, a font): run
   `pio run -e sim -t exec` and **look at** `out/sheet.png` / `out/no_link.png`, then
   `python tools/check_renders.py`. After an intended change, inspect the result, run
   `--update` and commit `test/golden/`.
5. Any `src/**` change: `pio run -e device-demo`; also build the transport env whose adapter
   changed.

Hardware behaviour is never verified here. Do not claim the LCD, CAN pins or BLE dongle work
on a real board.

## Architecture in one screen

```
src/device/*  or  src/sim/*        hardware / mock adapters
        │  IByteStream or ICanBus
        ▼
IObdClient (Elm327Client | CanObdClient)     obd/
        ▼
ChannelSampler ── SamplerLoop ── ChannelMailboxes     sampling/
   (core 0 on device)              │  lock-free, one slot per channel
                                   ▼
GaugeApp (core 1) ── GaugeModel ── FrameRenderer ── ILook     app/ gauge/ looks/
                                                      │  240x240 design space
                                                      ▼
                                                ScaledSurface ── ICanvas
```

* `lib/core` is the hardware-free product. It must **never** include Arduino, ESP-IDF,
  LovyanGFX, NimBLE or WiFi headers. Hardware lives behind `IClock`, `IByteStream`,
  `ICanBus`, `IObdClient`, `IReadingSource`, `IDrawSurface`, `IFrameCache`, `ICanvas` and
  `ILook`.
* `lib/core/src` is layered, each folder depending only on those before it:
  `base` ← `obd` ← `sampling` ← `gauge` ← `looks` (with `render`) ← `app`.
  Include by path (`#include "obd/Elm327Client.h"`); never add an upward include.
* `lib/fakes` holds test doubles and is also compiled into the `device-demo` firmware, so it
  must build with the ESP32 toolchain (old libstdc++: no floating-point `std::from_chars`).
* `lib/softcanvas` is the software canvas and PNG writer (tests, sim, render checks).
* `src/main.cpp` is the device composition root; `src/sim/Rig.h` mirrors it on the PC.
  `src/device/Transport.h` is the only place that knows which link a build uses.
* Looks draw in a fixed **240×240 design space**; `FrameRenderer` puts a `ScaledSurface`
  between the look and the real canvas. Never read the real screen size inside a look.
* Angles are **0° = 3 o'clock, clockwise** everywhere.
* The sampler is the only code that touches the OBD client/bus/stream. The render loop only
  reads a mailbox slot and must never block in `GaugeApp::tick`.

## Rules that cause rework if missed

* **Keep 100% coverage** on `lib/core` and `lib/softcanvas`. New logic ships with tests; never
  lower the gate or exclude code to make it pass.
* **One job per class; depend on the narrowest interface.** A look takes `IDrawSurface`, never
  the cache or `present`. Link health is `LinkSupervisor`, ambient pressure is
  `AmbientPressure`, the displayed value is `GaugeModel`. If a class grows a second reason to
  change, split it. Before adding to `GaugeApp` or `ChannelSampler`, ask whether it is a new
  collaborator instead.
* **No heap in the sampling/render hot path** (`std::array`, fixed buffers). Heap is fine in
  `lib/fakes` and `lib/softcanvas`.
* **No Boost C++ libraries**; C++17 standard library only.
* **Read-only OBD.** Only mode 01 requests. Never add anything that writes to the car's bus
  (modes 04/08/2x/3x, UDS writes, ...) unless the owner explicitly asks.
* **Never hand-edit generated files**: `src/looks/fonts/<profile>/*.h` comes from `tools/fonts.json`
  via `python tools/make_font.py --profile <profile>`; `src/looks/{Retro,Modern}Dial.h` and `*Spec.h` come from
  `looks/<name>/spec.json` + `dial.svg.in` via `python tools/make_look.py retro|modern`.
  Geometry shared by art and code has one source (`spec.json`); do not hard-code it in C++.
* **Brand artwork is never committed.** The Cooper S badge lives only in `assets/` and
  `src/looks/local/` (both git-ignored), included via `__has_include`. Never `git add -f`,
  copy it into tracked folders or upload it; the GitHub repo is public.
* **README media must not contain the badge.** Regenerate with `python tools/make_media.py`
  (it builds the `sim-media` env with `-DBOOST_GAUGE_NO_BADGE`), never from the normal sim.
* **Visual changes need new goldens.** An unexpected `check_renders.py` failure is a
  regression: inspect `out/render-diff/` before deciding to `--update`.
* **`GaugeState::value` is in the channel's canonical unit** (psi, °C, volts); a look converts
  with `GaugeProfile::toDisplay()`. Never compare it with a profile range without converting.
* **Keep it car-independent.** Car/owner-dependent values (unit, range, zones, protocol, CAN
  speed) belong in `GaugeProfile` and `src/config.h`, not in a look or the sampler. The Mini is
  the default, not an assumption. Only use PIDs whose layout is verified; `PidSupport`
  discovers what a car implements.
* Nested `Options` structs need **constructor overloads**, not `= {}` default arguments.

## Gotchas worth knowing before you hit them

* `GaugeApp` keeps a reference to its look, which must outlive it; passing a temporary is a
  compile error on purpose. `setLook()` discards the cached dial.
* The mailbox is a single lock-free 32-bit word (15-bit seq, valid bit, value in hundredths).
  Keep it that way.
* The device fallback font (`fonts::Font0`) and the built-in `SoftCanvas` font share a 6×8
  cell so sim layouts match the screen; keep their text metrics in sync.
  The built-in `SoftCanvas` bitmap fallback only has
  `0-9 - + . space A B C D E F G H I K L M N O P R S T U V`; add a glyph to `kGlyphs` before
  using another character in a look. (Real looks use generated `Font` faces instead; list new
  characters in `tools/fonts.json` and re-run `python tools/make_font.py --profile s3`.)
* Font quality is a build-time profile, not runtime scaling: a display class is a profile in
  `fonts.json` (its resolutions), generated with `make_font.py --profile <name>` and selected in
  `src/looks/Fonts.h`. `src/main.cpp` asserts the profile covers `min(w, h) / 240`, so a board
  cannot silently render text upscaled from a smaller face. Screen size and pins live in
  `src/boards/<name>/Board.h`.
* `ManualClock::delayMs` advances time (that is what makes timeout loops terminate in tests);
  do not make a fake clock's `delayMs` a no-op.
* `fillArc` wraps its sweep at 360, so a full circle draws nothing; draw two half arcs.
* Code in `lib/fakes` must compile for `device-demo` (old libstdc++).
* On the device, a settings-menu channel change must be handed to the sampler task; never call
  `ChannelSampler::clear/add` from the render path (it owns the car link on core 0).
* Live preview: use `127.0.0.1`, not `localhost` (Windows tries IPv6 first and stalls). The
  sim's stdout is binary frame data; log to stderr only. Stop `program.exe`/`preview.py`
  before rebuilding the sim on Windows, or the link step fails.

## Where changes go

| Change | Edit |
| ------ | ---- |
| New data source / link | new class in `src/device/` + branch in `src/device/Transport.h` + env in `platformio.ini` |
| New PID / channel | constant and decoder in `obd/PidDecode.h`, a `PidChannel` factory (or an `IChannel` if it needs several PIDs), a `GaugeProfile` preset, `FakeEcu::answer`, tests |
| Tune the Classic look | `lib/core/src/looks/ClassicLook.cpp`, then run the sim |
| Tune the Retro/Modern look | `looks/<name>/spec.json` or `dial.svg.in`, then `python tools/make_look.py <name>` |
| New display class / font quality | a profile in `tools/fonts.json`, `python tools/make_font.py --profile <name>`, and a branch in `src/looks/Fonts.h` |
| Pins, screen size, panel driver | `src/boards/<name>/` (`Board.h`, `Panel.h`), chosen by `-DBOARD_<NAME>` in `platformio.ini` |
| SSID / BLE name / protocol / what is shown | `src/config.h` |
| Button, touch or tilt input | an `IInput` adapter in `src/device/`, wired in `src/main.cpp` |
| Persist the chosen setup | an `ISettingsStore` adapter in `src/device/` (`encodeSetup`/`decodeSetup`) |

## Tests and layout

* `test/test_core/` has one doctest suite per unit, named for the unit (`test_gauge_model.cpp`,
  not `test_branches.cpp`). Shared helpers live in `test_support.h`. Coverage is a floor, not
  the goal.
* Match the surrounding style: Google-ish, 2-space indent, `namespace gauge` for core and
  `namespace device` for adapters, `kConstantName`, trailing-underscore members.
* Project skills live in `.claude/skills/` for Claude Code and `.dsh/skills/` for this harness (same four:
  `check`, `add-look`, `add-pid`, `add-transport`). Keep the copies in sync, and read the relevant
  `SKILL.md` before starting that kind of task.

## Git and CI

`main` is changed through a pull request. CI runs one gated pipeline: test (coverage gate,
layering, macOS unit tests, sim build, render regression, format, cppcheck) → the four ESP32
firmware builds → publish (merge to `main` keeps an artifact; a `v*` tag creates a GitHub
Release). Run the `check` skill locally first. Change `main` only through a PR; do not push,
tag or publish a release unless the owner asks.

## Known-good baseline

On 2026-11-10 every gate was green: the doctest suites passed, `lib/core` and `lib/softcanvas`
were at 100% line and branch coverage, `clang-format` and `cppcheck` were clean, all 54 golden
renders matched, and `device-demo` compiled. Re-run the gates after any change; treat this as
a known-good snapshot, not a target.

This file summarises `CLAUDE.md`; when a rule changes, update both so they cannot drift.
