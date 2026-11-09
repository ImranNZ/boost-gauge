# Boost Gauge: guidance for Claude

ESP32-S3 round-display boost gauge for turbocharged cars, built around an F56 Mini Cooper S. C++17, PlatformIO,
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
  Before adding to `GaugeApp` or `ChannelSampler`, ask whether it is a new collaborator instead.
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
* Generated files: `src/looks/fonts/<profile>/*.h` come from `tools/fonts.json` via
  `python tools/make_font.py --profile <profile>`
  (never hand-edit; the TTFs live under `looks/*/fonts/` with their OFL licences). `src/looks/{Retro,Modern}Dial.h` and `*Spec.h` come from
  `looks/<name>/spec.json` + `dial.svg.in` via `python tools/make_look.py retro|modern`; never
  hand-edit them, regenerate. `src/looks/retro/RetroDial.h` is big (~470 KB): avoid reading it.
* `GaugeState::value` is a reading in its channel's canonical unit (psi for pressure, degrees Celsius
  for a temperature, volts); a look converts it with `GaugeProfile::toDisplay()`. Never compare it
  against a profile's range without converting.
* **Format before you commit.** `python tools/format.py` (clang-format, `.clang-format`); CI fails on
  an unformatted file. `tools/static_analysis.sh` (cppcheck) must be clean: fix a finding, or suppress it
  with `// cppcheck-suppress <id>` and a reason. Generated files are not formatted.
* **Visual changes need new reference images.** `tools/check_renders.py` compares every look with
  `test/golden/` and CI fails on a difference. After an intended change to a look, a font or the
  canvas, look at the result, then `python tools/check_renders.py --update` and commit `test/golden/`
  with the change. A failure you did not expect is a regression: read `out/render-diff/`.
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
  only reads a `ChannelMailboxes` slot. Never call the sampler or a transport from the render
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
  stalls). Its Screen menu restarts the sim (`Sim.restart` in `tools/preview.py`) and the page
  re-sends its settings; a stopped old sim must be marked retired or its reader shuts the server. The sim's stdout is binary frame data, so log to stderr only. Stop
  `program.exe`/`preview.py` before rebuilding the sim, or the link step fails on Windows.

* Every look and the settings menu draw text with a `Font` (`lib/core/src/render/Font.h`) through `Font::draw`;
  no look uses `drawText` any more. A font has several faces, generated per **font profile**
  (`tools/fonts.json` `profiles`: the resolutions one display class ships). `src/looks/Fonts.h` picks the
  profile (`FONT_PROFILE_*`); `Font::draw` picks the face closest to `IDrawSurface::scale()` and lays the text
  out in that face's own pixels, so on a board whose scale has a face it is drawn 1:1 (crisp).
  `drawMask` takes a fractional position and a `MaskView::resolution` (pixels per design pixel);
  `ScaledSurface` passes a mask of the exact scale through and resamples any other. A new display class means
  a profile in `fonts.json`, `python tools/make_font.py --profile <name>`, a branch in `src/looks/Fonts.h`,
  and the build flag that selects it; `src/main.cpp` asserts that the profile's `kMaxResolution` covers
  `min(board width, height) / 240`, so a board cannot silently render text upscaled from a smaller face.
  A font only has the characters listed for it, so a new string may need more `chars`.
* Only the built-in bitmap font below (used by tests and as a canvas fallback) is limited to its glyph list.
* The built-in `SoftCanvas` font only covers `0-9 - + . space A B C D E F G H I K L M N O P R S T U V`. Using
  any other character in a look renders nothing in the sim/tests. Add the
  glyph to `kGlyphs` in `SoftCanvas.cpp` first.
* The device font (`fonts::Font0`) and `SoftCanvas` share the 6x8 cell, so sim
  layouts match the screen. Keep text metrics in sync if either changes.
* `ManualClock::delayMs` advances time. That is what makes timeout loops
  terminate in tests; do not make a fake clock's `delayMs` a no-op.
