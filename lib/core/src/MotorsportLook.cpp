#include "MotorsportLook.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegPerSegment = GaugeScale::kSweepDeg / MotorsportLook::kSegments;
constexpr float kSegmentGapDeg = 0.55f;  // trimmed from each side of a segment

constexpr int kCx = 120;
constexpr int kCy = 120;
constexpr int kBarInner = 90;
constexpr int kBarOuter = 108;
constexpr int kRimInner = 115;
constexpr int kRimOuter = 118;
constexpr float kDegToRad = 3.14159265f / 180.0f;

constexpr Color kBlue{59, 139, 255};
constexpr Color kTeal{25, 211, 162};
constexpr Color kLime{155, 224, 74};
constexpr Color kWhite{255, 255, 255};
constexpr Color kHotText{255, 90, 77};
constexpr Color kLabel{139, 147, 160};

constexpr int kLabelRadius = 70;
constexpr float kLabelPsi[] = {-15.0f, 0.0f, 10.0f, 20.0f, 25.0f};

// Text rows (top edge, pixels). The badge goes above them (see MotorsportLookFactory.h).
constexpr int kBoostY = 78;
constexpr int kReadoutScale = 4;
constexpr int kReadoutY = 96;
constexpr int kUnitY = 130;
constexpr int kPeakY = 196;
constexpr int kFpsY = 218;

constexpr float kMinDrawnPsi = 0.05f;
constexpr float kPulseMsPerRadian = 90.0f;

int polarX(float deg, int r) {
  return kCx + static_cast<int>(std::lround(std::cos(deg * kDegToRad) * static_cast<float>(r)));
}
int polarY(float deg, int r) {
  return kCy + static_cast<int>(std::lround(std::sin(deg * kDegToRad) * static_cast<float>(r)));
}

float segmentStart(int index) {
  return GaugeScale::kStartDeg + static_cast<float>(index) * kDegPerSegment;
}

}  // namespace

Color MotorsportLook::segmentColor(int index) {
  const float mid = GaugeScale{}.minPsi + static_cast<float>(index) + 0.5f;  // the segment's centre
  if (mid < 0.0f) return kBlue;
  if (mid < 10.0f) return kTeal;
  if (mid < 15.0f) return kLime;
  if (mid < kShiftPsi) return kAmber;
  return kRed;
}

bool MotorsportLook::isLit(int index, float psi) {
  return psi >= GaugeScale{}.minPsi + static_cast<float>(index) + 0.5f;
}

int MotorsportLook::segmentFor(float psi) {
  return std::clamp(static_cast<int>(std::floor(psi - GaugeScale{}.minPsi)), 0, kSegments - 1);
}

void MotorsportLook::drawDial(IDrawSurface& c) const {
  c.clear(kBackground);
  for (int i = 0; i < kSegments; ++i) drawSegment(c, i, 0.0f, kSegmentOff);
  drawRim(c, GaugeState{});

  const GaugeScale scale;
  for (float psi : kLabelPsi) {
    char label[8];
    std::snprintf(label, sizeof(label), "%d", static_cast<int>(std::lround(psi)));
    const float deg = scale.angleFor(psi);
    // Scale-2 glyphs are 14 px tall; centre them vertically on the radius.
    c.drawText(polarX(deg, kLabelRadius), polarY(deg, kLabelRadius) - 7, label, 2,
               TextAlign::Center, kLabel);
  }
  c.drawText(kCx, kBoostY, "BOOST", 2, TextAlign::Center, kLabel);
  c.drawText(kCx, kUnitY, "PSI", 2, TextAlign::Center, kLabel);

  if (badge_) c.drawImage(badge_->image, badge_->x, badge_->y);
}

void MotorsportLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  if (state.linked) {
    drawBar(c, state.psi);
    if (state.peakPsi > kMinDrawnPsi) drawPeak(c, state.peakPsi, state.psi);
    drawRim(c, state);
  }
  drawReadout(c, state);
}

// A segment of the bar, trimmed by `inset` pixels on every side (the peak outline).
void MotorsportLook::drawSegment(IDrawSurface& c, int index, float inset, Color color) const {
  const float from = segmentStart(index) + kSegmentGapDeg + inset;
  const float to = segmentStart(index) + kDegPerSegment - kSegmentGapDeg - inset;
  c.fillArc(kCx, kCy, kBarInner + static_cast<int>(inset), kBarOuter - static_cast<int>(inset),
            from, to, color);
}

void MotorsportLook::drawBar(IDrawSurface& c, float psi) const {
  for (int i = 0; i < kSegments && isLit(i, psi); ++i) drawSegment(c, i, 0.0f, segmentColor(i));
}

// The segment the pressure peaked in is outlined in white: a white segment with
// the segment's own colour redrawn inside it.
void MotorsportLook::drawPeak(IDrawSurface& c, float peakPsi, float psi) const {
  const int index = segmentFor(peakPsi);
  drawSegment(c, index, 0.0f, kWhite);
  drawSegment(c, index, 1.0f, isLit(index, psi) ? segmentColor(index) : kSegmentOff);
}

// The rim: dim normally, pulsing red once past the shift point.
void MotorsportLook::drawRim(IDrawSurface& c, const GaugeState& state) const {
  Color color = kRimOff;
  if (state.linked && state.psi > kShiftPsi) {
    const float t = static_cast<float>(state.timeMs) / kPulseMsPerRadian;
    const float pulse = 0.55f + 0.45f * std::sin(t);
    color = blend(kRimOff, kRed, static_cast<int>(std::lround(pulse * 255.0f)));
  }
  // Two halves: an arc's sweep wraps at 360, so one full-circle arc would draw nothing.
  c.fillArc(kCx, kCy, kRimInner, kRimOuter, 0.0f, 180.0f, color);
  c.fillArc(kCx, kCy, kRimInner, kRimOuter, 180.0f, 360.0f, color);
}

void MotorsportLook::drawReadout(IDrawSurface& c, const GaugeState& state) const {
  char text[16];
  if (state.linked) {
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(state.psi));
    c.drawText(kCx, kReadoutY, text, kReadoutScale, TextAlign::Center,
               state.psi > kShiftPsi ? kHotText : kWhite);
  } else {
    c.drawText(kCx, kReadoutY, "--.-", kReadoutScale, TextAlign::Center, kLabel);
    c.drawText(kCx, kBoostY, "NO LINK", 2, TextAlign::Center, kRed);
  }

  if (state.linked && state.peakPsi > kMinDrawnPsi) {
    std::snprintf(text, sizeof(text), "PEAK %.1f", static_cast<double>(state.peakPsi));
    c.drawText(kCx, kPeakY, text, 2, TextAlign::Center, kAmber);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    c.drawText(kCx, kFpsY, text, 1, TextAlign::Center, kLabel);
  }
}

}  // namespace gauge
