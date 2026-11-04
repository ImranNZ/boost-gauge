Firmware images for the boost gauge (an F56 Mini Cooper S on a Waveshare ESP32-S3-LCD-1.28-B round display). See the [README](https://github.com/ImranNZ/boost-gauge#readme) for what it does and pictures of the looks.

### Which image to flash

| File prefix | Connection to the car |
| --- | --- |
| `device-can` | Direct CAN via an SN65HVD230 transceiver on the OBD port |
| `device-ble` | Bluetooth LE ELM327 dongle |
| `device-wifi` | WiFi ELM327 dongle |
| `device-demo` | No car: a simulated ECU, for the bench |

Flash each set with `esptool` at `0x0` (bootloader), `0x8000` (partitions), `0xe000` (boot_app0) and `0x10000` (firmware), or build from source with PlatformIO (see the README). Check the files against `SHA256SUMS`. The gauge only sends read-only OBD-II requests.

---

