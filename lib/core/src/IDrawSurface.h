#pragma once

#include "Color.h"
#include "ImageView.h"

namespace gauge {

enum class TextAlign { Left, Center, Right };

/// What a look may draw with: shapes, text and pictures. Nothing about caching
/// or showing the frame, so a look depends on exactly what it uses.
///
/// Shapes are anti-aliased where the backend can (text uses the built-in
/// bitmap font). Angles are degrees, 0 = 3 o'clock, increasing clockwise.
class IDrawSurface {
 public:
  virtual ~IDrawSurface() = default;

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual void clear(Color color) = 0;
  virtual void fillCircle(int cx, int cy, int radius, Color color) = 0;
  /// Ring sector from startDeg clockwise to endDeg.
  virtual void fillArc(int cx, int cy, int innerRadius, int outerRadius,
                       float startDeg, float endDeg, Color color) = 0;
  /// Tapered stroke: radius r0 at (x0, y0) narrowing to r1 at (x1, y1). Float
  /// coordinates so a rotating needle moves smoothly. (A needle is a wedge.)
  virtual void drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                         Color color) = 0;
  virtual void drawLine(int x0, int y0, int x1, int y1, int thickness,
                        Color color) = 0;
  /// Built-in font: 6x8 cell (5x7 glyph) times an integer scale. `y` is the
  /// top of the text; `x` is the left/centre/right per `align`.
  virtual void drawText(int x, int y, const char* text, int scale, TextAlign align,
                        Color color) = 0;
  /// Copy a picture with its top-left at (x, y), clipped to the surface. Pixels
  /// with partial alpha blend over what is already there.
  virtual void drawImage(const ImageView& image, int x, int y) = 0;
};

}  // namespace gauge
