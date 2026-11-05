#include "render/ScaledSurface.h"

#include <algorithm>
#include <cmath>

namespace gauge {
namespace {

/// Where destination pixel `d` of `dstSize` samples a source of `srcSize`: the two source pixels
/// it sits between and how far (0-255) towards the second. Pixel centres line up, so the picture
/// is neither shifted nor shrunk at the edges.
struct Tap {
  int first = 0;
  int second = 0;
  int weight = 0;  // 0..255 towards `second`
};

Tap tapFor(int d, int dstSize, int srcSize) {
  const int64_t position =
      ((static_cast<int64_t>(d) * 2 + 1) * srcSize * 65536) / (static_cast<int64_t>(dstSize) * 2) -
      32768;
  const int64_t clamped = std::max<int64_t>(position, 0);
  Tap tap;
  tap.first = std::min(static_cast<int>(clamped >> 16), srcSize - 1);
  tap.second = std::min(tap.first + 1, srcSize - 1);
  tap.weight = static_cast<int>((clamped >> 8) & 0xFF);
  return tap;
}

int mix(int a, int b, int weight) { return (a * (256 - weight) + b * weight) >> 8; }

/// Bilinear blend of four 8-bit samples.
int bilinear(int s00, int s10, int s01, int s11, int wx, int wy) {
  return mix(mix(s00, s10, wx), mix(s01, s11, wx), wy);
}

uint16_t packRgb565(int r, int g, int b) {
  return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

}  // namespace

ScaledSurface::ScaledSurface(IDrawSurface& target) : target_(target) {
  const int w = target.width();
  const int h = target.height();
  scale_ = static_cast<float>(std::min(w, h)) / static_cast<float>(kDesignSize);
  originX_ = (static_cast<float>(w) - static_cast<float>(kDesignSize) * scale_) * 0.5f;
  originY_ = (static_cast<float>(h) - static_cast<float>(kDesignSize) * scale_) * 0.5f;
}

int ScaledSurface::mapX(int x) const {
  return static_cast<int>(std::lround(originX_ + static_cast<float>(x) * scale_));
}

int ScaledSurface::mapY(int y) const {
  return static_cast<int>(std::lround(originY_ + static_cast<float>(y) * scale_));
}

int ScaledSurface::mapLength(int length) const {
  return static_cast<int>(std::lround(static_cast<float>(length) * scale_));
}

int ScaledSurface::mapSize(int size) const { return std::max(1, mapLength(size)); }

void ScaledSurface::clear(Color color) { target_.clear(color); }

void ScaledSurface::fillCircle(int cx, int cy, int radius, Color color) {
  target_.fillCircle(mapX(cx), mapY(cy), mapLength(radius), color);
}

void ScaledSurface::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                            float endDeg, Color color) {
  target_.fillArc(mapX(cx), mapY(cy), mapLength(innerRadius), mapLength(outerRadius), startDeg,
                  endDeg, color);
}

void ScaledSurface::drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                              Color color) {
  // Float coordinates stay float: a needle moves by fractions of a pixel.
  target_.drawWedge(originX_ + x0 * scale_, originY_ + y0 * scale_, originX_ + x1 * scale_,
                    originY_ + y1 * scale_, r0 * scale_, r1 * scale_, color);
}

void ScaledSurface::drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) {
  target_.drawLine(mapX(x0), mapY(y0), mapX(x1), mapY(y1), mapSize(thickness), color);
}

void ScaledSurface::drawText(int x, int y, const char* text, int scale, TextAlign align,
                             Color color) {
  target_.drawText(mapX(x), mapY(y), text, mapSize(scale), align, color);
}

