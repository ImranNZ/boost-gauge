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
(`GaugeView`, `GaugeApp`, the OBD client) with a fake ECU on the other end. The
page has controls to drive the mock:

* **Drive cycle**: the scripted 20 s loop (idle vacuum, full-boost pull, lift-off, cruise)
* **Hold boost** slider: pin the ECU at any pressure from -15 to +25 psi
* **Drop link / Restore link**: see the NO LINK state and the sampler reconnecting

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
`GaugeConfig` in [`lib/core/src/GaugeView.h`](lib/core/src/GaugeView.h).

## How the repo is organised

```
lib/core        Hardware-free logic. Fully unit tested.
lib/fakes       Test doubles: FakeEcu, FakeCanBus, FakeElm327Stream, ...
lib/softcanvas  Software canvas + PNG writer (used by `sim` and render tests)
src/device      ESP32-only adapters: LgfxCanvas, TwaiBus, BleElmStream, WifiElmStream
src/sim         PC simulator: `Rig` (stack wired to mocks), stills and live streaming
tools/preview.* Live browser preview (Python server + page) over the sim's frame stream
src/main.cpp    Composition root: wires concrete classes, picks transport
src/config.h    Pins, SSID, addresses
test/test_core  doctest suites
tools/          coverage.sh and its PlatformIO helper
```

The design is a small set of interfaces with the hardware behind them:

```
IClock  IByteStream  ICanBus  IObdClient  ICanvas        <- the seams
```

`Elm327Client` turns any `IByteStream` (BLE, WiFi, fake) into an `IObdClient`;
`CanObdClient` does the same for any `ICanBus`. `BoostSampler` computes boost
and handles reconnects; `GaugeApp` runs sample -> smooth -> draw. `main.cpp` is
the only place that names concrete classes. If you know .NET or TypeScript:
interfaces are abstract classes, DI is constructor injection, and doctest is
your xUnit/Jest.

## Rendering and frame rate

* The panel is a fixed 240x240. Shapes are anti-aliased: the PC sim draws at 4x
  and averages down; on the device LovyanGFX does smooth lines/circles and
  `ArcCoverage` blends the dial arcs.
* The **dial** (zones, ticks, labels) never changes, so `GaugeView` draws it once
  and the canvas caches it (`saveBackground` / `restoreBackground`; on the device
  the cache lives in PSRAM). Each frame only draws the needle and readout. If
  the cache is unavailable the view just redraws the dial every frame.
* Text still uses the built-in 5x7 bitmap font. Smooth fonts are the next step.
* Frame rate is limited by the SPI transfer (about 23 ms per full frame at 40 MHz),
  not by drawing. Planned: 80 MHz SPI, a sampler task so a slow dongle cannot
  stall the screen, needle easing so motion is smooth at any data rate, and an
  fps counter for first flash. None of it can be measured until the board arrives.

## Common tasks

**Add a transport** (e.g. Bluetooth Classic): implement `IByteStream` (ELM327
text) or `ICanBus` (raw CAN) in `src/device/`, add a `-DTRANSPORT_X` env in
`platformio.ini` with a matching `build_src_filter`, and add a branch in
`src/main.cpp`. Nothing in `lib/core` changes.

**Show another PID**: see the pattern in `BoostSampler` (read via
`IObdClient::readPid`, add the PID constant in `IObdClient.h`, teach `FakeEcu`
to answer it, test it).

**Change the look**: edit `GaugeView.cpp`, run `python tools/preview.py`, and
watch it update (stop and re-run after each edit; it rebuilds the sim).

**Add a mock control**: add a command in `lib/fakes/src/EcuCommands.cpp` (with a
test), then a button in `tools/preview.html` that sends it.

## Troubleshooting

| Symptom | Likely cause |
| ------- | ------------ |
| `g++` / `pio` not found | New terminal needed after install, or `PATH` missing the WinLibs / Python `Scripts` folder |
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

* Sampling and drawing share one loop, so a slow dongle lowers the frame rate
  and a reconnect attempt briefly freezes the screen. Next step: a reader task
  plus a latest-value mailbox.
* PID `0B` has 1 kPa resolution (about 0.15 psi steps); `Ema` smooths it.
* No Boost C++ libraries: too heavy for the target. The code is C++17 standard
  library only, with no heap use in the sampling path.
