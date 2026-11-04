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
