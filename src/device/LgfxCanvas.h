#pragma once

#include <LovyanGFX.hpp>

#include "ICanvas.h"

namespace device {

/// ICanvas over the GC9A01 panel via LovyanGFX. Draws into an off-screen
/// 16-bit sprite and pushes it in one go, so frames never tear or flicker.
class LgfxCanvas : public gauge::ICanvas {
 public:
  LgfxCanvas();

  /// Initialise the panel and allocate the frame buffer. False if out of memory.
  bool begin();

  int width() const override;
  int height() const override;

  void clear(gauge::Color color) override;
  void fillCircle(int cx, int cy, int radius, gauge::Color color) override;
  void fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
               float endDeg, gauge::Color color) override;
  void drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                 gauge::Color color) override;
  void drawLine(int x0, int y0, int x1, int y1, int thickness,
                gauge::Color color) override;
  void drawText(int x, int y, const char* text, int scale, gauge::TextAlign align,
                gauge::Color color) override;
  void drawImage(const gauge::ImageView& image, int x, int y) override;
  void saveBackground() override;
  bool restoreBackground() override;
  void discardBackground() override { hasDial_ = false; }
  void present() override;

 private:
  class Panel;  // LovyanGFX device config for this board (defined in the .cpp)

  uint16_t rgb(gauge::Color c) { return frame_.color565(c.r, c.g, c.b); }
  gauge::Color readColor(int x, int y) {
    const auto p = frame_.readPixelRGB(x, y);
    return gauge::Color{p.R8(), p.G8(), p.B8()};
  }

  Panel* panel_;
  lgfx::LGFX_Sprite frame_;  // what we draw into; internal RAM for speed
  lgfx::LGFX_Sprite dial_;   // cached static dial; PSRAM (115 KB we cannot spare in DRAM)
  bool dialAllocated_ = false;
  bool hasDial_ = false;     // dial_ holds a saved frame
};

}  // namespace device
