// ScaledSurface: a look drawn for 240x240 shown on any size.

#include "render/ScaledSurface.h"
#include "test_support.h"

using namespace support;

namespace {

/// Remembers the last call of each kind, and the pictures it was handed.
class RecordingSurface : public IDrawSurface {
 public:
  RecordingSurface(int w, int h) : w_(w), h_(h) {}

  int width() const override { return w_; }
  int height() const override { return h_; }
  void clear(Color) override { ++clears; }
  void fillCircle(int cx, int cy, int r, Color) override { circle = {cx, cy, r}; }
  void fillArc(int cx, int cy, int inner, int outer, float, float, Color) override {
    arc = {cx, cy, inner, outer};
  }
  void drawWedge(float x0, float y0, float x1, float y1, float r0, float r1, Color) override {
    wedge = {x0, y0, x1, y1, r0, r1};
  }
  void drawLine(int x0, int y0, int x1, int y1, int t, Color) override {
    line = {x0, y0, x1, y1, t};
  }
  void drawText(int x, int y, const char*, int scale, TextAlign, Color) override {
    text = {x, y, scale};
  }
  void drawImage(const ImageView& image, int x, int y) override {
    ++images;
    imageAt = {x, y};
    imageSize = {image.width, image.height};
    imageHadAlpha = image.alpha != nullptr;
    for (int i = 0; i < image.width * image.height; ++i) {
      imagePixels.push_back(image.rgb565[i]);
      if (image.alpha != nullptr) imageAlpha.push_back(image.alpha[i]);
    }
  }
  void drawMask(const MaskView& mask, float x, float y, Color) override {
    ++masks;
    maskAt = {x, y};
    maskSize = {mask.width, mask.height};
    for (int i = 0; i < mask.width * mask.height; ++i) maskPixels.push_back(mask.alpha[i]);
  }

  int clears = 0;
  std::array<int, 3> circle{};
  std::array<int, 4> arc{};
  std::array<float, 6> wedge{};
  std::array<int, 5> line{};
  std::array<int, 3> text{};
  int images = 0;
  std::array<int, 2> imageAt{};
  std::array<int, 2> imageSize{};
  bool imageHadAlpha = false;
  std::vector<uint16_t> imagePixels;
  std::vector<uint8_t> imageAlpha;
  int masks = 0;
  std::array<float, 2> maskAt{};
  std::array<int, 2> maskSize{};
  std::vector<uint8_t> maskPixels;

 private:
  int w_;
  int h_;
};

}  // namespace

TEST_CASE("scaled surface: the scale is the shorter side over the design size") {
  CHECK(ScaledSurface::scaleFor(240, 240) == 1.0f);
  CHECK(ScaledSurface::scaleFor(466, 466) == doctest::Approx(466.0f / 240.0f));
  CHECK(ScaledSurface::scaleFor(720, 720) == 3.0f);
  // A portrait or landscape screen is still fitted to its shorter side.
  CHECK(ScaledSurface::scaleFor(320, 480) == doctest::Approx(320.0f / 240.0f));
  CHECK(ScaledSurface::scaleFor(480, 320) == doctest::Approx(320.0f / 240.0f));
}

TEST_CASE("scaled surface: a look always sees the 240x240 design space") {
  RecordingSurface big(466, 466);
  ScaledSurface surface(big);
  CHECK(surface.width() == 240);
  CHECK(surface.height() == 240);
  CHECK(surface.scale() == doctest::Approx(466.0f / 240.0f));
}

TEST_CASE("scaled surface: at 240x240 every call passes straight through") {
  RecordingSurface real(240, 240);
  ScaledSurface surface(real);
  CHECK(surface.scale() == 1.0f);

  surface.clear(kWhite);
  surface.fillCircle(120, 100, 10, kWhite);
  surface.fillArc(120, 120, 95, 105, 0.0f, 90.0f, kWhite);
  surface.drawWedge(120.0f, 120.0f, 200.5f, 60.25f, 5.0f, 0.6f, kWhite);
  surface.drawLine(1, 2, 3, 4, 2, kWhite);
  surface.drawText(120, 134, "1", 4, TextAlign::Center, kWhite);

  CHECK(real.clears == 1);
  CHECK(real.circle == std::array<int, 3>{120, 100, 10});
  CHECK(real.arc == std::array<int, 4>{120, 120, 95, 105});
  CHECK(real.wedge == std::array<float, 6>{120.0f, 120.0f, 200.5f, 60.25f, 5.0f, 0.6f});
  CHECK(real.line == std::array<int, 5>{1, 2, 3, 4, 2});
  CHECK(real.text == std::array<int, 3>{120, 134, 4});

  const uint16_t pixel = 0xFFFF;
  const uint8_t alpha = 128;
  surface.drawImage(ImageView{1, 1, &pixel, &alpha}, 7, 8);
  surface.drawMask(MaskView{1, 1, &alpha}, 9, 10, kWhite);
  CHECK(real.imageAt == std::array<int, 2>{7, 8});
  CHECK(real.imageSize == std::array<int, 2>{1, 1});
  CHECK(real.imageHadAlpha);
  CHECK(real.maskAt == std::array<float, 2>{9.0f, 10.0f});
}