void ScaledSurface::drawImage(const ImageView& image, int x, int y) {
  if (image.rgb565 == nullptr) return;
  if (scale_ == 1.0f) {
    target_.drawImage(image, mapX(x), mapY(y));
    return;
  }

  // Size from the mapped edges, not the mapped size, so a glyph's bottom lands on the same
  // row as its neighbours' (the baseline) whatever the rounding.
  const int left = mapX(x);
  const int top = mapY(y);
  const int dw = std::max(1, mapX(x + image.width) - left);
  const int dh = std::max(1, mapY(y + image.height) - top);
  if (dw > kBandPixels) return;
  const int rowsPerBand = kBandPixels / dw;

  for (int row0 = 0; row0 < dh; row0 += rowsPerBand) {
    const int rows = std::min(rowsPerBand, dh - row0);
    for (int dy = 0; dy < rows; ++dy) {
      const Tap v = tapFor(row0 + dy, dh, image.height);
      for (int dx = 0; dx < dw; ++dx) {
        const Tap u = tapFor(dx, dw, image.width);
        const size_t i00 = static_cast<size_t>(v.first) * image.width + u.first;
        const size_t i10 = static_cast<size_t>(v.first) * image.width + u.second;
        const size_t i01 = static_cast<size_t>(v.second) * image.width + u.first;
        const size_t i11 = static_cast<size_t>(v.second) * image.width + u.second;
        const Color c00 = rgb565ToColor(image.rgb565[i00]);
        const Color c10 = rgb565ToColor(image.rgb565[i10]);
        const Color c01 = rgb565ToColor(image.rgb565[i01]);
        const Color c11 = rgb565ToColor(image.rgb565[i11]);
        const size_t out = static_cast<size_t>(dy) * dw + dx;
        rgbBand_[out] = packRgb565(bilinear(c00.r, c10.r, c01.r, c11.r, u.weight, v.weight),
                                   bilinear(c00.g, c10.g, c01.g, c11.g, u.weight, v.weight),
                                   bilinear(c00.b, c10.b, c01.b, c11.b, u.weight, v.weight));
        alphaBand_[out] = image.alpha == nullptr
                              ? 255
                              : static_cast<uint8_t>(bilinear(image.alpha[i00], image.alpha[i10],
                                                              image.alpha[i01], image.alpha[i11],
                                                              u.weight, v.weight));
      }
    }
    ImageView band;
    band.width = dw;
    band.height = rows;
    band.rgb565 = rgbBand_.data();
    band.alpha = image.alpha == nullptr ? nullptr : alphaBand_.data();
    target_.drawImage(band, left, top + row0);
  }
}

void ScaledSurface::drawMask(const MaskView& mask, int x, int y, Color color) {
  if (mask.alpha == nullptr) return;
  if (scale_ == 1.0f) {
    target_.drawMask(mask, mapX(x), mapY(y), color);
    return;
  }

  // Size from the mapped edges, not the mapped size, so a glyph's bottom lands on the same
  // row as its neighbours' (the baseline) whatever the rounding.
  const int left = mapX(x);
  const int top = mapY(y);
  const int dw = std::max(1, mapX(x + mask.width) - left);
  const int dh = std::max(1, mapY(y + mask.height) - top);
  if (dw > kBandPixels) return;
  const int rowsPerBand = kBandPixels / dw;

  for (int row0 = 0; row0 < dh; row0 += rowsPerBand) {
    const int rows = std::min(rowsPerBand, dh - row0);
    for (int dy = 0; dy < rows; ++dy) {
      const Tap v = tapFor(row0 + dy, dh, mask.height);
      for (int dx = 0; dx < dw; ++dx) {
        const Tap u = tapFor(dx, dw, mask.width);
        const size_t i00 = static_cast<size_t>(v.first) * mask.width + u.first;
        const size_t i10 = static_cast<size_t>(v.first) * mask.width + u.second;
        const size_t i01 = static_cast<size_t>(v.second) * mask.width + u.first;
        const size_t i11 = static_cast<size_t>(v.second) * mask.width + u.second;
        alphaBand_[static_cast<size_t>(dy) * dw + dx] =
            static_cast<uint8_t>(bilinear(mask.alpha[i00], mask.alpha[i10], mask.alpha[i01],
                                          mask.alpha[i11], u.weight, v.weight));
      }
    }
    MaskView band;
    band.width = dw;
    band.height = rows;
    band.alpha = alphaBand_.data();
    target_.drawMask(band, left, top + row0, color);
  }
}

}  // namespace gauge
