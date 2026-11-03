#include "SoftCanvas.h"

#include <algorithm>
#include <cmath>
#include <cstring>

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
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
};

const Glyph* findGlyph(char c) {
  for (const Glyph& g : kGlyphs)
    if (g.ch == c) return &g;
  return nullptr;
}

float distanceToSegment(float px, float py, float x0, float y0, float x1, float y1) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float len2 = dx * dx + dy * dy;
  float t = len2 > 0.0f ? ((px - x0) * dx + (py - y0) * dy) / len2 : 0.0f;
  t = std::clamp(t, 0.0f, 1.0f);
  return std::hypot(px - (x0 + t * dx), py - (y0 + t * dy));
}

float edge(float ax, float ay, float bx, float by, float px, float py) {
  return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

}  // namespace

void SoftCanvas::fillCircle(int cx, int cy, int radius, Color color) {
  for (int y = cy - radius; y <= cy + radius; ++y)
    for (int x = cx - radius; x <= cx + radius; ++x)
      if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius)
        image_.set(x, y, color);
}

void SoftCanvas::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                         float endDeg, Color color) {
  float sweep = std::fmod(endDeg - startDeg, 360.0f);
  if (sweep < 0.0f) sweep += 360.0f;
  float start = std::fmod(startDeg, 360.0f);
  if (start < 0.0f) start += 360.0f;

  for (int y = cy - outerRadius; y <= cy + outerRadius; ++y) {
    for (int x = cx - outerRadius; x <= cx + outerRadius; ++x) {
      const int dx = x - cx;
      const int dy = y - cy;
      const int d2 = dx * dx + dy * dy;
      if (d2 < innerRadius * innerRadius || d2 > outerRadius * outerRadius) continue;

      float angle = std::atan2(static_cast<float>(dy), static_cast<float>(dx)) * kRadToDeg;
      if (angle < 0.0f) angle += 360.0f;
      float rel = angle - start;
      if (rel < 0.0f) rel += 360.0f;
      if (rel <= sweep) image_.set(x, y, color);
    }
  }
}

void SoftCanvas::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, Color color) {
  const int minX = std::min({x0, x1, x2});
  const int maxX = std::max({x0, x1, x2});
  const int minY = std::min({y0, y1, y2});
  const int maxY = std::max({y0, y1, y2});
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      const float px = static_cast<float>(x);
      const float py = static_cast<float>(y);
      const float e0 = edge(x0, y0, x1, y1, px, py);
      const float e1 = edge(x1, y1, x2, y2, px, py);
      const float e2 = edge(x2, y2, x0, y0, px, py);
      const bool allNonNeg = e0 >= 0 && e1 >= 0 && e2 >= 0;
      const bool allNonPos = e0 <= 0 && e1 <= 0 && e2 <= 0;
      if (allNonNeg || allNonPos) image_.set(x, y, color);
    }
  }
}

void SoftCanvas::drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) {
  const float radius = std::max(thickness, 1) * 0.5f;
  const int pad = static_cast<int>(std::ceil(radius)) + 1;
  for (int y = std::min(y0, y1) - pad; y <= std::max(y0, y1) + pad; ++y)
    for (int x = std::min(x0, x1) - pad; x <= std::max(x0, x1) + pad; ++x)
      if (distanceToSegment(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(x0), static_cast<float>(y0),
                            static_cast<float>(x1), static_cast<float>(y1)) <= radius)
        image_.set(x, y, color);
}

void SoftCanvas::drawText(int x, int y, const char* text, int scale, TextAlign align,
                          Color color) {
  const int length = static_cast<int>(std::strlen(text));
  const int width = length * 6 * scale;
  int left = x;
  if (align == TextAlign::Center) left = x - width / 2;
  if (align == TextAlign::Right) left = x - width;

  for (int i = 0; i < length; ++i) {
    const Glyph* glyph = findGlyph(text[i]);
    if (!glyph) continue;
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if (!(glyph->rows[row] & (0x10 >> col))) continue;
        for (int sy = 0; sy < scale; ++sy)
          for (int sx = 0; sx < scale; ++sx)
            image_.set(left + i * 6 * scale + col * scale + sx, y + row * scale + sy, color);
      }
    }
  }
}

}  // namespace gauge
