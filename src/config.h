#pragma once

// Board and deployment settings. Everything environment-specific lives here so
// the rest of the firmware never hard-codes a pin, SSID or address.

namespace config {

// --- Display: Waveshare ESP32-S3-LCD-1.28-B (GC9A01, 240x240, SPI) ---------
constexpr int kLcdSclk = 10;
constexpr int kLcdMosi = 11;
constexpr int kLcdDc = 8;
constexpr int kLcdCs = 9;
constexpr int kLcdRst = 12;
constexpr int kLcdBacklight = 40;
constexpr int kLcdSize = 240;
// SPI write clock. Waveshare quotes up to 80 MHz, which roughly doubles the
// frame-rate ceiling (a full frame is ~11 ms at 80 MHz, ~23 ms at 40 MHz). If the
// screen shows noise or tearing, drop this to 40000000.
constexpr int kLcdSpiHz = 80000000;

// --- Look ---------------------------------------------------------------------
// Retro: cream 60s dial with a red needle. Classic: dark dial with zones, drawn in code.
// Minimal: no needle, a thin glowing arc around a big number.
enum class Look { Retro, Classic, Minimal };
constexpr Look kLook = Look::Retro;

// --- Diagnostics -------------------------------------------------------------
// Show the measured frames-per-second in the bottom corner of the dial.
constexpr bool kShowFps = true;

// --- CAN (TRANSPORT_CAN): SN65HVD230 transceiver on two spare header pins --
// VERIFY against the header silkscreen of your board before wiring; these two
// are not used by the LCD, IMU or USB.
constexpr int kCanTx = 15;
constexpr int kCanRx = 16;

// --- BLE ELM327 (TRANSPORT_BLE) ---------------------------------------------
// Case-sensitive substring of the dongle's advertised name.
constexpr const char* kBleNameHint = "OBD";

// --- WiFi ELM327 (TRANSPORT_WIFI): typical dongle defaults -------------------
constexpr const char* kWifiSsid = "WiFi_OBDII";
constexpr const char* kWifiPassword = "";  // most dongles are open networks
constexpr const char* kElmHost = "192.168.0.10";
constexpr int kElmPort = 35000;

}  // namespace config
