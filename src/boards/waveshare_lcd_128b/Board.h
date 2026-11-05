#pragma once

// Waveshare ESP32-S3-LCD-1.28-B: 1.28" round GC9A01 on SPI, 240x240, ESP32-S3 with PSRAM.

namespace board {

constexpr const char* kName = "Waveshare ESP32-S3-LCD-1.28-B";

constexpr int kWidth = 240;
constexpr int kHeight = 240;
// 240x240 in 16 bits is 115 KB: it fits (and is faster) in internal RAM.
constexpr bool kFrameInPsram = false;

// --- Display (GC9A01 over SPI) ---------------------------------------------
constexpr int kLcdSclk = 10;
constexpr int kLcdMosi = 11;
constexpr int kLcdDc = 8;
constexpr int kLcdCs = 9;
constexpr int kLcdRst = 12;
constexpr int kLcdBacklight = 40;
// SPI write clock. Waveshare quotes up to 80 MHz, which roughly doubles the
// frame-rate ceiling (a full frame is ~11 ms at 80 MHz, ~23 ms at 40 MHz). If the
// screen shows noise or tearing, drop this to 40000000.
constexpr int kLcdSpiHz = 80000000;

// --- CAN (TRANSPORT_CAN): SN65HVD230 transceiver on two spare header pins --
// VERIFY against the header silkscreen of your board before wiring; these two
// are not used by the LCD, IMU or USB.
constexpr int kCanTx = 15;
constexpr int kCanRx = 16;

}  // namespace board
