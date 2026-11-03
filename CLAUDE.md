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
  `src/main.cpp`. New hardware = new adapter in `src/device/`, not edits to core.
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

* Live preview: use `127.0.0.1`, not `localhost` (Windows tries IPv6 first and
  stalls). The sim's stdout is binary frame data, so log to stderr only. Stop
  `program.exe`/`preview.py` before rebuilding the sim, or the link step fails on Windows.

* `SoftCanvas` fonts only cover `0-9 - + . space A B E I K L N O P S T`. Using
  any other character in `GaugeView` renders nothing in the sim/tests. Add the
  glyph to `kGlyphs` in `SoftCanvas.cpp` first.
* The device font (`fonts::Font0`) and `SoftCanvas` share the 6x8 cell, so sim
  layouts match the screen. Keep text metrics in sync if either changes.
* `ManualClock::delayMs` advances time. That is what makes timeout loops
  terminate in tests; do not make a fake clock's `delayMs` a no-op.
* Angles: 0 = 3 o'clock, clockwise (screen space), everywhere.
* `LgfxCanvas.cpp` and the CAN pins have not been run on real hardware yet (see
  README "Not yet verified"). Do not state they are confirmed working.

## Where things go

| Change | Edit |
| ------ | ---- |
| New data source / link | new class in `src/device/` + env in `platformio.ini` + branch in `src/main.cpp` |
| New PID | constant in `IObdClient.h`, `FakeEcu::answer`, `BoostSampler` (or a sibling), tests |
| Gauge look | `lib/core/src/GaugeView.cpp`, then run the sim |
| Pins / SSID / BLE name | `src/config.h` |

Project skills live in `.claude/skills/` (`check`, `add-transport`, `add-pid`).
