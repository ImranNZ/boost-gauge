#pragma once

#include <cstdint>

#include "Color.h"
#include "ImageView.h"

namespace gauge {

enum class TextAlign { Left, Center, Right };

/// Minimal 2D drawing surface. Implemented by the LCD driver on the device and
/// by a software rasteriser on the PC, so the gauge looks the same in both.
///
/// Shapes are anti-aliased where the backend can (text uses the built-in
/// bitmap font). Angles are degrees, 0 = 3 o'clock, increasing clockwise.
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
  /// Copy a picture with its top-left at (x, y), clipped to the canvas. Pixels
  /// with partial alpha blend over what is already there.
  virtual void drawImage(const ImageView& image, int x, int y) = 0;

  /// Remember the current frame so static artwork (the dial) is drawn once.
  virtual void saveBackground() = 0;
  /// Replace the frame with the saved one. False if nothing is saved (or the
  /// backend cannot cache), in which case the caller must redraw it itself.
  virtual bool restoreBackground() = 0;
  /// Forget the saved frame (the artwork changed, e.g. a different look).
  virtual void discardBackground() = 0;

  /// Push the finished frame to the display (no-op for offscreen canvases).
  virtual void present() = 0;
};

}  // namespace gauge
