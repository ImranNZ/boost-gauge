---
name: add-transport
description: Add a new way for the boost gauge to talk to the car (e.g. Bluetooth Classic ELM327, USB serial, a different CAN driver). Use when the user wants a new connection type or hardware adapter.
---

# Add a transport

A transport is an adapter behind an existing interface. `lib/core` should not change.

1. **Pick the seam**
   - ELM327-style text adapter (BLE, WiFi, serial, Bluetooth Classic): implement
     `gauge::IByteStream` (`connect`, `connected`, `write`, `read` returning -1 when
     empty, never blocking). `Elm327Client` does the rest.
   - Raw CAN controller: implement `gauge::ICanBus`. `CanObdClient` does the rest.
2. **Write the adapter** in `src/device/<Name>.h/.cpp`, `namespace device`.
   Radio callbacks run on another task: hand bytes over with
   `gauge::SpscRing` (see `BleElmStream`). Put any pins/SSIDs in `src/config.h`.
3. **Register the build** in `platformio.ini`: copy `[env:device-wifi]`, new
   `-DTRANSPORT_<NAME>` flag, and `build_src_filter = ${device.build_src_filter} +<device/<Name>.cpp>`
   so other environments don't compile it. Add extra `lib_deps` only in that env.
4. **Wire it** in `src/main.cpp`: an `#elif defined(TRANSPORT_<NAME>)` in the
   include block and in the object block. Update the `#error` message.
5. **Test the logic you can.** Hardware calls can't run on a PC, so keep the
   adapter thin; anything with logic (framing, retries, parsing) belongs in
   `lib/core` behind an interface with tests and keeps the 100% gate.
6. Compile it: `pio run -e device-<name>`, plus `pio run -e device-demo` to be sure
   nothing else broke. Run `tools/coverage.sh`.
7. Add a row to the README environment table and say plainly that the adapter is
   unverified on real hardware.
