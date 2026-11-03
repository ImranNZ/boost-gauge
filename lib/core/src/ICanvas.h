#pragma once

#include <cstdint>

namespace gauge {

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

enum class TextAlign { Left, Center, Right };

/// Minimal 2D drawing surface. Implemented by the LCD driver on the device and
/// by a software rasteriser on the PC, so the gauge looks the same in both.
///
/// Angles are degrees, 0 = 3 o'clock, increasing clockwise (screen space).
class ICanvas {
 public:
  virtual ~ICanvas() = default;

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual void clear(Color color) = 0;
  virtual void fillCircle(int cx, int cy, int radius, Color color) = 0;
  /// Ring sector from startDeg clockwise to endDeg.
  virtual void fillArc(int cx, int cy, int innerRadius, int outerRadius,
                       float startDeg, float endDeg, Color color) = 0;
  virtual void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
                            Color color) = 0;
  virtual void drawLine(int x0, int y0, int x1, int y1, int thickness,
                        Color color) = 0;
  /// Built-in font: 6x8 cell (5x7 glyph) times an integer scale. `y` is the
  /// top of the text; `x` is the left/centre/right per `align`.
  virtual void drawText(int x, int y, const char* text, int scale, TextAlign align,
                        Color color) = 0;

  /// Push the finished frame to the display (no-op for offscreen canvases).
  virtual void present() = 0;
};

}  // namespace gauge
