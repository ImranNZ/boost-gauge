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
