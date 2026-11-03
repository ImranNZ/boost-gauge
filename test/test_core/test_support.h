#pragma once

// Shared helpers for the test suites. Each test file is named for the unit it
// tests; anything used by more than one file lives here.

#include <doctest.h>

#include <array>
#include <cmath>
#include <deque>
#include <string>
#include <vector>

#include "AmbientPressure.h"
#include "ArcCoverage.h"
#include "BoostSampler.h"
#include "CanObdClient.h"
#include "ClassicLook.h"
#include "Elm327Client.h"
#include "Elm327Parse.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "FakeElm327Stream.h"
#include "FpsCounter.h"
#include "FrameRenderer.h"
#include "GaugeApp.h"
#include "GaugeModel.h"
#include "GaugeScale.h"
#include "Image.h"
#include "ImageView.h"
#include "LinkSupervisor.h"
#include "ManualClock.h"
#include "MinimalLook.h"
#include "MotorsportLook.h"
#include "ReadingMailbox.h"
#include "ReadingSource.h"
#include "RetroLook.h"
#include "SamplerLoop.h"
#include "ScriptedByteStream.h"
#include "SimCommands.h"
#include "Smoothing.h"
#include "SoftCanvas.h"
#include "SpscRing.h"
#include "StubObdClient.h"
#include "ThrottledSource.h"
#include "Units.h"

namespace support {

using namespace gauge;

// ---------------------------------------------------------------- colours --

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

inline bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
inline bool partial(Color c) { return c.r > 0 && c.r < 255; }

// ----------------------------------------------------------- reading pixels --

inline int countNonBlack(const Image& img) {
  int n = 0;
  for (int y = 0; y < img.height(); ++y)
    for (int x = 0; x < img.width(); ++x)
      if (!same(img.get(x, y), kBlack)) ++n;
  return n;
}

inline int columnHeight(const SoftCanvas& canvas, int x) {
  int n = 0;
  for (int y = 0; y < canvas.height(); ++y)
    if (!same(canvas.image().get(x, y), kBlack)) ++n;
  return n;
}

inline int countColor(const SoftCanvas& canvas, Color c, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x)
      if (same(canvas.image().get(x, y), c)) ++n;
  return n;
}

/// The pixel `radius` from the dial centre along the angle for `psi` (default
/// -15..25 scale). Negative radius looks behind the pivot (the needle's tail).
inline Color pointOnNeedle(const SoftCanvas& canvas, float psi, float radius) {
  const float rad = GaugeScale{}.angleFor(psi) * 3.14159265f / 180.0f;
  return canvas.image().get(120 + static_cast<int>(std::lround(std::cos(rad) * radius)),
                            120 + static_cast<int>(std::lround(std::sin(rad) * radius)));
}

inline Color pixelOnNeedle(const SoftCanvas& canvas, const ClassicLook& look, float psi,
                           int radius) {
  const float deg = look.angleFor(psi) * 3.14159265f / 180.0f;
  return canvas.image().get(120 + static_cast<int>(std::lround(std::cos(deg) * radius)),
                            120 + static_cast<int>(std::lround(std::sin(deg) * radius)));
}

inline Color needlePixel(const SoftCanvas& canvas, const ClassicLook& look, float psi) {
  return pixelOnNeedle(canvas, look, psi, 60);
}

/// Render one frame of `look` into `canvas` exactly as the app does.
inline void drawFrame(const ILook& look, SoftCanvas& canvas, const GaugeState& state) {
  FrameRenderer renderer(canvas);
  renderer.render(look, state);
}

// --------------------------------------------------------- pictures & looks --

/// A 240x240 dial of one flat colour, standing in for the real artwork.
class FlatDial {
 public:
  explicit FlatDial(uint16_t value) : pixels_(240 * 240, value) {}
  ImageView view() const { return {240, 240, pixels_.data(), nullptr}; }

 private:
  std::vector<uint16_t> pixels_;
};

constexpr uint16_t kCream = (30 << 11) | (57 << 5) | 25;  // ~ (247, 229, 205)

/// The geometry the real Retro artwork is generated with (looks/retro/spec.json).
constexpr RetroLook::Spec kRetroSpec{-15.0f, 25.0f, 91.0f, 27.0f, 99.0f, 107.0f, 8};

// ----------------------------------------------------------- reading sources --

/// A source whose sample the test sets directly.
class FixedSource : public IReadingSource {
 public:
  Sample latest() override { return sample; }
  void publish(float psi, bool valid = true) { sample = {{psi, valid}, ++seq}; }

  Sample sample;
  uint16_t seq = 0;
};

// -------------------------------------------------------- ELM327 and CAN rigs --

inline const char* const kOk = "OK\r\r>";

inline std::vector<std::string> happyHandshake() {
  return {"\r\rELM327 v1.5\r\r>", kOk, kOk, kOk, kOk, "41 00 00 20 00 01\r\r>"};
}

/// CAN bus double: replays queued frames, optionally burning time per receive.
class ScriptedBus : public ICanBus {
 public:
  explicit ScriptedBus(ManualClock& clock) : clock_(clock) {}

  bool sendResult = true;
  uint32_t msPerReceive = 0;
  std::deque<CanFrame> frames;

  bool begin() override { return true; }
  bool send(const CanFrame&) override { return sendResult; }
  bool receive(CanFrame& out, uint32_t) override {
    clock_.advance(msPerReceive);
    if (frames.empty()) return false;
    out = frames.front();
    frames.pop_front();
    return true;
  }

 private:
  ManualClock& clock_;
};

inline CanFrame response(uint32_t id, std::array<uint8_t, 8> data, uint8_t len = 8) {
  CanFrame f;
  f.id = id;
  f.data = data;
  f.len = len;
  return f;
}

}  // namespace support
