#include "looks/RetroLook.h"

#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

// Styling of the live parts. (Where they sit comes from the Spec.)
constexpr Color kShadow{190, 179, 147};  // the needle's shadow, tinted for the cream dial
constexpr Color kHubDark{27, 27, 28};
constexpr Color kHubChrome{169, 175, 183};
constexpr Color kHubCap{5, 5, 6};
constexpr Color kNoLink{161, 29, 20};
constexpr Color kDimInk{107, 100, 85};

constexpr float kShadowDx = 1.2f;
constexpr float kShadowDy = 2.2f;
constexpr float kNeedleBaseRadius = 2.6f;
constexpr float kNeedleTipRadius = 0.55f;
constexpr float kTailBaseRadius = 3.0f;
constexpr float kTailEndRadius = 1.6f;
constexpr float kCounterweightRadius = 3.4f;
constexpr float kFlagBaseRadius = 3.6f;
constexpr float kFlagTipRadius = 0.5f;

// Text baselines, in pixels below the dial centre.
constexpr int kNoLinkBaselineY = 28;
constexpr int kFpsBaselineY = 75;

}  // namespace

RetroLook::RetroLook(Picture dial, Spec spec, const Fonts& fonts)
    : dial_(dial), spec_(spec), fonts_(fonts), scale_{spec.minPsi, spec.maxPsi} {}

RetroLook::RetroLook(Picture dial, Spec spec, const Fonts& fonts, const Badge& badge)
    : dial_(dial),
      spec_(spec),
      fonts_(fonts),
      scale_{spec.minPsi, spec.maxPsi},
      badge_(badge),
      hasBadge_(true) {}

void RetroLook::drawDial(IDrawSurface& c) const {
  dial_.draw(c, 0, 0);
  if (hasBadge_) badge_.picture.draw(c, badge_.x, badge_.y);
}

void RetroLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
  const int cx = c.width() / 2;
  const int cy = c.height() / 2;

  if (state.linked) {
    if (state.peakValue > 0.05f) {
      drawPeakFlag(c, static_cast<float>(cx), static_cast<float>(cy),
                   scale_.angleFor(state.peakValue));
    }
    drawNeedle(c, static_cast<float>(cx), static_cast<float>(cy), scale_.angleFor(state.value));
  }
  drawHub(c, cx, cy);

  if (!state.linked) {
    fonts_.notice.draw(c, cx, cy + kNoLinkBaselineY, "NO LINK", TextAlign::Center, kNoLink);
  }

  if (state.fps > 0) {
    char text[16];
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    fonts_.small.draw(c, cx, cy + kFpsBaselineY, text, TextAlign::Center, kDimInk);
  }
}

void RetroLook::drawNeedle(IDrawSurface& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  drawNeedleShape(c, cx + kShadowDx, cy + kShadowDy, dx, dy, kShadow);
  drawNeedleShape(c, cx, cy, dx, dy, kNeedleRed);
}

// A slim pointer with a tapered tail and a small counterweight. Built from
// anti-aliased wedges so it needs nothing beyond the IDrawSurface primitives.
void RetroLook::drawNeedleShape(IDrawSurface& c, float cx, float cy, float dx, float dy,
                                Color color) const {
  const float tail = spec_.tailLength;
  c.drawWedge(cx, cy, cx + dx * spec_.needleLength, cy + dy * spec_.needleLength, kNeedleBaseRadius,
              kNeedleTipRadius, color);
  c.drawWedge(cx - dx * tail, cy - dy * tail, cx, cy, kTailEndRadius, kTailBaseRadius, color);
  c.drawWedge(cx - dx * (tail - 2.0f), cy - dy * (tail - 2.0f), cx - dx * (tail + 0.5f),
              cy - dy * (tail + 0.5f), kCounterweightRadius, kCounterweightRadius, color);
}

void RetroLook::drawPeakFlag(IDrawSurface& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  c.drawWedge(cx + dx * spec_.flagOuterRadius, cy + dy * spec_.flagOuterRadius,
              cx + dx * spec_.flagInnerRadius, cy + dy * spec_.flagInnerRadius, kFlagBaseRadius,
              kFlagTipRadius, kInk);
}

void RetroLook::drawHub(IDrawSurface& c, int cx, int cy) const {
  const int r = spec_.hubRadius;
  c.fillCircle(cx, cy, r, kHubDark);
  c.fillCircle(cx, cy, r - 1, kHubChrome);
  c.fillCircle(cx, cy, r - 2, kHubDark);
  c.fillCircle(cx, cy, 2, kHubCap);
}

}  // namespace gauge
