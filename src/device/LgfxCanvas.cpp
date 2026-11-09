#include "device/LgfxCanvas.h"

#include <cmath>

#include "render/ArcCoverage.h"

namespace device {

LgfxCanvas::LgfxCanvas() : panel_(new board::Panel()), frame_(panel_), dial_(panel_) {}

bool LgfxCanvas::begin() {
  panel_->init();
  panel_->setBrightness(200);

  frame_.setColorDepth(16);
  frame_.setPsram(board::kFrameInPsram);  // internal RAM is faster to push, if it fits
  if (frame_.createSprite(board::kWidth, board::kHeight) == nullptr) return false;

  // Optional: without it restoreBackground() reports false and the view simply
  // redraws the dial every frame (slower, but correct).
  dial_.setColorDepth(16);
  dial_.setPsram(true);
  hasDial_ = false;
  dialAllocated_ = dial_.createSprite(board::kWidth, board::kHeight) != nullptr;
  return true;
}

int LgfxCanvas::width() const { return board::kWidth; }
int LgfxCanvas::height() const { return board::kHeight; }

void LgfxCanvas::clear(gauge::Color color) { frame_.fillScreen(rgb(color)); }

void LgfxCanvas::fillCircle(int cx, int cy, int radius, gauge::Color color) {
  frame_.fillSmoothCircle(cx, cy, radius, rgb(color));
}

// LovyanGFX has no anti-aliased arc, so blend the ring edges ourselves using the
// shared coverage maths. Only used for the dial, which is drawn once and cached.
void LgfxCanvas::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                         float endDeg, gauge::Color color) {
  const float start = gauge::wrapDegrees(startDeg);
  const float sweep = gauge::wrapDegrees(endDeg - startDeg);
  const int reach = outerRadius + 1;

  for (int y = cy - reach; y <= cy + reach; ++y) {
    for (int x = cx - reach; x <= cx + reach; ++x) {
      const float coverage = gauge::arcCoverage(
          static_cast<float>(x - cx), static_cast<float>(y - cy), static_cast<float>(innerRadius),
          static_cast<float>(outerRadius), start, sweep);
      if (coverage <= 0.0f) continue;
      if (coverage >= 1.0f) {
        frame_.drawPixel(x, y, rgb(color));
        continue;
      }
      const int alpha = static_cast<int>(coverage * 255.0f + 0.5f);
      frame_.drawPixel(x, y, rgb(gauge::blend(readColor(x, y), color, alpha)));
    }
  }
}

void LgfxCanvas::drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                           gauge::Color color) {
  frame_.drawWedgeLine(x0, y0, x1, y1, r0, r1, rgb(color));
}

void LgfxCanvas::drawLine(int x0, int y0, int x1, int y1, int thickness, gauge::Color color) {
  const float radius = static_cast<float>(thickness < 1 ? 1 : thickness) * 0.5f;
  frame_.drawWideLine(x0, y0, x1, y1, radius, rgb(color));
}

void LgfxCanvas::drawImage(const gauge::ImageView& image, int x, int y) {
  if (image.rgb565 == nullptr) return;

  if (image.alpha == nullptr) {
    // Opaque (the dial): one bulk copy straight from flash. rgb565_t is the plain
    // 16-bit RGB565 layout, which is what the generated arrays hold.
    frame_.pushImage(x, y, image.width, image.height,
                     reinterpret_cast<const lgfx::rgb565_t*>(image.rgb565));
    return;
  }

  // With alpha (a badge): blend pixel by pixel. It is small and drawn once.
  for (int iy = 0; iy < image.height; ++iy) {
    for (int ix = 0; ix < image.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= frame_.width() || py >= frame_.height()) continue;

      const size_t index =
          static_cast<size_t>(iy) * static_cast<size_t>(image.width) + static_cast<size_t>(ix);
      const int alpha = image.alpha[index];
      if (alpha == 0) continue;
      const gauge::Color src = gauge::rgb565ToColor(image.rgb565[index]);
      if (alpha == 255) {
        frame_.drawPixel(px, py, rgb(src));
        continue;
      }
      frame_.drawPixel(px, py, rgb(gauge::blend(readColor(px, py), src, alpha)));
    }
  }
}

void LgfxCanvas::drawMask(const gauge::MaskView& mask, float xPos, float yPos, gauge::Color color) {
  const int x = static_cast<int>(std::lround(xPos));
  const int y = static_cast<int>(std::lround(yPos));
  if (mask.alpha == nullptr) return;

  for (int iy = 0; iy < mask.height; ++iy) {
    for (int ix = 0; ix < mask.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= frame_.width() || py >= frame_.height()) continue;

      const int alpha = mask.alpha[static_cast<size_t>(iy) * static_cast<size_t>(mask.width) +
                                   static_cast<size_t>(ix)];
      if (alpha == 0) continue;
      frame_.drawPixel(px, py,
                       rgb(alpha == 255 ? color : gauge::blend(readColor(px, py), color, alpha)));
    }
  }
}

void LgfxCanvas::saveBackground() {
  if (!dialAllocated_) return;
  frame_.pushSprite(&dial_, 0, 0);
  hasDial_ = true;
}

bool LgfxCanvas::restoreBackground() {
  if (!hasDial_) return false;
  dial_.pushSprite(&frame_, 0, 0);
  return true;
}

void LgfxCanvas::drawText(int x, int y, const char* text, int scale, gauge::TextAlign align,
                          gauge::Color color) {
  // Font0 is the 6x8 cell (5x7 glyph) font that SoftCanvas mimics.
  frame_.setFont(&fonts::Font0);
  frame_.setTextSize(scale);
  frame_.setTextColor(rgb(color));
  switch (align) {
    case gauge::TextAlign::Left:
      frame_.setTextDatum(lgfx::top_left);
      break;
    case gauge::TextAlign::Center:
      frame_.setTextDatum(lgfx::top_center);
      break;
    case gauge::TextAlign::Right:
      frame_.setTextDatum(lgfx::top_right);
      break;
  }
  frame_.drawString(text, x, y);
}

void LgfxCanvas::present() { frame_.pushSprite(panel_, 0, 0); }

}  // namespace device
