# Boost Gauge: guidance for Claude

ESP32-S3 round-display boost gauge for an F56 Mini Cooper S. C++17, PlatformIO,
Arduino framework on device. The owner's background is .NET/TypeScript, so
explain C++-specific idioms (headers, RAII, `constexpr`, lifetimes) briefly
when they matter.

## Commands

```bash
pio test -e native            # unit tests (PlatformIO may print SKIPPED / 0 cases; ignore)
tools/coverage.sh             # tests + coverage gate: 100% line AND branch on lib/core + lib/softcanvas
pio run -e sim -t exec        # render stills to out/sheet.png and out/no_link.png
python tools/preview.py       # live animation at http://127.0.0.1:8765 (mock ECU + controls)
pio run -e device-demo        # compile firmware (simulated ECU); also device-can / -ble / -wifi
```

Windows: `g++` (WinLibs) and `pio`/`gcovr` (Python `Scripts` dir) may not be on
`PATH` in a fresh shell. If a command is "not found", add those directories to
`PATH` for the command instead of reinstalling.

## Rules

* **Verify before claiming done.** Run `tools/coverage.sh` and read the
  `[doctest] Status:` line; for visual changes run the sim and look at
  `out/sheet.png`. Do not trust PlatformIO's own SKIPPED/ERRORED labels for
  doctest; trust doctest's summary and the exit code of `tools/coverage.sh`.
* **Keep 100% coverage** on `lib/core` and `lib/softcanvas`. New logic comes
  with tests. `src/device/**` and `src/main.cpp` are hardware glue and are
  intentionally not measured.
* **SOLID / modular.** `lib/core` must never include Arduino, ESP-IDF,
  LovyanGFX, NimBLE or WiFi headers. Hardware sits behind `IClock`,
  `IByteStream`, `ICanBus`, `ICanvas`; concrete classes are wired only in
  `src/main.cpp` (and `src/device/Transport.h` for the car link). New hardware =
  new adapter in `src/device/`, not edits to core.
* **One job per class; depend on the narrowest interface.** A look takes an
  `IDrawSurface` (never the cache or `present`); caching and presenting belong to
  `FrameRenderer`. Link health is `LinkSupervisor`, ambient pressure is `AmbientPressure`,
  the value shown is `GaugeModel`. If a class grows a second reason to change, split it.
  Before adding to `GaugeApp` or `BoostSampler`, ask whether it is a new collaborator instead.
* **No duplicated logic across backends.** Shared maths lives in `lib/core` (e.g.
  `blend`, `arcCoverage`); the PC canvas and the LCD canvas both call it.
* **Geometry that the artwork and the code share has one source** (`looks/*/spec.json`),
  generated into both. Do not hard-code such numbers in C++.
* **Tests are named for the unit they test** (`test_gauge_model.cpp`, not
  `test_branches.cpp`); shared helpers go in `test_support.h`. Coverage is a floor, not the goal.
* **No heap in the sampling/render hot path** (`std::array`, fixed buffers).
  Heap is fine in `lib/fakes` and `lib/softcanvas` (test/PC only).
* **No Boost C++ libraries.** Too heavy for the MCU. Use the C++17 standard library.
* **Nested `Options` structs need constructor overloads**, not `= {}` default
  arguments (a C++ restriction that already bit us once). Copy the existing
  delegating-constructor pattern.
* **Read-only OBD.** Only mode 01 requests. Never add anything that writes to the
  car's bus (modes 04/08/2x/3x, UDS writes, etc.) without the owner explicitly asking.
* Match the surrounding code style: Google-ish, 2-space indent, `namespace gauge`
  for core and `namespace device` for adapters, `kConstantName`, trailing-underscore members.

## Gotchas

* Looks: the app draws through an `ILook` (`ClassicLook`, `RetroLook`, `ModernLook`, `MinimalLook`, `MotorsportLook`) with two methods,
  `drawDial` (static, cached by `FrameRenderer`) and `drawDynamic` (every frame). `GaugeApp`
  keeps a reference to the look, so it must outlive the app; passing a temporary is a
  compile error on purpose. `setLook()` discards the cached dial. See the `add-look` skill.
* Generated files: `src/looks/fonts/*.h` come from `tools/fonts.json` via `python tools/make_font.py`
  (never hand-edit; the TTFs live under `looks/*/fonts/` with their OFL licences). `src/looks/{Retro,Modern}Dial.h` and `*Spec.h` come from
  `looks/<name>/spec.json` + `dial.svg.in` via `python tools/make_look.py retro|modern`; never
  hand-edit them, regenerate. `src/looks/retro/RetroDial.h` is big (~470 KB): avoid reading it.
* **README media must not contain the badge.** `docs/media/` is regenerated with
  `python tools/make_media.py`, which builds the `sim-media` env (`-DBOOST_GAUGE_NO_BADGE`). Never
  screenshot the normal sim or the live preview for anything committed: they show the local badge.
