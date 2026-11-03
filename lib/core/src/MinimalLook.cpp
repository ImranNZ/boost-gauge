#include "MinimalLook.h"

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

// The heat ramp for boost: pressure and colour at each stop, blended in between.
struct Stop {
  float psi;
  Color color;
};
constexpr Stop kRamp[] = {
    {0.0f, {47, 224, 176}},
    {12.0f, {155, 224, 74}},
    {17.0f, {255, 176, 32}},
    {21.0f, {255, 77, 77}},
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

constexpr int kLabelRadius = 74;
constexpr float kLabelPsi[] = {-15.0f, 0.0f, 10.0f, 20.0f, 25.0f};

// Text rows (top edge, pixels) and sizes.
// From the top: badge (see MinimalLookFactory.h), mode label, number, unit. The number
// stops above the "20" label so a five-character "-11.0" cannot run into it.
constexpr int kModeY = 68;
constexpr int kReadoutScale = 5;
constexpr int kReadoutY = 88;
constexpr int kUnitY = 132;
constexpr int kPeakY = 196;
constexpr int kFpsY = 224;

constexpr float kMinDrawnPsi = 0.05f;  // closer to zero than this, draw no arc

int polarX(float deg, int r) {
  return kCx + static_cast<int>(std::lround(std::cos(deg * kDegToRad) * static_cast<float>(r)));
}
int polarY(float deg, int r) {
  return kCy + static_cast<int>(std::lround(std::sin(deg * kDegToRad) * static_cast<float>(r)));
}

}  // namespace

Color MinimalLook::colorFor(float psi) {
  if (psi < 0.0f) return kVacuum;
  constexpr int kLast = sizeof(kRamp) / sizeof(kRamp[0]) - 1;
  if (psi >= kRamp[kLast].psi) return kRamp[kLast].color;
  int i = 1;
  while (psi > kRamp[i].psi) ++i;
  const float f = (psi - kRamp[i - 1].psi) / (kRamp[i].psi - kRamp[i - 1].psi);
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

  for (float psi : kLabelPsi) {
    char label[8];
    std::snprintf(label, sizeof(label), "%d", static_cast<int>(std::lround(psi)));
    const float deg = scale_.angleFor(psi);
    // Scale-2 glyphs are 14 px tall; centre them vertically on the radius.
    c.drawText(polarX(deg, kLabelRadius), polarY(deg, kLabelRadius) - 7, label, 2,
               TextAlign::Center, kLabel);
  }

  if (badge_) c.drawImage(badge_->image, badge_->x, badge_->y);
}

void MinimalLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  if (state.linked) {
    drawArc(c, state.psi);
    if (state.peakPsi > kMinDrawnPsi) drawPeak(c, state.peakPsi);
  }
  drawReadout(c, state);
}

void MinimalLook::drawArc(IDrawSurface& c, float psi) const {
  if (std::fabs(psi) < kMinDrawnPsi) return;

  const float zero = scale_.angleFor(0.0f);
  const float tip = scale_.angleFor(psi);
  const float from = std::min(zero, tip);
  const float to = std::max(zero, tip);
  const Color color = colorFor(psi);

  for (const Glow& g : kGlow) {
    c.fillArc(kCx, kCy, g.inner, g.outer, from, to, blend(kBackground, color, g.alpha));
  }
  c.fillArc(kCx, kCy, kArcInner, kArcOuter, from, to, color);
  c.fillCircle(polarX(zero, kArcRadius), polarY(zero, kArcRadius), kCapRadius, color);
  c.fillCircle(polarX(tip, kArcRadius), polarY(tip, kArcRadius), kCapRadius, color);
}

void MinimalLook::drawPeak(IDrawSurface& c, float peakPsi) const {
  const float deg = scale_.angleFor(peakPsi);
  const int x = polarX(deg, kArcRadius);
  const int y = polarY(deg, kArcRadius);
  c.fillCircle(x, y, 5, kBackground);
  c.fillCircle(x, y, 3, kText);
}

void MinimalLook::drawReadout(IDrawSurface& c, const GaugeState& state) const {
  char text[16];
  if (state.linked) {
    c.drawText(kCx, kModeY, state.psi >= 0.0f ? "BOOST" : "VACUUM", 2, TextAlign::Center,
               colorFor(state.psi));
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(state.psi));
    c.drawText(kCx, kReadoutY, text, kReadoutScale, TextAlign::Center, kText);
  } else {
    c.drawText(kCx, kModeY, "NO LINK", 2, TextAlign::Center, kNoLink);
    c.drawText(kCx, kReadoutY, "--.-", kReadoutScale, TextAlign::Center, kLabel);
  }
  c.drawText(kCx, kUnitY, "PSI", 2, TextAlign::Center, kLabel);

  if (state.linked && state.peakPsi > kMinDrawnPsi) {
    std::snprintf(text, sizeof(text), "PEAK %.1f", static_cast<double>(state.peakPsi));
    c.drawText(kCx, kPeakY, text, 2, TextAlign::Center, kSubtle);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    c.drawText(kCx, kFpsY, text, 1, TextAlign::Center, kLabel);
  }
}

}  // namespace gauge