TEST_CASE("scaled surface: shapes are scaled about the centre of the screen") {
  RecordingSurface real(480, 480);  // exactly 2x
  ScaledSurface surface(real);

  surface.fillCircle(120, 120, 10, kWhite);
  CHECK(real.circle == std::array<int, 3>{240, 240, 20});
  surface.fillArc(120, 120, 95, 105, 0.0f, 90.0f, kWhite);
  CHECK(real.arc == std::array<int, 4>{240, 240, 190, 210});
  surface.drawWedge(120.0f, 120.0f, 200.5f, 60.25f, 5.0f, 0.5f, kWhite);
  CHECK(real.wedge == std::array<float, 6>{240.0f, 240.0f, 401.0f, 120.5f, 10.0f, 1.0f});
  surface.drawLine(10, 20, 30, 40, 2, kWhite);
  CHECK(real.line == std::array<int, 5>{20, 40, 60, 80, 4});
  surface.drawText(120, 134, "1", 4, TextAlign::Center, kWhite);
  CHECK(real.text == std::array<int, 3>{240, 268, 8});
  surface.clear(kWhite);
  CHECK(real.clears == 1);
}

TEST_CASE("scaled surface: a thin line and small text never shrink away") {
  RecordingSurface real(60, 60);  // a quarter size
  ScaledSurface surface(real);
  surface.drawLine(0, 0, 120, 120, 1, kWhite);
  CHECK(real.line[4] == 1);
  surface.drawText(0, 0, "1", 1, TextAlign::Left, kWhite);
  CHECK(real.text[2] == 1);
}

TEST_CASE("scaled surface: a non-square screen centres the design on the longer side") {
  RecordingSurface wide(480, 320);  // the scale is set by the shorter side
  ScaledSurface surface(wide);
  CHECK(surface.scale() == doctest::Approx(320.0f / 240.0f));
  surface.fillCircle(120, 120, 6, kWhite);
  CHECK(wide.circle == std::array<int, 3>{240, 160, 8});  // the middle of 480 x 320

  RecordingSurface tall(320, 480);
  ScaledSurface tallSurface(tall);
  tallSurface.fillCircle(120, 120, 6, kWhite);
  CHECK(tall.circle == std::array<int, 3>{160, 240, 8});
}

TEST_CASE("scaled surface: a picture is resampled to the scaled size") {
  RecordingSurface real(480, 480);
  ScaledSurface surface(real);

  // 2x1 pixels, black then white: the result runs from black to white across 4x2.
  const uint16_t pixels[2] = {0x0000, 0xFFFF};
  surface.drawImage(ImageView{2, 1, pixels, nullptr}, 10, 20);

  CHECK(real.imageAt == std::array<int, 2>{20, 40});
  CHECK(real.imageSize == std::array<int, 2>{4, 2});
  CHECK_FALSE(real.imageHadAlpha);
  REQUIRE(real.imagePixels.size() == 8);
  CHECK(real.imagePixels[0] == 0x0000);  // left edge: all first pixel
  CHECK(real.imagePixels[3] == 0xFFFF);  // right edge: all second
  CHECK(real.imagePixels[1] > real.imagePixels[0]);
  CHECK(real.imagePixels[2] > real.imagePixels[1]);
  CHECK(real.imagePixels[4] == real.imagePixels[0]);  // both rows alike
}

TEST_CASE("scaled surface: a picture's alpha is resampled with it") {
  RecordingSurface real(480, 480);
  ScaledSurface surface(real);
  const uint16_t pixels[2] = {0xFFFF, 0xFFFF};
  const uint8_t alpha[2] = {0, 255};
  surface.drawImage(ImageView{2, 1, pixels, alpha}, 0, 0);

  CHECK(real.imageHadAlpha);
  REQUIRE(real.imageAlpha.size() == 8);
  CHECK(real.imageAlpha[0] == 0);
  CHECK(real.imageAlpha[3] == 255);
  CHECK(real.imageAlpha[1] > 0);
  CHECK(real.imageAlpha[1] < real.imageAlpha[2]);
}

