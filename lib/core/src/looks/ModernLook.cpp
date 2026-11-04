#include "looks/ModernLook.h"

#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

// Styling of the live parts. (Where they sit comes from the Spec.)
constexpr Color kShadow{2, 2, 3};
constexpr Color kHubRim{10, 10, 11};
constexpr Color kHubChrome{170, 176, 183};
constexpr Color kHubCap{21, 23, 26};
constexpr Color kReadout{255, 255, 255};
constexpr Color kDim{154, 160, 167};
constexpr Color kNoLink{255, 90, 77};

constexpr float kShadowDx = 1.2f;
constexpr float kShadowDy = 2.4f;
constexpr float kNeedleBaseRadius = 3.4f;
constexpr float kNeedleTipRadius = 0.7f;
constexpr float kTailEndRadius = 2.6f;
constexpr float kFlagBaseRadius = 3.8f;
constexpr float kFlagTipRadius = 0.5f;
constexpr int kHubCapRadius = 3;

// Where the text sits (pixels from the top / centre), as in the design.
constexpr int kCx = 120;
constexpr int kReadoutCentreY = 164;  // middle of the window the picture has
constexpr int kNoLinkBaseline = 136;
constexpr int kNoLinkTracking = 3;
constexpr int kFpsBaseline = 214;

}  // namespace

void ModernLook::drawDial(IDrawSurface& c) const {
  c.drawImage(dial_, 0, 0);
  if (badge_) c.drawImage(badge_->image, badge_->x, badge_->y);
}

void ModernLook::drawDynamic(IDrawSurface& c, const GaugeState& state) const {
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
  drawReadout(c, state);
}

void ModernLook::drawNeedle(IDrawSurface& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  drawNeedleShape(c, cx + kShadowDx, cy + kShadowDy, dx, dy, kShadow);
  drawNeedleShape(c, cx, cy, dx, dy, kNeedle);
}

// A slim tapered pointer with a short tail, from anti-aliased wedges.
void ModernLook::drawNeedleShape(IDrawSurface& c, float cx, float cy, float dx, float dy,
                                 Color color) const {
  c.drawWedge(cx, cy, cx + dx * spec_.needleLength, cy + dy * spec_.needleLength, kNeedleBaseRadius,
              kNeedleTipRadius, color);
  c.drawWedge(cx - dx * spec_.tailLength, cy - dy * spec_.tailLength, cx, cy, kTailEndRadius,
              kNeedleBaseRadius, color);
}

void ModernLook::drawPeakFlag(IDrawSurface& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  c.drawWedge(cx + dx * spec_.flagOuterRadius, cy + dy * spec_.flagOuterRadius,
              cx + dx * spec_.flagInnerRadius, cy + dy * spec_.flagInnerRadius, kFlagBaseRadius,
              kFlagTipRadius, kFlag);
}

void ModernLook::drawHub(IDrawSurface& c, int cx, int cy) const {
  c.fillCircle(cx, cy, spec_.hubRadius, kHubRim);
  c.fillCircle(cx, cy, spec_.hubRadius - 1, kHubChrome);
  c.fillCircle(cx, cy, kHubCapRadius, kHubCap);
}

void ModernLook::drawReadout(IDrawSurface& c, const GaugeState& state) const {
  const int baseline = kReadoutCentreY + fonts_.readout.ascent() / 2;
  if (state.linked) {
    char text[16];
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(state.value));
    fonts_.readout.draw(c, kCx, baseline, text, TextAlign::Center, kReadout);
  } else {
    fonts_.readout.draw(c, kCx, baseline, "--.-", TextAlign::Center, kDim);
    fonts_.caption.draw(c, kCx, kNoLinkBaseline, "NO LINK", TextAlign::Center, kNoLink,
                        kNoLinkTracking);
  }

  if (state.fps > 0) {
    char text[16];
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    fonts_.caption.draw(c, kCx, kFpsBaseline, text, TextAlign::Center, kDim);
  }
}

}  // namespace gauge
