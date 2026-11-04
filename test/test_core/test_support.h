#pragma once

// Shared helpers for the test suites. Each test file is named for the unit it
// tests; anything used by more than one file lives here.

#include <doctest.h>

#include <array>
#include <cmath>
#include <deque>
#include <string>
#include <vector>

#include "sampling/AmbientPressure.h"
#include "render/ArcCoverage.h"
#include "sampling/BoostSampler.h"
#include "obd/CanObdClient.h"
#include "looks/ClassicLook.h"
#include "obd/Elm327Client.h"
#include "obd/Elm327Parse.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "FakeElm327Stream.h"
#include "render/Font.h"
#include "gauge/FpsCounter.h"
#include "looks/FrameRenderer.h"
#include "app/GaugeApp.h"
#include "gauge/GaugeModel.h"
#include "gauge/GaugeScale.h"
#include "Image.h"
#include "render/ImageView.h"
#include "sampling/LinkSupervisor.h"
#include "ManualClock.h"
#include "looks/MinimalLook.h"
#include "looks/ModernLook.h"
#include "looks/MotorsportLook.h"
#include "sampling/ReadingMailbox.h"
#include "sampling/ReadingSource.h"
#include "looks/RetroLook.h"
#include "sampling/SamplerLoop.h"
#include "ScriptedByteStream.h"
#include "SimCommands.h"
#include "gauge/Smoothing.h"
#include "SoftCanvas.h"
#include "sampling/SpscRing.h"
#include "StubObdClient.h"
#include "ThrottledSource.h"
#include "obd/Units.h"

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

/// A font whose glyphs are solid boxes (a space is empty), so a test can find text by
/// where it is rather than what it looks like. `height` is also the ascent: glyphs sit
/// on the baseline.
class SolidFont {
 public:
  SolidFont(int glyphWidth, int glyphHeight, int advance) : height_(glyphHeight) {
    const std::string chars = "0123456789-+. ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (char ch : chars) {
      const bool space = ch == ' ';
      const int w = space ? 0 : glyphWidth;
      const int h = space ? 0 : glyphHeight;
      glyphs_.push_back(Glyph{ch, static_cast<uint8_t>(w), static_cast<uint8_t>(h), 1,
                              static_cast<int8_t>(-h), static_cast<uint8_t>(advance),
                              static_cast<uint32_t>(alpha_.size())});
      alpha_.insert(alpha_.end(), static_cast<size_t>(w * h), uint8_t{255});
    }
  }
  SolidFont(const SolidFont&) = delete;
  SolidFont& operator=(const SolidFont&) = delete;

  Font font() const {
    return Font(glyphs_.data(), static_cast<int>(glyphs_.size()), alpha_.data(), height_);
  }

 private:
  int height_;
  std::vector<Glyph> glyphs_;
  std::vector<uint8_t> alpha_;
};

inline const SolidFont kNumberFont(20, 30, 24);
inline const SolidFont kCaptionFont(7, 11, 9);
inline const SolidFont kLabelFont(6, 10, 8);

inline MinimalLook::Fonts minimalFonts() { return {kNumberFont.font(), kLabelFont.font()}; }
inline MotorsportLook::Fonts motorsportFonts() {
  return {kNumberFont.font(), kLabelFont.font(), kCaptionFont.font()};
}

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