* **Brand artwork is never committed.** The Cooper S badge lives in `assets/` and
  `src/looks/local/` (both in `.gitignore`) and is included via `__has_include`: `Badge.h` (dark
  lettering, for the cream Retro dial) and `BadgeLight.h` (namespace `badge_light`, for the dark looks). Do not
  `git add -f` those, do not copy brand images into tracked folders, and do not upload them
  anywhere public. The GitHub repo is public.
* `ICanvas::drawImage` takes an `ImageView` (RGB565 plus optional 8-bit alpha). Opaque images are a
  bulk copy on the device; images with alpha are blended per pixel, so keep them small.

* Threading model: the sampler (`SamplerLoop`, in `SamplerTask` on core 0) is the only
  thing that touches the OBD client/bus/stream. The render loop (`GaugeApp`, core 1)
  only reads the `ReadingMailbox`. Never call the sampler or a transport from the render
  path, and never block in `GaugeApp::tick`. The mailbox is a single lock-free
  32-bit word (15-bit seq, valid bit, psi in hundredths); keep it that way.
* `GaugeApp` takes an `IReadingSource`. Tests and the sim use `PollingSource` (a
  synchronous source) or `ThrottledSource` (a link-rate limiter); the firmware uses the mailbox.

* Canvas contract: shapes are anti-aliased (no `fillTriangle`; a needle is `drawWedge` with
  float coordinates). `IDrawSurface` is the drawing side; `IFrameCache` is
  `saveBackground()` / `restoreBackground()` (false when nothing is saved: the caller redraws,
  see `FrameRenderer::render`); `ICanvas` adds `present()`. Looks must only use `IDrawSurface`.
* `SoftCanvas(w, h, supersample)`: default 1 is hard-edged and pixel-exact (use it
  in unit tests); the sim `Rig` uses 4. `image()` returns the downsampled frame.
* Code in `lib/fakes` is also compiled into the device-demo firmware, so it must
  build with the ESP32 toolchain (old libstdc++: no floating-point `std::from_chars`).
  Run `pio run -e device-demo` after touching it.

* Live preview: use `127.0.0.1`, not `localhost` (Windows tries IPv6 first and
  stalls). The sim's stdout is binary frame data, so log to stderr only. Stop
  `program.exe`/`preview.py` before rebuilding the sim, or the link step fails on Windows.

* Looks that need nice type take a `Font` (`lib/core/src/render/Font.h`) and draw with `Font::draw`; only
  the built-in bitmap font below is limited to its glyph list.
* The built-in `SoftCanvas` font only covers `0-9 - + . space A B C E F I K L M N O P S T U V`. Using
  any other character in a look renders nothing in the sim/tests. Add the
  glyph to `kGlyphs` in `SoftCanvas.cpp` first.
* The device font (`fonts::Font0`) and `SoftCanvas` share the 6x8 cell, so sim
  layouts match the screen. Keep text metrics in sync if either changes.
* `ManualClock::delayMs` advances time. That is what makes timeout loops
  terminate in tests; do not make a fake clock's `delayMs` a no-op.
* `fillArc` wraps its sweep at 360, so a full circle (end - start = 360) draws nothing: draw two
  half arcs, as the Motorsport rim does.
* Looks that animate (the Motorsport shift light) read `GaugeState::timeMs`; a look has no clock.
* Angles: 0 = 3 o'clock, clockwise (screen space), everywhere.
* `LgfxCanvas.cpp` and the CAN pins have not been run on real hardware yet (see
  README "Not yet verified"). Do not state they are confirmed working.

## Layout of lib/core/src

Grouped by domain, each folder depending only on the ones before it: `base` (IClock) <- `obd`
<- `sampling` <- `gauge` <- `looks` (with `render`, the drawing toolkit) <- `app` (GaugeApp).
Include by path (`#include "obd/Elm327Client.h"`). A new file goes in the folder for what it is
about; if it needs something from a folder later in that list, it belongs later too. Do not add
upward includes (e.g. `gauge/` must not include `looks/`), and do not put files at the root.

## Where things go

| Change | Edit |
| ------ | ---- |
| New data source / link | new class in `src/device/` + a branch in `src/device/Transport.h` + env in `platformio.ini` |
| New PID | constant in `obd/IObdClient.h`, `FakeEcu::answer`, a collaborator beside `AmbientPressure` / `BoostSampler`, tests |
| Tune the Classic look | `lib/core/src/looks/ClassicLook.cpp`, then run the sim |
| Tune the Retro look | `looks/retro/spec.json` or `dial.svg.in`, then `python tools/make_look.py retro` |
| Pins / SSID / BLE name | `src/config.h` |

Project skills live in `.claude/skills/` (`check`, `add-transport`, `add-pid`, `add-look`).
