#include "Font.h"

namespace gauge {

const Glyph* Font::find(char ch) const {
  for (int i = 0; i < count_; ++i) {
    if (glyphs_[i].ch == ch) return &glyphs_[i];
  }
  return nullptr;
}

int Font::width(const char* text, int tracking) const {
  int total = 0;
  int shown = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    const Glyph* glyph = find(*p);
    if (glyph == nullptr) continue;
    total += glyph->advance;
    ++shown;
  }
  return shown > 0 ? total + tracking * (shown - 1) : 0;
}

void Font::draw(IDrawSurface& surface, int x, int baselineY, const char* text, TextAlign align,
                Color color, int tracking) const {
  const int total = width(text, tracking);
  int pen = x;
  if (align == TextAlign::Center) pen -= total / 2;
  if (align == TextAlign::Right) pen -= total;

  for (const char* p = text; *p != '\0'; ++p) {
    const Glyph* glyph = find(*p);
    if (glyph == nullptr) continue;
    // A space has an empty bitmap, which draws nothing.
    surface.drawMask(MaskView{glyph->width, glyph->height, alpha_ + glyph->offset},
                     pen + glyph->xOffset, baselineY + glyph->yOffset, color);
    pen += glyph->advance + tracking;
  }
}

}  // namespace gauge