TEST_CASE("scaled surface: a mask is resampled and drawn where the scaled text would go") {
  RecordingSurface real(480, 480);
  ScaledSurface surface(real);
  const uint8_t alpha[2] = {0, 255};
  surface.drawMask(MaskView{2, 1, alpha}, 5, 6, kWhite);

  CHECK(real.maskAt == std::array<float, 2>{10.0f, 12.0f});
  CHECK(real.maskSize == std::array<int, 2>{4, 2});
  REQUIRE(real.maskPixels.size() == 8);
  CHECK(real.maskPixels[0] == 0);
  CHECK(real.maskPixels[3] == 255);
  CHECK(real.maskPixels[1] < real.maskPixels[2]);
}

TEST_CASE("scaled surface: a picture shrunk below a pixel still draws one") {
  RecordingSurface real(60, 60);  // a quarter size
  ScaledSurface surface(real);
  const uint8_t alpha = 255;
  surface.drawMask(MaskView{1, 1, &alpha}, 0, 0, kWhite);
  CHECK(real.maskSize == std::array<int, 2>{1, 1});
}

TEST_CASE("scaled surface: a picture's bottom edge lands where the scaled baseline is") {
  RecordingSurface real(466, 466);
  ScaledSurface surface(real);
  const uint8_t alpha[3] = {255, 255, 255};
  // Three glyphs of different heights all ending on design row 100.
  for (int h = 1; h <= 3; ++h) {
    surface.drawMask(MaskView{1, h, alpha}, 0, 100 - h, kWhite);
    CHECK(static_cast<int>(real.maskAt[1]) + real.maskSize[1] ==
          static_cast<int>(std::lround(100 * 466.0 / 240.0)));
  }
}

TEST_CASE("scaled surface: a large picture is handed over in bands that join up") {
  RecordingSurface real(480, 480);
  ScaledSurface surface(real);

  // 100x100 -> 200x200 = 40,000 pixels: more than one band's worth.
  std::vector<uint16_t> pixels(100 * 100, 0x1234);
  surface.drawImage(ImageView{100, 100, pixels.data(), nullptr}, 0, 0);
  CHECK(real.images > 1);
  CHECK(real.imagePixels.size() == 200u * 200u);
  for (uint16_t p : real.imagePixels)
    CHECK(p == 0x1234);  // a flat colour survives the round trip through 8 bits

  std::vector<uint8_t> coverage(100 * 100, 200);
  surface.drawMask(MaskView{100, 100, coverage.data()}, 0, 0, kWhite);
  CHECK(real.masks > 1);
  CHECK(real.maskPixels.size() == 200u * 200u);
  for (uint8_t a : real.maskPixels) CHECK(a == 200);
}

TEST_CASE("scaled surface: nothing is drawn for a missing picture, or one too wide to band") {
  RecordingSurface real(480, 480);
  ScaledSurface surface(real);
  surface.drawImage(ImageView{}, 0, 0);
  surface.drawMask(MaskView{}, 0, 0, kWhite);
  CHECK(real.images == 0);
  CHECK(real.masks == 0);

  RecordingSurface same(240, 240);
  ScaledSurface passthrough(same);
  passthrough.drawMask(MaskView{}, 0, 0, kWhite);
  CHECK(same.masks == 0);

  // 3000 px wide at 2x is 6000: past what one band can hold, so it is skipped, not overrun.
  std::vector<uint16_t> wide(3000, 0xFFFF);
  surface.drawImage(ImageView{3000, 1, wide.data(), nullptr}, 0, 0);
  std::vector<uint8_t> wideMask(3000, 255);
  surface.drawMask(MaskView{3000, 1, wideMask.data()}, 0, 0, kWhite);
  CHECK(real.images == 0);
  CHECK(real.masks == 0);
}

TEST_CASE("frame renderer: a look is drawn in the 240 design space whatever the canvas size") {
  struct SizeLook : ILook {
    void drawDial(IDrawSurface& surface) const override {
      width = surface.width();
      height = surface.height();
    }
    void drawDynamic(IDrawSurface&, const GaugeState&) const override {}
    mutable int width = 0;
    mutable int height = 0;
  } look;

  SoftCanvas canvas(466, 466);
  FrameRenderer renderer(canvas);
  renderer.render(look, GaugeState{});
  CHECK(look.width == 240);
  CHECK(look.height == 240);
}
