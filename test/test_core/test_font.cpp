// Font: laying out and drawing pre-rendered glyphs.

#include "test_support.h"

using namespace support;

namespace {

// Glyphs are 6 wide, 10 tall (sitting on the baseline), 1 px in from the pen, advancing 8.
const Font& label() {
  static const Font font = kLabelFont.font();
  return font;
}

}  // namespace

TEST_CASE("font: ascent is the cap height the font was built with") {
  CHECK(label().ascent() == 10);
}

TEST_CASE("font: width adds the advances, plus tracking between characters") {
  CHECK(label().width("") == 0);
  CHECK(label().width("A") == 8);
  CHECK(label().width("AB") == 16);
  CHECK(label().width("AB", 3) == 19);  // tracking goes between, not after
  CHECK(label().width("A B") == 24);    // a space advances like any other character
}

TEST_CASE("font: characters the font lacks take no space and draw nothing") {
  CHECK(label().width("a~") == 0);
  CHECK(label().width("A~B") == 16);

  SoftCanvas canvas(60, 30);
  label().draw(canvas, 0, 20, "a~", TextAlign::Left, kWhite);
  CHECK(countNonBlack(canvas.image()) == 0);
}

TEST_CASE("font: draws each glyph from its offset, on the baseline") {
  SoftCanvas canvas(60, 30);
  label().draw(canvas, 10, 20, "AB", TextAlign::Left, kWhite);

  // 'A' covers x 11..16, y 10..19; 'B' starts one advance (8) later.
  CHECK(same(canvas.image().get(11, 10), kWhite));
  CHECK(same(canvas.image().get(16, 19), kWhite));
  CHECK(same(canvas.image().get(10, 10), kBlack));  // left of the glyph
  CHECK(same(canvas.image().get(17, 15), kBlack));  // the gap between glyphs
  CHECK(same(canvas.image().get(19, 15), kWhite));  // 'B'
  CHECK(same(canvas.image().get(11, 20), kBlack));  // nothing below the baseline
  CHECK(countNonBlack(canvas.image()) == 2 * 6 * 10);
}

TEST_CASE("font: tracking spaces the glyphs out") {
  SoftCanvas canvas(60, 30);
  label().draw(canvas, 10, 20, "AB", TextAlign::Left, kWhite, 4);
  CHECK(same(canvas.image().get(23, 15), kWhite));  // 'B' now starts at 10 + 8 + 4 + 1
  CHECK(same(canvas.image().get(19, 15), kBlack));
}

TEST_CASE("font: aligns to the left, centre or right edge") {
  SoftCanvas left(60, 30);
  label().draw(left, 30, 20, "AB", TextAlign::Left, kWhite);
  CHECK(same(left.image().get(31, 15), kWhite));

  SoftCanvas centre(60, 30);
  label().draw(centre, 30, 20, "AB", TextAlign::Center, kWhite);
  CHECK(same(centre.image().get(23, 15), kWhite));  // starts 8 px (half the width) left

  SoftCanvas right(60, 30);
  label().draw(right, 30, 20, "AB", TextAlign::Right, kWhite);
  CHECK(same(right.image().get(15, 15), kWhite));  // starts 16 px left
  CHECK(same(right.image().get(31, 15), kBlack));
}

TEST_CASE("font: a space moves the pen but draws nothing") {
  SoftCanvas canvas(60, 30);
  label().draw(canvas, 0, 20, "A B", TextAlign::Left, kWhite);
  CHECK(countNonBlack(canvas.image()) == 2 * 6 * 10);
  CHECK(same(canvas.image().get(17, 15), kWhite));  // 'B' after two advances (16) + 1
}

namespace {

// Two sizes of the same boxes: 6x10 for the design size, and 12x20 for a screen twice as big.
const SolidFont kSmallFace(6, 10, 8);
const SolidFont kBigFace(12, 20, 16);
const SolidFont kHugeFace(24, 40, 32);

Font twoFaces() {
  const FontFace faces[] = {kSmallFace.face(1.0f), kBigFace.face(2.0f)};
  return Font(faces, 2);
}

Font threeFaces() {
  const FontFace faces[] = {kSmallFace.face(1.0f), kBigFace.face(2.0f), kHugeFace.face(3.0f)};
  return Font(faces, 3);
}

/// Draw "A" through a ScaledSurface on a square canvas `side` pixels across: the surface's scale
/// is side / 240.
int inkedPixels(const Font& font, int side) {
  SoftCanvas canvas(side, side);
  ScaledSurface surface(canvas);
  font.draw(surface, 120, 120, "A", TextAlign::Left, kWhite);
  return countNonBlack(canvas.image());
}

}  // namespace

TEST_CASE("font: with several faces, the first sets the design size") {
  const Font font = twoFaces();
  CHECK(font.ascent() == 10);
  CHECK(font.width("AB") == 16);
}

TEST_CASE("font: draws with the smallest face that is at least as big as the screen needs") {
  const Font font = twoFaces();
  CHECK(inkedPixels(font, 240) == 6 * 10);   // 1x: the small face, as it is
  CHECK(inkedPixels(font, 480) == 12 * 20);  // 2x: the big face, as it is (not stretched)
  CHECK(inkedPixels(font, 360) == 9 * 15);   // 1.5x: the big face, shrunk to size
}

TEST_CASE("font: on a screen bigger than any face, the biggest is stretched") {
  CHECK(inkedPixels(twoFaces(), 720) == 18 * 30);  // 3x from the 2x face
}

TEST_CASE("font: a face generated for a 3x screen is used at 3x") {
  CHECK(inkedPixels(threeFaces(), 720) == 24 * 40);  // the 3x face, as it is
}

TEST_CASE("font: a profile covers a screen up to its largest face") {
  CHECK(fontCoversScale(1.9417f, 1.0f));
  CHECK(fontCoversScale(1.9417f, 1.9417f));
  CHECK(fontCoversScale(1.9417f, 1.9417f + 0.0005f));  // the same float-rounding slack
  CHECK_FALSE(fontCoversScale(1.9417f, 3.0f));         // what a 720 screen needs
  CHECK(fontCoversScale(3.0f, 3.0f));
}

TEST_CASE("font: a face of the exact size is used unstretched, spacing included") {
  const Font font = twoFaces();
  SoftCanvas canvas(480, 480);
  ScaledSurface surface(canvas);
  font.draw(surface, 100, 100, "AB", TextAlign::Left, kWhite);
  // Anchored at real pixel (200, 200). The 12x20 glyphs sit 1 px in from the pen and rise 20 px
  // above the baseline; 'B' starts one 16 px advance after 'A'.
  CHECK(same(canvas.image().get(201, 190), kWhite));
  CHECK(same(canvas.image().get(212, 190), kWhite));
  CHECK(same(canvas.image().get(213, 190), kBlack));  // the gap between them
  CHECK(same(canvas.image().get(216, 190), kBlack));
  CHECK(same(canvas.image().get(217, 190), kWhite));  // 'B'
  CHECK(countNonBlack(canvas.image()) == 2 * 12 * 20);
}

TEST_CASE("font: faces beyond the limit are ignored") {
  FontFace faces[Font::kMaxFaces + 2];
  for (FontFace& face : faces) face = kSmallFace.face(1.0f);
  const Font font(faces, Font::kMaxFaces + 2);
  SoftCanvas canvas(60, 30);
  font.draw(canvas, 10, 20, "A", TextAlign::Left, kWhite);
  CHECK(countNonBlack(canvas.image()) == 6 * 10);
}
