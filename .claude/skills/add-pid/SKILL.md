---
name: add-pid
description: Add another OBD-II value to the boost gauge (e.g. intake air temp, coolant temp, RPM, throttle) with fake ECU support and tests. Use when the user wants to read or display another engine metric.
---

# Add an OBD-II PID

Read-only mode 01 PIDs only. Never add writes, actuator tests or UDS services.

1. **Constant**: add `kPid<Name>` in the `obd` namespace in
   `lib/core/src/obd/IObdClient.h` with a comment giving the unit/scaling.
2. **Decode**: put the formula (e.g. `A - 40` for coolant °C, `(256A+B)/4` for
   RPM) in a small pure function in `lib/core/src` (a header like `obd/Units.h`, or a
   new one). `ObdData` holds up to 4 data bytes (`bytes[0]` = A).
3. **Sampling**: extend `BoostSampler` only if it is part of boost; otherwise add a collaborator beside
   `AmbientPressure` / `LinkSupervisor` and a coordinator (single responsibility). Throttle slow-changing values like baro does
   (`AmbientPressure::Options::refreshMs` / `retryMs`) so unsupported PIDs don't stall every frame.
   The car's PID discovery (`obd/PidSupport.h`) says whether it implements the PID: use
   `support.knows(pid) && !support.supports(pid)` to skip a PID the car does not report, and treat
   an unknown one as possibly supported.
4. **Fake ECU**: teach `FakeEcu::answer` (`lib/fakes/src/FakeEcu.cpp`) to return
   it, and list it with `FakeEcu::setSupported` (the support bitmaps are generated from that set).
5. **Tests** (`test/test_core/`): decode formula incl. edge values, supported and
   unsupported paths, and throttling. Then `tools/coverage.sh` must stay at 100%.
6. **Display** (if shown): change `GaugeState` / the look(s). Only use glyphs in
   `SoftCanvas` (`0-9 - + . space A B C E F I K L M N O P S T U V`) or add new ones to
   `kGlyphs`. Run `pio run -e sim -t exec` and inspect `out/sheet.png`.
7. Confirm the F56 actually answers that PID before trusting it (an ELM327 app,
   `01xx`); say it is unverified until then.
