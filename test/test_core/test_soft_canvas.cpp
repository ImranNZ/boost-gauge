// The software canvas: shapes, text, pictures, supersampling, cache.

#include "test_support.h"

using namespace support;

TEST_CASE("softcanvas: size, clear, present counter") {
  SoftCanvas c(32, 16);
  CHECK(c.width() == 32);
  CHECK(c.height() == 16);
  c.clear(Color{9, 8, 7});
  CHECK(c.image().get(31, 15).r == 9);
  CHECK(c.framesPresented() == 0);
  c.present();
  CHECK(c.framesPresented() == 1);
}

TEST_CASE("softcanvas: circle") {
  SoftCanvas c(40, 40);
  c.fillCircle(20, 20, 5, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));
  CHECK(same(c.image().get(25, 20), kWhite));
  CHECK_FALSE(same(c.image().get(26, 20), kWhite));
  CHECK_FALSE(same(c.image().get(25, 25), kWhite));
}

TEST_CASE("softcanvas: arc quadrants, negative start and wrap-around") {
  SoftCanvas c(100, 100);

  c.fillArc(50, 50, 10, 20, -90.0f, 0.0f, kWhite);  // top-right quadrant
  CHECK(same(c.image().get(60, 40), kWhite));
  CHECK_FALSE(same(c.image().get(40, 60), kWhite));
  CHECK_FALSE(same(c.image().get(50, 50), kWhite));  // inside the hole

  c.clear(kBlack);
  c.fillArc(50, 50, 10, 20, 350.0f, 10.0f, kWhite);  // wraps through 0 degrees
  CHECK(same(c.image().get(65, 50), kWhite));
  CHECK_FALSE(same(c.image().get(50, 65), kWhite));

  c.clear(kBlack);
  c.fillArc(50, 50, 10, 20, 90.0f, 180.0f, kWhite);  // bottom-left (y grows down)
  CHECK(same(c.image().get(40, 60), kWhite));
  CHECK_FALSE(same(c.image().get(60, 40), kWhite));
}

TEST_CASE("softcanvas: lines, thickness clamp and zero length") {
  SoftCanvas c(40, 40);
  c.drawLine(5, 20, 35, 20, 1, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));
  CHECK_FALSE(same(c.image().get(20, 18), kWhite));

  c.clear(kBlack);
  c.drawLine(5, 20, 35, 20, 0, kWhite);  // clamped up to 1px
  CHECK(same(c.image().get(20, 20), kWhite));

  c.clear(kBlack);
  c.drawLine(5, 20, 35, 20, 7, kWhite);
  CHECK(same(c.image().get(20, 17), kWhite));

  c.clear(kBlack);
  c.drawLine(10, 10, 10, 10, 3, kWhite);  // degenerate segment
  CHECK(same(c.image().get(10, 10), kWhite));
}

TEST_CASE("softcanvas wedge: tapers from the base radius to the tip radius") {
  SoftCanvas c(100, 40);
  c.drawWedge(10.0f, 20.0f, 90.0f, 20.0f, 6.0f, 1.0f, kWhite);

  const int nearBase = columnHeight(c, 14);
  const int nearTip = columnHeight(c, 86);
  CHECK(nearBase > nearTip);
  CHECK(nearTip >= 1);
  CHECK(same(c.image().get(50, 20), kWhite));
  CHECK(same(c.image().get(95, 20), kBlack));  // past the tip
}

TEST_CASE("softcanvas wedge: accepts sub-pixel coordinates") {
  SoftCanvas a(60, 60);
  SoftCanvas b(60, 60);
  a.drawWedge(10.0f, 30.0f, 50.0f, 30.0f, 2.0f, 2.0f, kWhite);
  b.drawWedge(10.0f, 30.4f, 50.0f, 30.4f, 2.0f, 2.0f, kWhite);  // 0.4 px lower
  CHECK(columnHeight(a, 30) > 0);
  CHECK(columnHeight(b, 30) > 0);
}

