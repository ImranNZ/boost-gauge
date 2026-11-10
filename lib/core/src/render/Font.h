#pragma once

#include <array>
#include <cstdint>

#include "render/Color.h"
#include "render/IDrawSurface.h"

namespace gauge {

/// One character of a pre-rendered font: an 8-bit coverage bitmap and where it sits. All sizes
/// are in the pixels of the face the glyph belongs to.
struct Glyph {
  char ch;
  uint8_t width;
  uint8_t height;
  int8_t xOffset;   // pixels from the pen position to the bitmap's left edge
  int16_t yOffset;  // pixels from the baseline to the bitmap's top edge (negative = above)
  uint8_t advance;  // pixels the pen moves on after this glyph
  uint32_t offset;  // where its width * height coverage bytes start in the face's array
};

/// The glyphs of one font rendered at one size. `resolution` is how many of its pixels make one
/// design-space pixel: 1 for the size the looks are designed at (240x240), about 1.94 for a
/// 466-pixel screen, and so on. `ascent` is in this face's pixels.
struct FontFace {
  const Glyph* glyphs = nullptr;
  int count = 0;
  const uint8_t* alpha = nullptr;
  int ascent = 0;
  float resolution = 1.0f;
};

/// Whether a font profile whose largest face is `maxResolution` covers `surfaceScale` without
/// upscaling. Font picks the smallest face at least the scale, so a bigger face is only shrunk;
/// only a screen beyond the largest face is stretched. The slack matches Font::faceFor, so a face
/// generated for exactly this scale is not missed by float rounding. The firmware asserts this for
/// its board (see src/main.cpp); a font profile is generated per display class (tools/fonts.json).
constexpr bool fontCoversScale(float maxResolution, float surfaceScale) {
  return maxResolution + 0.001f >= surfaceScale;
}

/// A font whose glyphs were rendered ahead of time from a TrueType file (see
/// tools/make_font.py), at one or more sizes. The tables live in flash and are never copied; this
/// just reads them and draws through IDrawSurface::drawMask, so it works on any canvas.
///
/// Text is laid out in design pixels, but drawn with the face closest to the size it will appear
/// at (IDrawSurface::scale()), so it stays sharp on a bigger screen instead of being stretched.
class Font {
 public:
  static constexpr int kMaxFaces = 6;

  /// A font with one face, at the design resolution.
  Font(const Glyph* glyphs, int count, const uint8_t* alpha, int ascent);

  /// A font with several faces, in order of increasing resolution (the first is the design size).
  /// Faces beyond kMaxFaces are ignored.
  Font(const FontFace* faces, int count);

  /// Height of a capital or digit above the baseline, in design pixels, to centre text
  /// vertically.
  int ascent() const;

  /// Width of `text` in design pixels, with `tracking` extra pixels between characters.
  /// Characters the font does not have take no space.
  int width(const char* text, int tracking = 0) const;

  /// Draw `text` with its baseline at `baselineY`. `x` is the left, centre or right
  /// edge per `align`. Characters the font does not have are skipped.
  void draw(IDrawSurface& surface, int x, int baselineY, const char* text, TextAlign align,
            Color color, int tracking = 0) const;

 private:
  /// The smallest face at least `scale` big (so it is only ever shrunk, which stays sharper than
  /// stretching), or the biggest there is.
  const FontFace& faceFor(float scale) const;
  static const Glyph* find(const FontFace& face, char ch);
  /// Width of `text` in `face`'s pixels, with `tracking` face pixels between characters.
  static float widthIn(const FontFace& face, const char* text, float tracking);

  std::array<FontFace, kMaxFaces> faces_{};
  int count_ = 0;
};

}  // namespace gauge