* `fillArc` wraps its sweep at 360, so a full circle (end - start = 360) draws nothing: draw two
  half arcs, as the Motorsport rim does.
* Looks that animate (the Motorsport shift light) read `GaugeState::timeMs`; a look has no clock.
* Looks draw in a fixed 240x240 design space whatever the screen is: `FrameRenderer` wraps the canvas
  in a `ScaledSurface` (render/), so `surface.width()` is always 240 inside a look. Never read the
  real canvas size in a look or hard-code a larger screen; use design-space numbers. At 240x240 it is
  a pass-through, so the 240 goldens do not move. Real screen size lives in `board::kWidth`/`kHeight` (`src/boards/<name>/Board.h`).
* Settings menu (`app/SettingsMenu`, `SettingsController`): the choice is a `Setup` (look, channel, unit),
  always repaired by `normalised()` (a boost-dial look cannot show another channel). The menu is an
  `IOverlay` drawn by `FrameRenderer` over the look in design space; its fonts are given at construction (`looks::menuFonts()`).
  What applies a new `Setup` is an `ISetupTarget` per composition root (`sim::Rig` does). **On the
  device, a new channel means the sampler must switch, and the sampler only runs on core 0**: do not
  call `ChannelSampler::clear/add` from the render path; hand the sampler task the request.
* Angles: 0 = 3 o'clock, clockwise (screen space), everywhere.
* `LgfxCanvas.cpp` and the CAN pins have not been run on real hardware yet (see
  README "Not yet verified"). Do not state they are confirmed working.

## Not Mini-specific

The code uses standard OBD-II only; the Mini is the default setup and the first test car, not an
assumption. Keep it that way: car- and owner-dependent values (unit, range, zones, bus protocol, CAN
speed) belong in `GaugeProfile` and `src/config.h`, not in a look or the sampler. The Mini
branding (badge, "Modern Mini" look) stays optional. The README's "Other cars" table says what is
and is not portable (`PidSupport` discovers what a car implements; never use a PID whose layout you
have not verified); keep it accurate when you change protocol or PID handling.

## Pull requests and CI

Change `main` through a pull request. CI (`.github/workflows/ci.yml`) runs gated stages: test (coverage
gate, macOS unit tests, sim build), then the four firmware builds, then, on a push only, publish
(an artifact for `main`, a GitHub Release for a `v*` tag, made by `tools/publish_release.sh` as a retried
draft; `v0.*` is a pre-release). Run the `check` skill locally first; the
firmware envs you did not touch are the slowest thing CI checks, so build the ones you did.

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
| New PID / channel | constant and decoder in `obd/PidDecode.h`, a `PidChannel` factory (or an `IChannel` if it needs more than one PID), a `GaugeProfile` preset, `FakeEcu::answer`, tests |
| Tune the Classic look | `lib/core/src/looks/ClassicLook.cpp`, then run the sim |
| Tune the Retro look | `looks/retro/spec.json` or `dial.svg.in`, then `python tools/make_look.py retro` |
| A new screen size or font quality | a profile in `tools/fonts.json` + `python tools/make_font.py --profile <name>` + a branch in `src/looks/Fonts.h`; the screen size and pins stay in `src/boards/<name>/Board.h` |
| Pins, screen size, panel driver | `src/boards/<name>/` (`Board.h`, `Panel.h`), chosen by `-DBOARD_<NAME>` in `platformio.ini` |
| SSID / BLE name / protocol / what is shown | `src/config.h` |
| A button, touch or tilt to drive the settings menu | an `IInput` adapter in `src/device/` (turns the hardware into `Next` / `Select`), wired in `src/main.cpp` |
| Where the chosen setup is kept across power cycles | an `ISettingsStore` adapter in `src/device/` (`encodeSetup` / `decodeSetup` give it one word) |

Project skills live in `.claude/skills/` (`check`, `add-transport`, `add-pid`, `add-look`); these are
mirrored into `.dsh/skills/` for DeepSeek Harness, so keep the two copies in sync.
