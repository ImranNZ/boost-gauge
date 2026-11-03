#pragma once

#include <cstdint>

#include "Color.h"
#include "IDrawSurface.h"

namespace gauge {

/// One character of a pre-rendered font: an 8-bit coverage bitmap and where it sits.
struct Glyph {
  char ch;
  uint8_t width;
  uint8_t height;
  int8_t xOffset;   // pixels from the pen position to the bitmap's left edge
  int8_t yOffset;   // pixels from the baseline to the bitmap's top edge (negative = above)
  uint8_t advance;  // pixels the pen moves on after this glyph
  uint32_t offset;  // where its width * height coverage bytes start in the font's array
};

/// A font whose glyphs were rendered ahead of time from a TrueType file (see
/// tools/make_font.py). The tables live in flash and are never copied; this just
/// reads them and draws through IDrawSurface::drawMask, so it works on any canvas.
class Font {
 public:
  Font(const Glyph* glyphs, int count, const uint8_t* alpha, int ascent)
      : glyphs_(glyphs), count_(count), alpha_(alpha), ascent_(ascent) {}

  /// Height of a capital or digit above the baseline, to centre text vertically.
  int ascent() const { return ascent_; }

  /// Width of `text` in pixels, with `tracking` extra pixels between characters.
  /// Characters the font does not have take no space.
  int width(const char* text, int tracking = 0) const;

  /// Draw `text` with its baseline at `baselineY`. `x` is the left, centre or right
  /// edge per `align`. Characters the font does not have are skipped.
  void draw(IDrawSurface& surface, int x, int baselineY, const char* text, TextAlign align,
            Color color, int tracking = 0) const;

 private:
  const Glyph* find(char ch) const;

  const Glyph* glyphs_;
  int count_;
  const uint8_t* alpha_;
  int ascent_;
};

}  // namespace gauge
