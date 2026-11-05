#pragma once

#include "gauge/ChannelProfile.h"
#include "sampling/ChannelKind.h"

// Deployment settings: what the gauge shows, and how it reaches the car (protocol, SSID, address).
// Anything tied to the board (screen size, display and CAN pins) lives in src/boards/<name>/ and is
// picked by the -DBOARD_<NAME> flag in platformio.ini, so nothing else hard-codes a pin.

namespace config {

// --- Look ---------------------------------------------------------------------
// Retro: cream 60s dial with a red needle. Classic: dark dial with zones, drawn in code.
// Minimal: no needle, a thin glowing arc around a big number.
// Motorsport: segmented bar that lights up, with a shift-light rim past 20 psi.
// Modern: black dial in a chrome ring with an orange needle (the F56 interior).
enum class Look { Retro, Classic, Minimal, Motorsport, Modern };
constexpr Look kLook = Look::Retro;

// --- What the gauge shows ------------------------------------------------------
// The reading: Boost, Coolant (engine coolant temperature), IntakeAir (intake air temperature) or
// Voltage (battery voltage). Only the chosen one is read from the car. Retro and Modern are
// pictures of a psi boost dial, so a non-boost channel needs Classic, Minimal or Motorsport
// (checked at build time in main.cpp).
constexpr gauge::ChannelKind kChannel = gauge::ChannelKind::Boost;

// --- Gauge profile -----------------------------------------------------------
// The unit, the scale's range and where the amber and red zones start, for the channel above. The
// default is the channel's own: for boost, psi from -15 to +25 with red from 20, which suits a
// stock turbo like the F56's. Other units: `GaugeProfile::bar()` and `::kpa()` for boost,
// `::coolantF()` and `::intakeAirF()` for Fahrenheit. Every field can be set for a car with more or
// less range, e.g.
//   gauge::GaugeProfile{gauge::Unit::Psi, -15, 35, 5, 25, 30}   // a tuned car
// Classic, Minimal and Motorsport follow it.
inline constexpr gauge::GaugeProfile kProfile = gauge::defaultProfile(kChannel);

// --- PID discovery -----------------------------------------------------------
// Each time the link comes up the gauge asks the car which mode 01 PIDs it implements. Set this to
// print the answer to the serial console (115200 baud): the "PID scanner", handy for seeing what
// another car offers before trusting the gauge on it.
constexpr bool kLogPidSupport = false;

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

// --- BLE ELM327 (TRANSPORT_BLE) ---------------------------------------------
// Case-sensitive substring of the dongle's advertised name.
constexpr const char* kBleNameHint = "OBD";

// --- WiFi ELM327 (TRANSPORT_WIFI): typical dongle defaults -------------------
constexpr const char* kWifiSsid = "WiFi_OBDII";
constexpr const char* kWifiPassword = "";  // most dongles are open networks
constexpr const char* kElmHost = "192.168.0.10";
constexpr int kElmPort = 35000;

}  // namespace config
