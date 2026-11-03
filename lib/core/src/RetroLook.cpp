#include "RetroLook.h"

#include <cmath>
#include <cstdio>

namespace gauge {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

constexpr Color kShadow{190, 179, 147};  // the needle's shadow, tinted for the cream dial
constexpr Color kHubDark{27, 27, 28};
constexpr Color kHubChrome{169, 175, 183};
constexpr Color kHubCap{5, 5, 6};
constexpr Color kNoLink{161, 29, 20};
constexpr Color kDimInk{107, 100, 85};

constexpr float kNeedleLength = 91.0f;
constexpr float kTailLength = 27.0f;
constexpr float kShadowDx = 1.2f;
constexpr float kShadowDy = 2.2f;

}  // namespace

RetroLook::RetroLook(ImageView dial) : dial_(dial) {}

RetroLook::RetroLook(ImageView dial, Badge badge)
    : dial_(dial), badge_(badge), hasBadge_(true) {}

void RetroLook::drawDial(ICanvas& c) const {
  c.drawImage(dial_, 0, 0);
  if (hasBadge_) c.drawImage(badge_.image, badge_.x, badge_.y);
}

void RetroLook::drawDynamic(ICanvas& c, const GaugeState& state) const {
  const int cx = c.width() / 2;
  const int cy = c.height() / 2;

  if (state.linked) {
    if (state.peakPsi > 0.05f) {
      drawPeakFlag(c, static_cast<float>(cx), static_cast<float>(cy),
                   scale_.angleFor(state.peakPsi));
    }
    drawNeedle(c, static_cast<float>(cx), static_cast<float>(cy), scale_.angleFor(state.psi));
  }
  drawHub(c, cx, cy);

  if (!state.linked) c.drawText(cx, cy + 14, "NO LINK", 2, TextAlign::Center, kNoLink);

  if (state.fps > 0) {
    char text[16];
    std::snprintf(text, sizeof(text), "%d FPS", state.fps);
    c.drawText(cx, cy + 68, text, 1, TextAlign::Center, kDimInk);
  }
}

void RetroLook::drawNeedle(ICanvas& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  drawNeedleShape(c, cx + kShadowDx, cy + kShadowDy, dx, dy, kShadow);
  drawNeedleShape(c, cx, cy, dx, dy, kNeedleRed);
}

// A slim pointer with a tapered tail and a small counterweight. Built from
// anti-aliased wedges so it needs nothing beyond the ICanvas primitives.
void RetroLook::drawNeedleShape(ICanvas& c, float cx, float cy, float dx, float dy,
                                Color color) const {
  c.drawWedge(cx, cy, cx + dx * kNeedleLength, cy + dy * kNeedleLength, 2.6f, 0.55f, color);
  c.drawWedge(cx - dx * kTailLength, cy - dy * kTailLength, cx, cy, 1.6f, 3.0f, color);
  c.drawWedge(cx - dx * 25.0f, cy - dy * 25.0f, cx - dx * 27.5f, cy - dy * 27.5f, 3.4f, 3.4f,
              color);
}

void RetroLook::drawPeakFlag(ICanvas& c, float cx, float cy, float deg) const {
  const float dx = std::cos(deg * kDegToRad);
  const float dy = std::sin(deg * kDegToRad);
  c.drawWedge(cx + dx * 107.0f, cy + dy * 107.0f, cx + dx * 99.0f, cy + dy * 99.0f, 3.6f, 0.5f,
              kInk);
}

void RetroLook::drawHub(ICanvas& c, int cx, int cy) const {
  c.fillCircle(cx, cy, 8, kHubDark);
  c.fillCircle(cx, cy, 7, kHubChrome);
  c.fillCircle(cx, cy, 6, kHubDark);
  c.fillCircle(cx, cy, 2, kHubCap);
}

}  // namespace gauge
