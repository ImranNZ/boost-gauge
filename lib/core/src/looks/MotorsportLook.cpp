#include "looks/MotorsportLook.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr int kMaxSegments = 200;
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

constexpr int kLabelRadius = 77;

// Text baselines (pixels from the top) and letter-spacing, as in the design.
constexpr int kBoostBaseline = 82;
constexpr int kBoostTracking = 5;
constexpr int kReadoutCentreY = 118;
constexpr int kUnitBaseline = 154;
constexpr int kUnitTracking = 5;
constexpr int kPeakBaseline = 174;
constexpr int kPeakTracking = 2;
constexpr int kFpsBaseline = 222;

constexpr float kMinDrawnPsi = 0.05f;
constexpr float kPulseMsPerRadian = 90.0f;

int polarX(float deg, int r) {
  return kCx + static_cast<int>(std::lround(std::cos(deg * kDegToRad) * static_cast<float>(r)));
}
int polarY(float deg, int r) {
  return kCy + static_cast<int>(std::lround(std::sin(deg * kDegToRad) * static_cast<float>(r)));
}

int segmentCountFor(const GaugeProfile& profile) {
  const long count = std::lround((profile.max - profile.min) / profile.segmentStep());
  return static_cast<int>(std::clamp(count, 1L, static_cast<long>(kMaxSegments)));
}

}  // namespace

MotorsportLook::MotorsportLook(const Fonts& fonts, const GaugeProfile& profile)
    : fonts_(fonts), profile_(profile), segments_(segmentCountFor(profile)) {}

MotorsportLook::MotorsportLook(const Fonts& fonts, const GaugeProfile& profile, const Badge& badge)
    : fonts_(fonts), profile_(profile), segments_(segmentCountFor(profile)), badge_(badge) {}

float MotorsportLook::segmentStartDeg(int index) const {
  return GaugeScale::kStartDeg +
         static_cast<float>(index) * (GaugeScale::kSweepDeg / static_cast<float>(segments_));
}

Color MotorsportLook::segmentColor(int index) const {
  // The segment's centre, in the profile's unit.
  const float mid = profile_.min + (static_cast<float>(index) + 0.5f) * profile_.segmentStep();
  if (mid < 0.0f) return kBlue;
  if (mid < profile_.amberFrom * (2.0f / 3.0f)) return kTeal;
  if (mid < profile_.amberFrom) return kLime;
  if (mid < profile_.redFrom) return kAmber;
  return kRed;
}

bool MotorsportLook::isLit(int index, float value) const {
  return value >= profile_.min + (static_cast<float>(index) + 0.5f) * profile_.segmentStep();
}

int MotorsportLook::segmentFor(float value) const {
  return std::clamp(static_cast<int>(std::floor((value - profile_.min) / profile_.segmentStep())),
                    0, segments_ - 1);
}

void MotorsportLook::drawDial(IDrawSurface& c) const {
  c.clear(kBackground);
  for (int i = 0; i < segments_; ++i) drawSegment(c, i, 0.0f, kSegmentOff);
  drawRim(c, GaugeState{});

  const GaugeScale scale = profile_.scale();
  for (int k = 0;; ++k) {
    const float value = profile_.min + static_cast<float>(k) * profile_.majorStep;
    if (value > profile_.max + 1e-3f) break;
    char label[8];
    std::snprintf(label, sizeof(label), "%.*f", profile_.labelDecimals(),
                  static_cast<double>(value));
    const float deg = scale.angleFor(value);
    // Centre the digits on the label radius: the baseline sits half a cap height lower.
    fonts_.scale.draw(c, polarX(deg, kLabelRadius),
                      polarY(deg, kLabelRadius) + fonts_.scale.ascent() / 2, label,
                      TextAlign::Center, kLabel);
  }
  fonts_.caption.draw(c, kCx, kBoostBaseline, profile_.label, TextAlign::Center, kLabel,
                      kBoostTracking);
  fonts_.caption.draw(c, kCx, kUnitBaseline, profile_.unitLabel(), TextAlign::Center, kLabel,
                      kUnitTracking);

  if (badge_) badge_->picture.draw(c, badge_->x, badge_->y);
}

void MotorsportLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  if (state.linked) {
    const float value = profile_.toDisplay(state.value);
    drawBar(c, value);
    if (state.peakValue > kMinDrawnPsi) drawPeak(c, profile_.toDisplay(state.peakValue), value);
    drawRim(c, state);
  }
  drawReadout(c, state);
}

// A segment of the bar, trimmed by `inset` pixels on every side (the peak outline).
void MotorsportLook::drawSegment(IDrawSurface& c, int index, float inset, Color color) const {
  const float degPerSegment = GaugeScale::kSweepDeg / static_cast<float>(segments_);
  const float from = segmentStartDeg(index) + kSegmentGapDeg + inset;
  const float to = segmentStartDeg(index) + degPerSegment - kSegmentGapDeg - inset;
  c.fillArc(kCx, kCy, kBarInner + static_cast<int>(inset), kBarOuter - static_cast<int>(inset),
            from, to, color);
}

void MotorsportLook::drawBar(IDrawSurface& c, float value) const {
  for (int i = 0; i < segments_ && isLit(i, value); ++i) drawSegment(c, i, 0.0f, segmentColor(i));
}

// The segment the pressure peaked in is outlined in white: a white segment with
// the segment's own colour redrawn inside it.
void MotorsportLook::drawPeak(IDrawSurface& c, float peakValue, float value) const {
  const int index = segmentFor(peakValue);
  drawSegment(c, index, 0.0f, kWhite);
  drawSegment(c, index, 1.0f, isLit(index, value) ? segmentColor(index) : kSegmentOff);
}

// The rim: dim normally, pulsing red once past the shift point.
void MotorsportLook::drawRim(IDrawSurface& c, const GaugeState& state) const {
  Color color = kRimOff;
  if (state.linked && profile_.toDisplay(state.value) > profile_.redFrom) {
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
  const int numberBaseline = kReadoutCentreY + fonts_.number.ascent() / 2;
  if (state.linked) {
    const float value = profile_.toDisplay(state.value);
    std::snprintf(text, sizeof(text), "%.*f", profile_.decimals(), static_cast<double>(value));
    fonts_.number.draw(c, kCx, numberBaseline, text, TextAlign::Center,
                       value > profile_.redFrom ? kHotText : kWhite);
  } else {
    fonts_.number.draw(c, kCx, numberBaseline, "--.-", TextAlign::Center, kLabel);
    fonts_.caption.draw(c, kCx, kBoostBaseline, "NO LINK", TextAlign::Center, kRed, kBoostTracking);
  }

  if (state.linked && state.peakValue > kMinDrawnPsi) {
    std::snprintf(text, sizeof(text), "PEAK %.*f", profile_.decimals(),
                  static_cast<double>(profile_.toDisplay(state.peakValue)));
    fonts_.caption.draw(c, kCx, kPeakBaseline, text, TextAlign::Center, kAmber, kPeakTracking);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    fonts_.scale.draw(c, kCx, kFpsBaseline, text, TextAlign::Center, kLabel);
  }
}

}  // namespace gauge
