#include "looks/MinimalLook.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

constexpr Color kVacuum{74, 163, 255};
constexpr Color kText{255, 255, 255};
constexpr Color kLabel{138, 147, 157};
constexpr Color kSubtle{207, 213, 219};
constexpr Color kZeroTick{231, 234, 238};
constexpr Color kNoLink{255, 77, 77};

// The heat ramp for boost: where each colour sits, as a fraction of the pressure at which the
// red zone starts (0, 12, 17 and 21 psi for the default profile), blended in between.
struct Stop {
  float atRed = 0.0f;
  Color color;
};
constexpr Stop kRamp[] = {
    {0.0f, {47, 224, 176}},
    {0.6f, {155, 224, 74}},
    {0.85f, {255, 176, 32}},
    {1.05f, {255, 77, 77}},
};

constexpr int kCx = 120;
constexpr int kCy = 120;
constexpr int kArcRadius = 100;  // centre line of the 10 px arc
constexpr int kArcInner = 95;
constexpr int kArcOuter = 105;
constexpr int kCapRadius = 5;  // half the arc's width: rounds its ends

// Two soft bands behind the arc stand in for a glow: the arc's colour at low alpha.
struct Glow {
  int inner;
  int outer;
  int alpha;
};
constexpr Glow kGlow[] = {{88, 112, 26}, {92, 108, 56}};

constexpr int kLabelRadius = 82;
constexpr int kMaxLabels = 8;  // beyond the ends and zero

// Text baselines (pixels from the top) and letter-spacing, as in the design.
constexpr int kModeBaseline = 82;
constexpr int kModeTracking = 4;
constexpr int kReadoutCentreY = 120;
constexpr int kUnitBaseline = 158;
constexpr int kUnitTracking = 4;
constexpr int kPeakBaseline = 182;
constexpr int kPeakTracking = 2;
constexpr int kFpsBaseline = 224;

constexpr float kMinDrawnPsi = 0.05f;  // closer to zero than this, draw no arc

int polarX(float deg, int r) {
  return kCx + static_cast<int>(std::lround(std::cos(deg * kDegToRad) * static_cast<float>(r)));
}
int polarY(float deg, int r) {
  return kCy + static_cast<int>(std::lround(std::sin(deg * kDegToRad) * static_cast<float>(r)));
}

/// Where the numbers go on the scale: both ends, zero, and every second labelled step between
/// (-15, 0, 10, 20, 25 for the default profile). Returns how many were written to `out`.
int scaleLabels(const GaugeProfile& profile, float* out) {
  int count = 0;
  out[count++] = profile.min;
  if (profile.min < 0.0f) out[count++] = 0.0f;
  for (int k = 1; k <= kMaxLabels; ++k) {
    const float value = 2.0f * profile.majorStep * static_cast<float>(k);
    if (value > profile.max - 0.5f * profile.majorStep) break;
    out[count++] = value;
  }
  if (profile.max - out[count - 1] > 1e-3f) out[count++] = profile.max;
  return count;
}

}  // namespace

Color MinimalLook::colorFor(float value) const {
  if (value < 0.0f) return kVacuum;
  constexpr int kLast = sizeof(kRamp) / sizeof(kRamp[0]) - 1;
  const float red = profile_.redFrom;
  if (value >= kRamp[kLast].atRed * red) return kRamp[kLast].color;
  int i = 1;
  while (value > kRamp[i].atRed * red) ++i;
  const float from = kRamp[i - 1].atRed * red;
  const float to = kRamp[i].atRed * red;
  const float f = (value - from) / (to - from);
  return blend(kRamp[i - 1].color, kRamp[i].color, static_cast<int>(std::lround(f * 255.0f)));
}

