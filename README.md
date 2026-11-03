# Boost Gauge

A digital boost gauge for an F56 Mini Cooper S (B48), running on a Waveshare
[ESP32-S3-LCD-1.28-B](https://www.waveshare.com/product/esp32-s3-lcd-1.28-b.htm)
(240x240 round GC9A01 display). It reads manifold absolute pressure (OBD-II
PID `0B`) and barometric pressure (PID `33`) and shows **boost in psi**
(negative = vacuum) with a needle, peak hold and a NO LINK state.

You do not need the hardware to work on this. The gauge logic, the OBD
protocols and the rendering all run on your PC against a simulated ECU.

## Get started (5 minutes, no hardware)

### 1. Install the tools

You need a C++ compiler (for PC builds), PlatformIO, and gcovr (coverage).

**Windows**

```bash
winget install BrechtSanders.WinLibs.POSIX.UCRT
pip install platformio gcovr
```

**macOS / Linux**: install `gcc`/`g++` (Xcode CLT or `build-essential`), then
`pip install platformio gcovr`.

Open a new terminal afterwards so the tools are on your `PATH`
(`g++ --version` and `pio --version` should both work).

### 2. Run the tests

```bash
pio test -e native
```

The first run downloads the doctest library. PlatformIO may label the result
`SKIPPED` with `0 test cases`; that is a reporting quirk of its doctest
runner. Use `tools/coverage.sh` (below) for a trustworthy pass/fail.

### 3. Run the tests with the coverage gate

```bash
tools/coverage.sh          # add "html" for out/coverage/index.html
```

Fails unless `lib/core` and `lib/softcanvas` are at 100% line and branch
coverage.

### 4. See the gauge, live

```bash
python tools/preview.py
```

Builds the simulator, opens <http://127.0.0.1:8765> and shows the gauge animating
in real time (~30 fps) against the mock ECU. It runs the real gauge code
(the looks, `GaugeApp`, the OBD client) with a fake ECU on the other end. The
page has controls to drive the mock:

* **Drive cycle**: the scripted 20 s loop (idle vacuum, full-boost pull, lift-off, cruise)
* **Hold boost** slider: pin the ECU at any pressure from -15 to +25 psi
* **Drop link / Restore link**: see the NO LINK state and the sampler reconnecting
* **Data rate**: how often fresh readings arrive (30 / 25 / 10 / 5 / 2 Hz), to see how
  the needle behaves with a fast CAN link versus a slow dongle

The **View** menu switches between fit-to-page, 1:1 (240 px) and **real size**
(the actual 32.5 mm display, which is how pixelated it will really look); "Show
raw pixels" turns off the browser's smooth scaling.

Options: `--port 9000`, `--no-browser`, `--no-build` (reuse the last build).
Press Ctrl+C to stop. Needs only Python 3 and the tools from step 1.

Stop the preview before rebuilding the sim (Windows locks `program.exe` while it runs).

For still images instead, `pio run -e sim -t exec` writes `out/sheet.png` (8
frames of the drive) and `out/no_link.png`.

## Put it on the board

Plug the board in over USB-C, then:

```bash
pio run -e device-demo -t upload     # simulated ECU: the gauge "drives" itself
pio device monitor                   # serial log
```

Once the demo looks right, flash the build for how you will connect to the car:

| Environment   | Connection to the car                                      | Update rate |
| ------------- | ---------------------------------------------------------- | ----------- |
| `device-can`  | SN65HVD230 CAN transceiver wired to the OBD port (no dongle) | ~25 Hz      |
| `device-ble`  | Bluetooth LE ELM327 dongle                                 | ~8-12 Hz    |
| `device-wifi` | WiFi ELM327 dongle                                         | ~8-12 Hz    |
| `device-demo` | none, simulated ECU                                        | n/a         |

If the board doesn't enter upload mode, hold **BOOT**, tap **RESET**, release
BOOT, and retry.

### Wiring for `device-can`

| Transceiver  | Goes to                                                     |
| ------------ | ----------------------------------------------------------- |
| `TXD`        | ESP32 GPIO15 (`kCanTx` in `src/config.h`)                   |
| `RXD`        | ESP32 GPIO16 (`kCanRx`)                                     |
| `3V3`, `GND` | board 3V3 and GND                                           |
| `CANH`       | OBD pin 6                                                   |
| `CANL`       | OBD pin 14                                                  |
| (power)      | OBD pin 16 (12 V) via a 5 V buck converter to the board; OBD pin 4/5 = GND |

Check GPIO15/16 against your board's header silkscreen before wiring. Only
read-only mode 01 requests are sent.

### Settings

All pins, the BLE dongle name hint, WiFi SSID and the dongle address live in
[`src/config.h`](src/config.h). Gauge range, zones and tick spacing are
`ClassicLookConfig` in [`lib/core/src/ClassicLook.h`](lib/core/src/ClassicLook.h).

## How the repo is organised

```
lib/core        Hardware-free logic. Fully unit tested.
lib/fakes       Test doubles: FakeEcu, FakeCanBus, FakeElm327Stream, ...
lib/softcanvas  Software canvas + PNG writer (used by `sim` and render tests)
src/device      ESP32-only adapters: LgfxCanvas, TwaiBus, BleElmStream, WifiElmStream
src/sim         PC simulator: `Rig` (stack wired to mocks), stills and live streaming
tools/preview.* Live browser preview (Python server + page) over the sim's frame stream
tools/make_look.py  Turns look artwork (SVG, PNG) into the headers the firmware and sim compile in
looks/          Look artwork sources (retro/: spec.json, dial.svg.in, its font)
src/looks/      Generated artwork headers + the factory that builds the Retro look
                (src/looks/local/ is git-ignored: brand artwork lives only there)
src/main.cpp    Composition root: wires concrete classes, picks transport
src/config.h    Pins, SSID, addresses
test/test_core  doctest suites
tools/          coverage.sh and its PlatformIO helper
```

The design is a small set of narrow interfaces with the hardware behind them:

```
IClock  IByteStream  ICanBus  IObdClient            <- talking to the car
IReadingSource                                      <- handing readings to the screen
IDrawSurface  IFrameCache  (ICanvas = both + present)  <- drawing
ILook                                               <- a visual style
```

Each class has one job:

| Class | Job |
| ----- | --- |
| `Elm327Client` / `CanObdClient` | Speak ELM327 or CAN OBD over any `IByteStream` / `ICanBus` |
| `LinkSupervisor` | Connect, retry and notice a dead link |
| `AmbientPressure` | Barometric pressure, cached, with throttled retries |
| `BoostSampler` | Coordinate the above: MAP minus ambient = boost |
| `SamplerLoop` + `ReadingMailbox` | Publish readings from the sampler task to the screen (lock-free) |
| `GaugeModel` | Readings -> what to show: eased value, peak, link state, fps (pure logic) |
| `FrameRenderer` | Draw a frame: restore or cache the dial, draw the moving parts, present |
| `ILook` (`ClassicLook`, `RetroLook`) | Describe the drawing only: the dial and the moving parts |
| `GaugeApp` | Thin coordinator: source -> model -> renderer |

On the device a `SamplerTask` (core 0) runs `SamplerLoop::step()` forever; the render
loop (`loop()`, core 1) runs `GaugeApp::tick()`. They share only the mailbox.
`main.cpp` is the composition root, the only place that names concrete classes, and
`device/Transport.h` is the only place that knows which link to the car this build
uses. If you know .NET or TypeScript: interfaces are abstract classes, DI is
constructor injection, and doctest is your xUnit/Jest.

## Rendering and frame rate

* The panel is a fixed 240x240. Shapes are anti-aliased: the PC sim draws at 4x
  and averages down; on the device LovyanGFX does smooth lines/circles and
  `ArcCoverage` blends the dial arcs.
* The **dial** (zones, ticks, labels) never changes, so `FrameRenderer` has the look
  draw it once and the canvas caches it (`IFrameCache`; on the device the cache lives
  in PSRAM). Each frame only draws the needle and readout. A look never knows about
  the cache. If the cache is unavailable the dial is simply redrawn every frame.
* Text drawn live (the Classic look's readout, NO LINK, fps) still uses the built-in
  5x7 bitmap font. The Retro look's numerals and labels are part of its dial
  picture, so they are proper Jost lettering.
* **The screen never waits on the car.** Sampling runs on its own task and the
  render loop just reads the latest value from the mailbox, so a slow dongle or a
  dropped link cannot lower the frame rate or freeze the display.
* **Needle easing.** The displayed value glides toward the newest reading every
  frame (`Easer`, time constant `GaugeApp::Options::smoothingMs`, 80 ms), so motion
  is smooth even when readings arrive at only 5-25 Hz. It is frame-rate independent.
  Try the preview's **Data rate** control.
* **SPI at 80 MHz** (`kLcdSpiHz` in `src/config.h`): a full frame is about 11 ms
  versus 23 ms at 40 MHz. If the screen shows noise or tearing, drop it to 40 MHz.
* **fps counter** (`kShowFps`): the measured frame rate is drawn in the bottom
  corner of the dial. Use it at first flash to see the real figure.

None of the frame-rate numbers are measured on hardware yet; they come from the
SPI maths and the PC preview.

## Looks

A **look** (`ILook`) is a complete visual style. The app hands it the current
state each frame and it draws. Two exist today:

| Look | What it is |
| ---- | ---------- |
| **Classic** (`ClassicLook`) | Dark dial drawn entirely in code, coloured zones, big digital readout. |
| **Retro 60s** (`RetroLook`) | Cream dial that is a pre-rendered picture, slim red needle with a counterweight tail, a peak flag and a chrome-ringed hub. Purely analog. |

Pick one for the firmware with `config::kUseRetroLook` in `src/config.h`. In the
preview, use the **Look** menu to switch live (that is the switcher the board
will get once it has an input method).

**How the Retro look is built.** `looks/retro/spec.json` is the single source of truth
for the scale and pointer geometry (scale range, tick radii, needle length, hub size).
`tools/make_look.py retro` uses it twice: it fills the graduations, numerals and red
band into `looks/retro/dial.svg.in`, renders that with resvg and dithers it to RGB565
(`src/looks/RetroDial.h`, a 115 KB array in flash), and it writes the same numbers
to `src/looks/RetroSpec.h`, which `RetroLook` draws the needle with. So the picture
and the code cannot drift apart; a `static_assert` in the factory also checks the
start angle and sweep against `GaugeScale`. At run time the picture is drawn once, the
canvas caches it, and each frame only the needle, flag and hub are drawn on top.

```bash
pip install resvg-py pillow fonttools
python tools/make_look.py retro
```

Change `spec.json` or the template, re-run the command, and the sim preview and
firmware both pick up the change. The design mockups the SVG came from are on the design canvas
(see the project's Artifact link).

**The Cooper S badge** is brand artwork, so it is never committed. Put your PNG in
`assets/` (git-ignored), then:

```bash
python tools/make_look.py badge assets/brand/cooper-s.png src/looks/local/Badge.h --width 70
```

If `src/looks/local/Badge.h` exists, the Retro look draws the badge under the
5 psi numeral; if not, it is simply left out and everything still builds.

## Common tasks

**Add a look:** see the `add-look` skill (`.claude/skills/add-look/SKILL.md`). In short:
design the artwork, generate its picture, write an `ILook` (usually a
`ILook`), register it in the sim's `Rig::selectLook` and add tests.

**Add a transport** (e.g. Bluetooth Classic): implement `IByteStream` (ELM327
text) or `ICanBus` (raw CAN) in `src/device/`, add a `-DTRANSPORT_X` env in
`platformio.ini` with a matching `build_src_filter`, and add a branch in
`src/main.cpp`. Nothing in `lib/core` changes.

**Show another PID**: see the pattern in `BoostSampler` (read via
`IObdClient::readPid`, add the PID constant in `IObdClient.h`, teach `FakeEcu`
to answer it, test it).

**Change the look**: edit `ClassicLook.cpp`, run `python tools/preview.py`, and
watch it update (stop and re-run after each edit; it rebuilds the sim).

**Add a mock control**: add a command in `lib/fakes/src/SimCommands.cpp` (with a
test), then a button in `tools/preview.html` that sends it.

## Troubleshooting

| Symptom | Likely cause |
| ------- | ------------ |
| `g++` / `pio` not found | New terminal needed after install, or `PATH` missing the WinLibs / Python `Scripts` folder. `tools/preview.py` finds the WinGet copy of `g++` itself, but `pio test` and `tools/coverage.sh` need it on `PATH` |
| `pio test` says `SKIPPED, 0 test cases` | Known PlatformIO doctest reporting quirk; run `tools/coverage.sh` |
| Screen colours inverted | Toggle `cfg.invert` in `src/device/LgfxCanvas.cpp` |
| Gauge shows NO LINK in the car | Check wiring/pairing; confirm the car answers PID `0B` (try `010B` in an ELM327 terminal app) |
| Needle scale looks wrong | Confirm `GaugeConfig` min/max for your tune; boost is MAP minus barometric |

## Not yet verified on real hardware

These can only be confirmed with the board and car:

* LCD colour inversion and arc angle convention (`LgfxCanvas.cpp`)
* CAN pin choice, and that PIDs `0B`/`33` respond on your F56
* BLE dongle GATT layout (services `FFF0`/`FFE0`/`18F0` are assumed)

## Known MVP limits

* PID `0B` has 1 kPa resolution (about 0.15 psi steps); needle easing smooths it.
* The sampler task and the display are untested together on real hardware
  (task stack size, core pinning, and BLE/WiFi behaviour on core 0 are unverified).
* No Boost C++ libraries: too heavy for the target. The code is C++17 standard
  library only, with no heap use in the sampling path.
