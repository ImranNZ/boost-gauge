#pragma once

#include <array>
#include <cstdint>

#include "render/IDrawSurface.h"

namespace gauge {

/// Lets a look drawn for a 240x240 screen run on any size.
///
/// The looks draw in a fixed "design" space of kDesignSize pixels (a circle of radius 120 around
/// (120, 120)). This surface sits between a look and the real surface and maps that space onto the
/// real one: scaled to fit the shorter side and centred on the longer. Shapes keep their
/// proportions and stay sharp at any size. Text and pictures made for this scale (see Font and
/// Picture) pass through as they are; other bitmaps are resampled bilinearly. At 240x240 the
/// original design-resolution assets pass straight through.
///
/// It owns a small scratch buffer for resampling, so make one per display, not per frame.
class ScaledSurface : public IDrawSurface {
 public:
  /// The size the looks are drawn for.
  static constexpr int kDesignSize = 240;

  /// Real pixels per design pixel for a `width` x `height` surface: fitted to the shorter side.
  static constexpr float scaleFor(int width, int height) {
    return static_cast<float>(width < height ? width : height) / static_cast<float>(kDesignSize);
  }

  explicit ScaledSurface(IDrawSurface& target);

  /// Always kDesignSize: what a look sees.
  int width() const override { return kDesignSize; }
  int height() const override { return kDesignSize; }

  /// Real pixels per design pixel.
  float scale() const override { return scale_; }

  void clear(Color color) override;
  void fillCircle(int cx, int cy, int radius, Color color) override;
  void fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg, float endDeg,
               Color color) override;
  void drawWedge(float x0, float y0, float x1, float y1, float r0, float r1, Color color) override;
  void drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) override;
  void drawText(int x, int y, const char* text, int scale, TextAlign align, Color color) override;
  void drawImage(const ImageView& image, int x, int y) override;
  void drawMask(const MaskView& mask, float x, float y, Color color) override;

 private:
  /// Pixels of a resampled picture handed to the target at a time.
  static constexpr int kBandPixels = 4096;
  /// How close a bitmap's resolution must be to the scale to be drawn without resampling.
  static constexpr float kSameResolution = 0.001f;

  /// Where a picture of `width` x `height` design pixels lands, in real pixels. The extents may be
  /// fractional (a bitmap rendered at twice the resolution is half as many design pixels).
  struct Placement {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
  };
  /// Fill `out`; false if it is too wide for one band to hold (it is then not drawn).
  bool place(float x, float y, float width, float height, Placement& out) const;
  /// Resample a source picture into the bands: `fill(index, u, v)` computes one band pixel from
  /// the two taps, `emit(width, rows, left, top)` hands a finished band to the target.
  template <typename Fill, typename Emit>
  void resample(const Placement& at, int srcWidth, int srcHeight, Fill fill, Emit emit);

  int mapX(float x) const;
  int mapY(float y) const;
  int mapLength(int length) const;
  /// A length that must not vanish (a line's thickness, text scale): at least 1.
  int mapSize(int size) const;

  IDrawSurface& target_;
  float scale_;
  float originX_;
  float originY_;
  std::array<uint16_t, kBandPixels> rgbBand_{};
  std::array<uint8_t, kBandPixels> alphaBand_{};
};

}  // namespace gauge
