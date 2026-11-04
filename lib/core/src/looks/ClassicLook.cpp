#include "looks/ClassicLook.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

constexpr Color kVacuum{46, 134, 255};
constexpr Color kNormal{46, 204, 113};
constexpr Color kAmber{255, 176, 0};
constexpr Color kRed{255, 48, 48};
constexpr Color kTick{200, 200, 200};
constexpr Color kLabel{170, 170, 170};
constexpr Color kDim{120, 120, 120};
constexpr Color kText{255, 255, 255};
constexpr Color kHub{60, 60, 60};

constexpr int kZoneOuter = 119;
constexpr int kZoneInner = 112;
constexpr int kTickOuter = 108;
constexpr int kMajorInner = 96;
constexpr int kMinorInner = 101;
constexpr int kLabelRadius = 82;
constexpr float kNeedleLength = 94.0f;
constexpr float kNeedleBaseRadius = 5.0f;
constexpr float kNeedleTipRadius = 0.6f;

int polarX(int cx, float deg, int r) {
  return cx + static_cast<int>(std::lround(std::cos(deg * kDegToRad) * r));
}
int polarY(int cy, float deg, int r) {
  return cy + static_cast<int>(std::lround(std::sin(deg * kDegToRad) * r));
}

}  // namespace

float ClassicLook::angleFor(float psi) const {
  return GaugeScale{config_.minPsi, config_.maxPsi}.angleFor(psi);
}

void ClassicLook::drawDial(IDrawSurface& c) const {
  const int cx = c.width() / 2;
  const int cy = c.height() / 2;
  c.clear(kBackground);
  drawZones(c, cx, cy);
  drawScale(c, cx, cy);
}

void ClassicLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  const int cx = c.width() / 2;
  const int cy = c.height() / 2;

  if (state.linked) {
    if (state.peakPsi > 0.05f) drawPeakMarker(c, cx, cy, state.peakPsi);
    drawNeedle(c, cx, cy, state.psi);
  }
  c.fillCircle(cx, cy, 10, kHub);
  c.fillCircle(cx, cy, 4, kBackground);
  drawReadout(c, cx, state);
}

void ClassicLook::drawZones(IDrawSurface& c, int cx, int cy) const {
  struct Zone {
    float from = 0.0f;
    float to = 0.0f;
    Color color;
  };
  const Zone zones[] = {
      {config_.minPsi, 0.0f, kVacuum},
      {0.0f, config_.amberFromPsi, kNormal},
      {config_.amberFromPsi, config_.redFromPsi, kAmber},
      {config_.redFromPsi, config_.maxPsi, kRed},
  };
  for (const Zone& z : zones) {
    c.fillArc(cx, cy, kZoneInner, kZoneOuter, angleFor(z.from), angleFor(z.to), z.color);
  }
}

void ClassicLook::drawScale(IDrawSurface& c, int cx, int cy) const {
  const float step = config_.majorStepPsi / static_cast<float>(config_.minorPerMajor);
  for (int i = 0;; ++i) {
    const float psi = config_.minPsi + static_cast<float>(i) * step;
    if (psi > config_.maxPsi + 1e-3f) break;

    const bool major = (i % config_.minorPerMajor) == 0;
    const float deg = angleFor(psi);
    const int inner = major ? kMajorInner : kMinorInner;
    c.drawLine(polarX(cx, deg, inner), polarY(cy, deg, inner), polarX(cx, deg, kTickOuter),
               polarY(cy, deg, kTickOuter), major ? 2 : 1, kTick);

    if (major) {
      char label[8];
      std::snprintf(label, sizeof(label), "%d", static_cast<int>(std::lround(psi)));
      // Scale-2 glyphs are 16 px tall; centre vertically on the radius.
      c.drawText(polarX(cx, deg, kLabelRadius), polarY(cy, deg, kLabelRadius) - 8, label, 2,
                 TextAlign::Center, kLabel);
    }
  }
}

void ClassicLook::drawPeakMarker(IDrawSurface& c, int cx, int cy, float peakPsi) const {
  const float deg = angleFor(peakPsi);
  c.drawLine(polarX(cx, deg, kMajorInner - 6), polarY(cy, deg, kMajorInner - 6),
             polarX(cx, deg, kTickOuter), polarY(cy, deg, kTickOuter), 3, kText);
}

void ClassicLook::drawNeedle(IDrawSurface& c, int cx, int cy, float psi) const {
  // Float coordinates (no rounding) so the needle glides instead of stepping.
  const float rad = angleFor(psi) * kDegToRad;
  const float tipX = static_cast<float>(cx) + std::cos(rad) * kNeedleLength;
  const float tipY = static_cast<float>(cy) + std::sin(rad) * kNeedleLength;
  c.drawWedge(static_cast<float>(cx), static_cast<float>(cy), tipX, tipY, kNeedleBaseRadius,
              kNeedleTipRadius, kNeedle);
}

void ClassicLook::drawReadout(IDrawSurface& c, int cx, const GaugeState& state) const {
  char text[16];
  if (state.linked) {
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(state.psi));
  } else {
    std::snprintf(text, sizeof(text), "--.-");
  }
  // Sits in the gap under the hub, clear of the bottom-corner scale labels.
  c.drawText(cx, 134, text, 4, TextAlign::Center, state.linked ? kText : kDim);
  c.drawText(cx, 170, "PSI", 2, TextAlign::Center, kDim);

  if (!state.linked) {
    c.drawText(cx, 74, "NO LINK", 2, TextAlign::Center, kRed);
  } else if (state.peakPsi > 0.05f) {
    std::snprintf(text, sizeof(text), "PK %.1f", static_cast<double>(state.peakPsi));
    c.drawText(cx, 204, text, 2, TextAlign::Center, kDim);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    c.drawText(cx, 224, text, 1, TextAlign::Center, kDim);
  }
}

}  // namespace gauge
