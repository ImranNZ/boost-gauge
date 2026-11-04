#include "SoftCanvas.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "render/ArcCoverage.h"

namespace gauge {
namespace {

constexpr float kRadToDeg = 180.0f / 3.14159265f;

struct Glyph {
  char ch;
  uint8_t rows[7];  // 5 bits per row, MSB = leftmost column
};

constexpr Glyph kGlyphs[] = {
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
};

const Glyph* findGlyph(char c) {
  for (const Glyph& g : kGlyphs)
    if (g.ch == c) return &g;
  return nullptr;
}

/// Distance from (px, py) to the segment, and where along it (0..1) the nearest point is.
float distanceToSegment(float px, float py, float x0, float y0, float x1, float y1, float& t) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float len2 = dx * dx + dy * dy;
  t = len2 > 0.0f ? ((px - x0) * dx + (py - y0) * dy) / len2 : 0.0f;
  t = std::clamp(t, 0.0f, 1.0f);
  return std::hypot(px - (x0 + t * dx), py - (y0 + t * dy));
}

/// Fill every pixel within a radius (linearly r0 -> r1 along the segment) of it.
void strokeSegment(Image& img, float x0, float y0, float x1, float y1, float r0, float r1,
                   Color color) {
  const float pad = std::max(r0, r1) + 1.0f;
  const int minX = static_cast<int>(std::floor(std::min(x0, x1) - pad));
  const int maxX = static_cast<int>(std::ceil(std::max(x0, x1) + pad));
  const int minY = static_cast<int>(std::floor(std::min(y0, y1) - pad));
  const int maxY = static_cast<int>(std::ceil(std::max(y0, y1) + pad));
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      float t = 0.0f;
      const float d =
          distanceToSegment(static_cast<float>(x), static_cast<float>(y), x0, y0, x1, y1, t);
      if (d <= r0 + (r1 - r0) * t) img.set(x, y, color);
    }
  }
}

}  // namespace

SoftCanvas::SoftCanvas(int width, int height, int supersample)
    : width_(width),
      height_(height),
      k_(std::max(supersample, 1)),
      hi_(width * k_, height * k_),
      out_(width, height) {}

void SoftCanvas::fillCircle(int cx, int cy, int radius, Color color) {
  const float hx = centre(cx);
  const float hy = centre(cy);
  const float r = static_cast<float>(radius * k_);
  const int x0 = static_cast<int>(std::floor(hx - r));
  const int x1 = static_cast<int>(std::ceil(hx + r));
  const int y0 = static_cast<int>(std::floor(hy - r));
  const int y1 = static_cast<int>(std::ceil(hy + r));
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const float dx = static_cast<float>(x) - hx;
      const float dy = static_cast<float>(y) - hy;
      if (dx * dx + dy * dy <= r * r) hi_.set(x, y, color);
    }
  }
}

void SoftCanvas::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                         float endDeg, Color color) {
  const float sweep = wrapDegrees(endDeg - startDeg);
  const float start = wrapDegrees(startDeg);
  const float hx = centre(cx);
  const float hy = centre(cy);
  const float inner = static_cast<float>(innerRadius * k_);
  const float outer = static_cast<float>(outerRadius * k_);

  const int x0 = static_cast<int>(std::floor(hx - outer));
  const int x1 = static_cast<int>(std::ceil(hx + outer));
  const int y0 = static_cast<int>(std::floor(hy - outer));
  const int y1 = static_cast<int>(std::ceil(hy + outer));
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const float dx = static_cast<float>(x) - hx;
      const float dy = static_cast<float>(y) - hy;
      const float d2 = dx * dx + dy * dy;
      if (d2 < inner * inner || d2 > outer * outer) continue;
      const float angle = std::atan2(dy, dx) * kRadToDeg;
      if (wrapDegrees(angle - start) <= sweep) hi_.set(x, y, color);
    }
  }
}

