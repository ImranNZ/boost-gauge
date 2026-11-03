#pragma once

#include <optional>

#include "ICanvas.h"
#include "Image.h"

namespace gauge {

/// Software rasteriser implementing ICanvas into an in-memory Image, so the
/// real GaugeView runs (and is tested) on a PC.
///
/// `supersample` > 1 draws at N times the resolution and box-filters down, which
/// anti-aliases every shape. With 1 (the default) edges are hard and pixel-exact,
/// which keeps unit tests simple.
///
/// Font matches the device's built-in 6x8 font, but only covers:
/// 0-9 - + . space A B E F I K L N O P S T
class SoftCanvas : public ICanvas {
 public:
  SoftCanvas(int width, int height, int supersample = 1);

  int width() const override { return width_; }
  int height() const override { return height_; }

  void clear(Color color) override { hi_.fill(color); }
  void fillCircle(int cx, int cy, int radius, Color color) override;
  void fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
               float endDeg, Color color) override;
  void drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                 Color color) override;
  void drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) override;
  void drawText(int x, int y, const char* text, int scale, TextAlign align,
                Color color) override;
  void drawImage(const ImageView& image, int x, int y) override;
  void drawMask(const MaskView& mask, int x, int y, Color color) override;
  void saveBackground() override { background_ = hi_; }
  bool restoreBackground() override;
  void discardBackground() override { background_.reset(); }
  void present() override { ++framesPresented_; }

  /// The frame at logical size (downsampled when supersampling).
  const Image& image() const;
  int framesPresented() const { return framesPresented_; }

 private:
  // Logical pixel p spans hi-res [p*k, p*k + k); its centre is p*k + (k-1)/2.
  float centre(int logical) const { return static_cast<float>(logical * k_) + (k_ - 1) * 0.5f; }
  float centreF(float logical) const { return logical * static_cast<float>(k_) + (k_ - 1) * 0.5f; }

  int width_;
  int height_;
  int k_;
  Image hi_;
  std::optional<Image> background_;
  mutable Image out_;
  int framesPresented_ = 0;
};

}  // namespace gauge
