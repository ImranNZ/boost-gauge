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
// Text baselines (pixels from the top). The readout sits in the gap under the hub, clear of the
// bottom-corner scale labels.
constexpr int kNumberBaseline = 162;
constexpr int kUnitBaseline = 184;
constexpr int kNoLinkBaseline = 88;
constexpr int kPeakBaseline = 218;
constexpr int kFpsBaseline = 231;
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

float ClassicLook::angleFor(float value) const { return profile_.scale().angleFor(value); }

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
    if (state.peakValue > 0.05f) drawPeakMarker(c, cx, cy, profile_.toDisplay(state.peakValue));
    drawNeedle(c, cx, cy, profile_.toDisplay(state.value));
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
      {profile_.min, 0.0f, kVacuum},
      {0.0f, profile_.amberFrom, kNormal},
      {profile_.amberFrom, profile_.redFrom, kAmber},
      {profile_.redFrom, profile_.max, kRed},
  };
  for (const Zone& z : zones) {
    c.fillArc(cx, cy, kZoneInner, kZoneOuter, angleFor(z.from), angleFor(z.to), z.color);
  }
}

void ClassicLook::drawScale(IDrawSurface& c, int cx, int cy) const {
  const float step = profile_.majorStep / static_cast<float>(kMinorPerMajor);
  for (int i = 0;; ++i) {
    const float value = profile_.min + static_cast<float>(i) * step;
    if (value > profile_.max + 1e-3f) break;

    const bool major = (i % kMinorPerMajor) == 0;
    const float deg = angleFor(value);
    const int inner = major ? kMajorInner : kMinorInner;
    c.drawLine(polarX(cx, deg, inner), polarY(cy, deg, inner), polarX(cx, deg, kTickOuter),
               polarY(cy, deg, kTickOuter), major ? 2 : 1, kTick);

    if (major) {
      char label[8];
      std::snprintf(label, sizeof(label), "%.*f", profile_.labelDecimals(),
                    static_cast<double>(value));
      // Centre the digits vertically on the radius.
      fonts_.label.draw(c, polarX(cx, deg, kLabelRadius),
                        polarY(cy, deg, kLabelRadius) + fonts_.label.ascent() / 2, label,
                        TextAlign::Center, kLabel);
    }
  }
}

void ClassicLook::drawPeakMarker(IDrawSurface& c, int cx, int cy, float peakValue) const {
  const float deg = angleFor(peakValue);
  c.drawLine(polarX(cx, deg, kMajorInner - 6), polarY(cy, deg, kMajorInner - 6),
             polarX(cx, deg, kTickOuter), polarY(cy, deg, kTickOuter), 3, kText);
}

void ClassicLook::drawNeedle(IDrawSurface& c, int cx, int cy, float value) const {
  // Float coordinates (no rounding) so the needle glides instead of stepping.
  const float rad = angleFor(value) * kDegToRad;
  const float tipX = static_cast<float>(cx) + std::cos(rad) * kNeedleLength;
  const float tipY = static_cast<float>(cy) + std::sin(rad) * kNeedleLength;
  c.drawWedge(static_cast<float>(cx), static_cast<float>(cy), tipX, tipY, kNeedleBaseRadius,
              kNeedleTipRadius, kNeedle);
}

void ClassicLook::drawReadout(IDrawSurface& c, int cx, const GaugeState& state) const {
  char text[16];
  if (state.linked) {
    std::snprintf(text, sizeof(text), "%.*f", profile_.decimals(),
                  static_cast<double>(profile_.toDisplay(state.value)));
  } else {
    std::snprintf(text, sizeof(text), "--.-");
  }
  fonts_.number.draw(c, cx, kNumberBaseline, text, TextAlign::Center, state.linked ? kText : kDim);
  fonts_.label.draw(c, cx, kUnitBaseline, profile_.unitLabel(), TextAlign::Center, kDim);

  if (!state.linked) {
    fonts_.label.draw(c, cx, kNoLinkBaseline, "NO LINK", TextAlign::Center, kRed);
  } else if (state.peakValue > 0.05f) {
    std::snprintf(text, sizeof(text), "PK %.*f", profile_.decimals(),
                  static_cast<double>(profile_.toDisplay(state.peakValue)));
    fonts_.label.draw(c, cx, kPeakBaseline, text, TextAlign::Center, kDim);
  }

  if (state.fps > 0) {
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    fonts_.small.draw(c, cx, kFpsBaseline, text, TextAlign::Center, kDim);
  }
}

}  // namespace gauge
