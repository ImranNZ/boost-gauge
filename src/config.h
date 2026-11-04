#pragma once

#include "gauge/GaugeProfile.h"

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
// Motorsport: segmented bar that lights up, with a shift-light rim past 20 psi.
// Modern: black dial in a chrome ring with an orange needle (the F56 interior).
enum class Look { Retro, Classic, Minimal, Motorsport, Modern };
constexpr Look kLook = Look::Retro;

// --- Gauge profile -----------------------------------------------------------
// What the gauge shows and where its zones start: the unit (psi, bar or kPa), the scale's range
// and the amber and red points. The default is psi, -15 to +25, red from 20: a stock turbo
// like the F56's. `GaugeProfile::bar()` and `GaugeProfile::kpa()` are metric equivalents, and
// every field can be set for a car with more or less boost, e.g.
//   gauge::GaugeProfile{gauge::Unit::Psi, -15, 35, 5, 25, 30}   // a tuned car
// Classic, Minimal and Motorsport follow it. Retro and Modern are pictures of a psi dial (see
// looks/<name>/spec.json).
inline constexpr gauge::GaugeProfile kProfile = gauge::GaugeProfile::psi();

// --- Diagnostics -------------------------------------------------------------
// Show the measured frames-per-second in the bottom corner of the dial.
constexpr bool kShowFps = true;

// --- Car bus protocol ---------------------------------------------------------
// ELM327 dongles (BLE and WiFi): the adapter's `ATSP` protocol digit. "6" is ISO 15765-4 CAN,
// 11-bit IDs, 500 kbit/s: the F56, and most cars from about 2008. "0" lets the dongle search for
// whatever the car speaks (slower to connect); see Elm327Client::Options for the others, e.g. "7"
// for 29-bit CAN or "8" for 250 kbit/s.
constexpr const char* kElmProtocol = "6";
// Direct CAN (TRANSPORT_CAN): the bus speed in kbit/s: 125, 250, 500 or 1000. The direct client
// speaks 11-bit OBD-II only; for a 29-bit car use an ELM327 dongle with protocol "7" or "9".
constexpr int kCanBitrateKbps = 500;

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
