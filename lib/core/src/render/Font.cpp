#include "render/Font.h"

#include <algorithm>
#include <cmath>

namespace gauge {

Font::Font(const Glyph* glyphs, int count, const uint8_t* alpha, int ascent) : count_(1) {
  faces_[0] = FontFace{glyphs, count, alpha, ascent, 1.0f};
}

Font::Font(const FontFace* faces, int count) : count_(std::min(count, kMaxFaces)) {
  for (int i = 0; i < count_; ++i) faces_[static_cast<size_t>(i)] = faces[i];
}

int Font::ascent() const {
  return static_cast<int>(std::lround(static_cast<float>(faces_[0].ascent) / faces_[0].resolution));
}

const FontFace& Font::faceFor(float scale) const {
  for (int i = 0; i < count_; ++i) {
    // A hair of slack, so a face made for exactly this size is found despite float rounding.
    if (faces_[static_cast<size_t>(i)].resolution >= scale - 0.001f) {
      return faces_[static_cast<size_t>(i)];
    }
  }
  return faces_[static_cast<size_t>(count_ - 1)];
}

const Glyph* Font::find(const FontFace& face, char ch) {
  for (int i = 0; i < face.count; ++i) {
    if (face.glyphs[i].ch == ch) return &face.glyphs[i];
  }
  return nullptr;
}

float Font::widthIn(const FontFace& face, const char* text, float tracking) {
  float total = 0.0f;
  int shown = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    const Glyph* glyph = find(face, *p);
    if (glyph == nullptr) continue;
    total += static_cast<float>(glyph->advance);
    ++shown;
  }
  return shown > 0 ? total + tracking * static_cast<float>(shown - 1) : 0.0f;
}

int Font::width(const char* text, int tracking) const {
  const FontFace& face = faces_[0];
  const float px = widthIn(face, text, static_cast<float>(tracking) * face.resolution);
  return static_cast<int>(std::lround(px / face.resolution));
}

void Font::draw(IDrawSurface& surface, int x, int baselineY, const char* text, TextAlign align,
                Color color, int tracking) const {
  const FontFace& face = faceFor(surface.scale());
  // Laid out in the face's own pixels, so the glyphs keep the spacing they were rendered with;
  // converted to design pixels only to say where each lands.
  const float perPixel = 1.0f / face.resolution;
  const float spacing = static_cast<float>(tracking) * face.resolution;
  const float total = widthIn(face, text, spacing);
  float pen = 0.0f;
  if (align == TextAlign::Center) pen = -total / 2.0f;
  if (align == TextAlign::Right) pen = -total;

  for (const char* p = text; *p != '\0'; ++p) {
    const Glyph* glyph = find(face, *p);
    if (glyph == nullptr) continue;
    // A space has an empty bitmap, which draws nothing.
    surface.drawMask(
        MaskView{glyph->width, glyph->height, face.alpha + glyph->offset, face.resolution},
        static_cast<float>(x) + (pen + static_cast<float>(glyph->xOffset)) * perPixel,
        static_cast<float>(baselineY) + static_cast<float>(glyph->yOffset) * perPixel, color);
    pen += static_cast<float>(glyph->advance) + spacing;
  }
}

}  // namespace gauge