void SoftCanvas::drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                           Color color) {
  strokeSegment(hi_, centreF(x0), centreF(y0), centreF(x1), centreF(y1),
                r0 * static_cast<float>(k_), r1 * static_cast<float>(k_), color);
}

void SoftCanvas::drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) {
  const float radius = std::max(thickness, 1) * 0.5f * static_cast<float>(k_);
  strokeSegment(hi_, centre(x0), centre(y0), centre(x1), centre(y1), radius, radius, color);
}

void SoftCanvas::drawText(int x, int y, const char* text, int scale, TextAlign align, Color color) {
  const int length = static_cast<int>(std::strlen(text));
  const int width = length * 6 * scale;
  int left = x;
  if (align == TextAlign::Center) left = x - width / 2;
  if (align == TextAlign::Right) left = x - width;

  const int block = scale * k_;  // one font pixel, in hi-res pixels
  for (int i = 0; i < length; ++i) {
    const Glyph* glyph = findGlyph(text[i]);
    if (!glyph) continue;
    const int glyphX = (left + i * 6 * scale) * k_;
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if (!(glyph->rows[row] & (0x10 >> col))) continue;
        for (int sy = 0; sy < block; ++sy)
          for (int sx = 0; sx < block; ++sx)
            hi_.set(glyphX + col * block + sx, y * k_ + row * block + sy, color);
      }
    }
  }
}

void SoftCanvas::drawImage(const ImageView& image, int x, int y) {
  if (image.rgb565 == nullptr) return;

  for (int iy = 0; iy < image.height; ++iy) {
    for (int ix = 0; ix < image.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= width_ || py >= height_) continue;  // clipped

      const size_t index =
          static_cast<size_t>(iy) * static_cast<size_t>(image.width) + static_cast<size_t>(ix);
      const int alpha = image.alpha != nullptr ? image.alpha[index] : 255;
      if (alpha == 0) continue;
      const Color src = rgb565ToColor(image.rgb565[index]);

      // Each source pixel covers a k x k block at the supersampled resolution.
      for (int sy = 0; sy < k_; ++sy) {
        for (int sx = 0; sx < k_; ++sx) {
          const int hx = px * k_ + sx;
          const int hy = py * k_ + sy;
          if (alpha == 255) {
            hi_.set(hx, hy, src);
            continue;
          }
          hi_.set(hx, hy, blend(hi_.get(hx, hy), src, alpha));
        }
      }
    }
  }
}

void SoftCanvas::drawMask(const MaskView& mask, int x, int y, Color color) {
  if (mask.alpha == nullptr) return;

  for (int iy = 0; iy < mask.height; ++iy) {
    for (int ix = 0; ix < mask.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= width_ || py >= height_) continue;  // clipped

      const int alpha = mask.alpha[static_cast<size_t>(iy) * static_cast<size_t>(mask.width) +
                                   static_cast<size_t>(ix)];
      if (alpha == 0) continue;
      for (int sy = 0; sy < k_; ++sy) {
        for (int sx = 0; sx < k_; ++sx) {
          const int hx = px * k_ + sx;
          const int hy = py * k_ + sy;
          hi_.set(hx, hy, alpha == 255 ? color : blend(hi_.get(hx, hy), color, alpha));
        }
      }
    }
  }
}

bool SoftCanvas::restoreBackground() {
  if (!background_) return false;
  hi_ = *background_;
  return true;
}

const Image& SoftCanvas::image() const {
  if (k_ == 1) return hi_;

  const int area = k_ * k_;
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      int r = 0, g = 0, b = 0;
      for (int sy = 0; sy < k_; ++sy) {
        for (int sx = 0; sx < k_; ++sx) {
          const Color c = hi_.get(x * k_ + sx, y * k_ + sy);
          r += c.r;
          g += c.g;
          b += c.b;
        }
      }
      out_.set(x, y,
               Color{static_cast<uint8_t>((r + area / 2) / area),
                     static_cast<uint8_t>((g + area / 2) / area),
                     static_cast<uint8_t>((b + area / 2) / area)});
    }
  }
  return out_;
}

}  // namespace gauge
