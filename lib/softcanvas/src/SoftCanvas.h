#pragma once

#include "ICanvas.h"
#include "Image.h"

namespace gauge {

/// Software rasteriser implementing ICanvas into an in-memory Image. Lets the
/// real GaugeView run (and be tested) on a PC. Font matches the device's
/// built-in 6x8 font, but only covers: 0-9 - + . space A B E I K L N O P S T
class SoftCanvas : public ICanvas {
 public:
  SoftCanvas(int width, int height) : image_(width, height) {}

  int width() const override { return image_.width(); }
  int height() const override { return image_.height(); }

  void clear(Color color) override { image_.fill(color); }
  void fillCircle(int cx, int cy, int radius, Color color) override;
  void fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
               float endDeg, Color color) override;
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, Color color) override;
  void drawLine(int x0, int y0, int x1, int y1, int thickness, Color color) override;
  void drawText(int x, int y, const char* text, int scale, TextAlign align,
                Color color) override;
  void present() override { ++framesPresented_; }

  const Image& image() const { return image_; }
  int framesPresented() const { return framesPresented_; }

 private:
  Image image_;
  int framesPresented_ = 0;
};

}  // namespace gauge