void MinimalLook::drawDial(IDrawSurface& c) const {
  c.clear(kBackground);

  // The empty track, and the zero mark the arc grows from.
  const float start = GaugeScale::kStartDeg;
  const float end = start + GaugeScale::kSweepDeg;
  c.fillArc(kCx, kCy, kArcInner, kArcOuter, start, end, kTrack);
  c.fillCircle(polarX(start, kArcRadius), polarY(start, kArcRadius), kCapRadius, kTrack);
  c.fillCircle(polarX(end, kArcRadius), polarY(end, kArcRadius), kCapRadius, kTrack);
  const float zero = scale_.angleFor(0.0f);
  c.drawLine(polarX(zero, 90), polarY(zero, 90), polarX(zero, 110), polarY(zero, 110), 2,
             kZeroTick);

  float labels[kMaxLabels + 3];
  const int labelCount = scaleLabels(profile_, labels);
  for (int i = 0; i < labelCount; ++i) {
    char label[8];
    std::snprintf(label, sizeof(label), "%.*f", profile_.labelDecimals(),
                  static_cast<double>(labels[i]));
    const float deg = scale_.angleFor(labels[i]);
    // Centre the digits on the label radius: the baseline sits half a cap height lower.
    fonts_.label.draw(c, polarX(deg, kLabelRadius),
                      polarY(deg, kLabelRadius) + fonts_.label.ascent() / 2, label,
                      TextAlign::Center, kLabel);
  }

  if (badge_) badge_->picture.draw(c, badge_->x, badge_->y);
}

void MinimalLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  if (state.linked) {
    drawArc(c, state.value);
    if (state.peakValue > kMinDrawnPsi) drawPeak(c, state.peakValue);
  }
  drawReadout(c, state);
}

void MinimalLook::drawArc(IDrawSurface& c, float psi) const {
  if (std::fabs(psi) < kMinDrawnPsi) return;

  const float value = profile_.toDisplay(psi);
  const float zero = scale_.angleFor(0.0f);
  const float tip = scale_.angleFor(value);
  const float from = std::min(zero, tip);
  const float to = std::max(zero, tip);
  const Color color = colorFor(value);

  for (const Glow& g : kGlow) {
    c.fillArc(kCx, kCy, g.inner, g.outer, from, to, blend(kBackground, color, g.alpha));
  }
  c.fillArc(kCx, kCy, kArcInner, kArcOuter, from, to, color);
  c.fillCircle(polarX(zero, kArcRadius), polarY(zero, kArcRadius), kCapRadius, color);
  c.fillCircle(polarX(tip, kArcRadius), polarY(tip, kArcRadius), kCapRadius, color);
}

void MinimalLook::drawPeak(IDrawSurface& c, float peakValue) const {
  const float deg = scale_.angleFor(profile_.toDisplay(peakValue));
  const int x = polarX(deg, kArcRadius);
  const int y = polarY(deg, kArcRadius);
  c.fillCircle(x, y, 5, kBackground);
  c.fillCircle(x, y, 3, kText);
}

void MinimalLook::drawReadout(IDrawSurface& c, const GaugeState& state) const {
  char text[16];
  const int numberBaseline = kReadoutCentreY + fonts_.number.ascent() / 2;
  if (state.linked) {
    const float value = profile_.toDisplay(state.value);
    const bool below = state.value < 0.0f && profile_.negativeLabel != nullptr;
    fonts_.label.draw(c, kCx, kModeBaseline, below ? profile_.negativeLabel : profile_.label,
                      TextAlign::Center, colorFor(value), kModeTracking);
    std::snprintf(text, sizeof(text), "%.*f", profile_.decimals(), static_cast<double>(value));
    fonts_.number.draw(c, kCx, numberBaseline, text, TextAlign::Center, kText);
  } else {
    fonts_.label.draw(c, kCx, kModeBaseline, "NO LINK", TextAlign::Center, kNoLink, kModeTracking);
    fonts_.number.draw(c, kCx, numberBaseline, "--.-", TextAlign::Center, kLabel);
  }
  fonts_.label.draw(c, kCx, kUnitBaseline, profile_.unitLabel(), TextAlign::Center, kLabel,
                    kUnitTracking);

  if (state.linked && state.peakValue > kMinDrawnPsi) {
    std::snprintf(text, sizeof(text), "PEAK %.*f", profile_.decimals(),
                  static_cast<double>(profile_.toDisplay(state.peakValue)));
    fonts_.label.draw(c, kCx, kPeakBaseline, text, TextAlign::Center, kSubtle, kPeakTracking);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    fonts_.label.draw(c, kCx, kFpsBaseline, text, TextAlign::Center, kLabel);
  }
}

}  // namespace gauge