TEST_CASE("softcanvas: text alignment, scale and unknown glyphs") {
  // '-' draws a 5px bar on glyph row 3.
  SoftCanvas left(60, 20);
  left.drawText(20, 0, "-", 1, TextAlign::Left, kWhite);
  CHECK(same(left.image().get(20, 3), kWhite));
  CHECK(same(left.image().get(24, 3), kWhite));
  CHECK_FALSE(same(left.image().get(25, 3), kWhite));

  SoftCanvas right(60, 20);
  right.drawText(20, 0, "-", 1, TextAlign::Right, kWhite);  // cell is 6 wide
  CHECK(same(right.image().get(14, 3), kWhite));
  CHECK_FALSE(same(right.image().get(20, 3), kWhite));

  SoftCanvas centre(60, 20);
  centre.drawText(20, 0, "-", 1, TextAlign::Center, kWhite);
  CHECK(same(centre.image().get(17, 3), kWhite));
  CHECK_FALSE(same(centre.image().get(16, 3), kWhite));

  SoftCanvas scaled(60, 20);
  scaled.drawText(0, 0, "-", 3, TextAlign::Left, kWhite);
  CHECK(same(scaled.image().get(14, 9), kWhite));  // 5*3 wide, row 3 * 3 tall
  CHECK(same(scaled.image().get(0, 11), kWhite));

  SoftCanvas unknown(60, 20);
  unknown.drawText(0, 0, "Z~", 2, TextAlign::Left, kWhite);
  CHECK(countNonBlack(unknown.image()) == 0);
}

TEST_CASE("softcanvas: every glyph the gauge uses renders something") {
  const std::string used = "0123456789-+.ABCEFIKLMNOPSTUV";
  for (char ch : used) {
    SoftCanvas c(10, 10);
    const char text[2] = {ch, 0};
    c.drawText(0, 0, text, 1, TextAlign::Left, kWhite);
    CAPTURE(ch);
    CHECK(countNonBlack(c.image()) > 0);
  }
}

TEST_CASE("drawMask: tints with the colour, using the mask as coverage") {
  const uint8_t alpha[] = {255, 0, 128, 255};  // 2 x 2
  const MaskView mask{2, 2, alpha};
  SoftCanvas c(10, 10);
  c.clear(Color{0, 0, 100});
  c.drawMask(mask, 3, 4, Color{200, 0, 0});

  CHECK(same(c.image().get(3, 4), Color{200, 0, 0}));    // fully covered
  CHECK(same(c.image().get(4, 4), Color{0, 0, 100}));    // no coverage: untouched
  CHECK(same(c.image().get(3, 5), blend(Color{0, 0, 100}, Color{200, 0, 0}, 128)));
  CHECK(same(c.image().get(4, 5), Color{200, 0, 0}));
}

TEST_CASE("drawMask: is clipped to the canvas and ignores an empty mask") {
  const uint8_t alpha[] = {255, 255, 255, 255};
  const MaskView mask{2, 2, alpha};
  SoftCanvas c(4, 4);
  c.drawMask(mask, -1, -1, kWhite);  // only the bottom-right pixel lands, at (0, 0)
  c.drawMask(mask, 3, 3, kWhite);    // only the top-left pixel lands, at (3, 3)
  CHECK(countNonBlack(c.image()) == 2);
  CHECK(same(c.image().get(0, 0), kWhite));
  CHECK(same(c.image().get(3, 3), kWhite));

  SoftCanvas empty(4, 4);
  empty.drawMask(MaskView{2, 2, nullptr}, 0, 0, kWhite);
  CHECK(countNonBlack(empty.image()) == 0);
}

TEST_CASE("drawMask: a supersampled canvas blends each covered pixel the same way") {
  const uint8_t alpha[] = {255, 128};
  const MaskView mask{2, 1, alpha};
  SoftCanvas c(8, 8, 4);
  c.drawMask(mask, 2, 3, kWhite);
  CHECK(same(c.image().get(2, 3), kWhite));
  CHECK(same(c.image().get(3, 3), blend(kBlack, kWhite, 128)));
}

TEST_CASE("drawImage: copies an opaque picture at an offset") {
  const uint16_t px[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};  // red green / blue white
  const ImageView image{2, 2, px, nullptr};
  SoftCanvas c(10, 10);
  c.drawImage(image, 3, 4);

  CHECK(same(c.image().get(3, 4), Color{255, 0, 0}));
  CHECK(same(c.image().get(4, 4), Color{0, 255, 0}));
  CHECK(same(c.image().get(3, 5), Color{0, 0, 255}));
  CHECK(same(c.image().get(4, 5), kWhite));
  CHECK(same(c.image().get(2, 4), kBlack));  // untouched around it
  CHECK(same(c.image().get(5, 5), kBlack));
}

TEST_CASE("drawImage: ignores a null picture and clips on every edge") {
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  const ImageView image{2, 2, px, nullptr};
  SoftCanvas c(4, 4);

  c.drawImage(ImageView{}, 0, 0);  // nothing to draw
  CHECK(same(c.image().get(0, 0), kBlack));

  c.drawImage(image, -1, 0);  // left edge: only the right column lands, at x = 0
  CHECK(same(c.image().get(0, 0), kWhite));
  c.clear(kBlack);
  c.drawImage(image, 0, -1);  // top edge
  CHECK(same(c.image().get(0, 0), kWhite));
  c.clear(kBlack);
  c.drawImage(image, 3, 0);   // right edge: only x = 3 lands
  CHECK(same(c.image().get(3, 0), kWhite));
  CHECK(same(c.image().get(2, 0), kBlack));
  c.clear(kBlack);
  c.drawImage(image, 0, 3);   // bottom edge
  CHECK(same(c.image().get(0, 3), kWhite));
  CHECK(same(c.image().get(0, 2), kBlack));
}

TEST_CASE("drawImage: alpha skips, blends and overwrites") {
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF};
  const uint8_t alpha[] = {0, 128, 255};
  const ImageView image{3, 1, px, alpha};
  SoftCanvas c(3, 1);
  c.clear(Color{100, 100, 100});
  c.drawImage(image, 0, 0);

  CHECK(same(c.image().get(0, 0), Color{100, 100, 100}));  // alpha 0: unchanged
  const Color blended = c.image().get(1, 0);
  CHECK(blended.r > 100);
  CHECK(blended.r < 255);  // halfway-ish between the background and white
  CHECK(same(c.image().get(2, 0), kWhite));  // alpha 255: replaced
}

TEST_CASE("drawImage: supersampled canvas reproduces the picture exactly") {
  const uint16_t px[] = {0xF800, 0x07E0};
  const ImageView image{2, 1, px, nullptr};
  SoftCanvas c(4, 2, 4);
  c.drawImage(image, 1, 1);
  CHECK(same(c.image().get(1, 1), Color{255, 0, 0}));
  CHECK(same(c.image().get(2, 1), Color{0, 255, 0}));
  CHECK(same(c.image().get(0, 0), kBlack));
}

TEST_CASE("supersampling: logical size is unchanged and interiors stay exact") {
  SoftCanvas c(40, 40, 4);
  CHECK(c.width() == 40);
  CHECK(c.height() == 40);
  CHECK(c.image().width() == 40);
  CHECK(c.image().height() == 40);

  c.fillCircle(20, 20, 10, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));  // fully covered: exact colour
  CHECK(same(c.image().get(2, 2), kBlack));    // untouched
}

TEST_CASE("supersampling: edges blend instead of stepping") {
  SoftCanvas hard(40, 40, 1);
  SoftCanvas smooth(40, 40, 4);
  hard.fillCircle(20, 20, 10, kWhite);
  smooth.fillCircle(20, 20, 10, kWhite);

  int hardPartial = 0, smoothPartial = 0;
  for (int y = 0; y < 40; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (partial(hard.image().get(x, y))) ++hardPartial;
      if (partial(smooth.image().get(x, y))) ++smoothPartial;
    }
  }
  CHECK(hardPartial == 0);
  CHECK(smoothPartial > 20);
}

TEST_CASE("supersampling: arcs, lines and text render at scale") {
  SoftCanvas c(60, 60, 3);
  c.fillArc(30, 30, 15, 25, 0.0f, 90.0f, kWhite);
  CHECK(same(c.image().get(30 + 20, 30 + 1), kWhite));   // inside the ring, near 3 o'clock
  CHECK(same(c.image().get(30 - 20, 30), kBlack));

  c.clear(kBlack);
  c.drawLine(5, 30, 55, 30, 4, kWhite);
  CHECK(same(c.image().get(30, 30), kWhite));

  c.clear(kBlack);
  c.drawText(10, 10, "0", 2, TextAlign::Left, kWhite);
  CHECK(columnHeight(c, 14) > 0);
}

TEST_CASE("supersampling: factor below 1 is treated as 1") {
  SoftCanvas c(10, 10, 0);
  c.fillCircle(5, 5, 2, kWhite);
  CHECK(same(c.image().get(5, 5), kWhite));
  CHECK(c.image().width() == 10);
}

TEST_CASE("background: restore reports nothing saved, then restores exactly") {
  for (int k : {1, 4}) {
    SoftCanvas c(20, 20, k);
    CAPTURE(k);
    CHECK_FALSE(c.restoreBackground());

    c.clear(Color{10, 20, 30});
    c.saveBackground();
    c.clear(kWhite);
    CHECK(same(c.image().get(5, 5), kWhite));

    CHECK(c.restoreBackground());
    CHECK(same(c.image().get(5, 5), Color{10, 20, 30}));
    CHECK(c.restoreBackground());  // reusable every frame
  }
}

TEST_CASE("discardBackground forgets the saved frame") {
  SoftCanvas c(8, 8);
  c.clear(kWhite);
  c.saveBackground();
  CHECK(c.restoreBackground());
  c.discardBackground();
  CHECK_FALSE(c.restoreBackground());
}
