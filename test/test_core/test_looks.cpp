// Looks: picture drawing, the Retro look, and switching looks at run time.

#include <doctest.h>

#include <cmath>
#include <string>
#include <vector>

#include "GaugeApp.h"
#include "GaugeScale.h"
#include "GaugeView.h"
#include "ImageView.h"
#include "ManualClock.h"
#include "RetroLook.h"
#include "SimCommands.h"
#include "SoftCanvas.h"

using namespace gauge;

namespace {

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

uint16_t pack(int r, int g, int b) {
  return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/// A 240x240 dial of one flat colour, standing in for the real artwork.
class FlatDial {
 public:
  explicit FlatDial(uint16_t value) : pixels_(240 * 240, value) {}
  ImageView view() const { return {240, 240, pixels_.data(), nullptr}; }

 private:
  std::vector<uint16_t> pixels_;
};

constexpr uint16_t kCream = (30 << 11) | (57 << 5) | 25;  // ~ (247, 229, 205)

Color pointOnNeedle(const SoftCanvas& canvas, float psi, float radius) {
  const float rad = GaugeScale{}.angleFor(psi) * 3.14159265f / 180.0f;
  return canvas.image().get(120 + static_cast<int>(std::lround(std::cos(rad) * radius)),
                            120 + static_cast<int>(std::lround(std::sin(rad) * radius)));
}

int countColor(const SoftCanvas& canvas, Color c, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x)
      if (same(canvas.image().get(x, y), c)) ++n;
  return n;
}

}  // namespace

// ------------------------------------------------------------- pictures ---

TEST_CASE("rgb565ToColor expands 5/6/5 bits to full range") {
  CHECK(same(rgb565ToColor(0x0000), kBlack));
  CHECK(same(rgb565ToColor(0xFFFF), kWhite));
  CHECK(same(rgb565ToColor(0xF800), Color{255, 0, 0}));
  CHECK(same(rgb565ToColor(0x07E0), Color{0, 255, 0}));
  CHECK(same(rgb565ToColor(0x001F), Color{0, 0, 255}));
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

TEST_CASE("discardBackground forgets the saved frame") {
  SoftCanvas c(8, 8);
  c.clear(kWhite);
  c.saveBackground();
  CHECK(c.restoreBackground());
  c.discardBackground();
  CHECK_FALSE(c.restoreBackground());
}

// ----------------------------------------------------------- retro look ---

TEST_CASE("retro look: draws the dial picture once, then restores it") {
  FlatDial dial(kCream);
  RetroLook look(dial.view());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 5.0f;

  look.draw(canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  look.draw(canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));
}

TEST_CASE("retro look: needle follows the pressure, with a tail and a hub") {
  FlatDial dial(kCream);
  RetroLook look(dial.view());

  for (float psi : {-12.0f, 0.0f, 9.0f, 20.0f}) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    state.psi = psi;
    look.draw(canvas, state);
    CAPTURE(psi);
    CHECK(same(pointOnNeedle(canvas, psi, 60.0f), RetroLook::kNeedleRed));
    CHECK(same(pointOnNeedle(canvas, psi, -20.0f), RetroLook::kNeedleRed));  // counterweight tail
  }

  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  look.draw(canvas, state);
  CHECK(same(canvas.image().get(120, 120), Color{5, 5, 6}));  // hub cap
}

TEST_CASE("retro look: peak flag only above zero and only while linked") {
  FlatDial dial(kCream);
  RetroLook look(dial.view());

  SoftCanvas withPeak(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 2.0f;
  state.peakPsi = 18.0f;
  look.draw(withPeak, state);
  CHECK(same(pointOnNeedle(withPeak, 18.0f, 103.0f), RetroLook::kInk));

  SoftCanvas noPeak(240, 240);
  state.peakPsi = 0.0f;
  look.draw(noPeak, state);
  CHECK_FALSE(same(pointOnNeedle(noPeak, 18.0f, 103.0f), RetroLook::kInk));
}

TEST_CASE("retro look: unlinked hides the needle and says NO LINK") {
  FlatDial dial(kCream);
  RetroLook look(dial.view());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = false;
  state.psi = 10.0f;
  state.peakPsi = 12.0f;
  look.draw(canvas, state);

  CHECK_FALSE(same(pointOnNeedle(canvas, 10.0f, 60.0f), RetroLook::kNeedleRed));
  CHECK(countColor(canvas, Color{161, 29, 20}, 80, 130, 160, 160) > 20);  // the red banner
}

TEST_CASE("retro look: fps label only for a positive value") {
  FlatDial dial(kCream);
  RetroLook look(dial.view());
  const auto lit = [&look](int fps) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    state.fps = fps;
    look.draw(canvas, state);
    return countColor(canvas, Color{107, 100, 85}, 90, 185, 150, 200);
  };
  CHECK(lit(0) == 0);
  CHECK(lit(60) > 0);
}

TEST_CASE("retro look: optional badge is laid over the dial") {
  FlatDial dial(kCream);
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  const uint8_t alpha[] = {255, 255, 255, 0};  // last pixel fully transparent
  RetroLook::Badge badge;
  badge.image = {2, 2, px, alpha};
  badge.x = 20;
  badge.y = 30;
  RetroLook look(dial.view(), badge);

  SoftCanvas canvas(240, 240);
  look.draw(canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), rgb565ToColor(kCream)));  // transparent corner
}

// ------------------------------------------------------ switching looks ---

namespace {

class FixedSource : public IReadingSource {
 public:
  Sample latest() override { return {{5.0f, true}, 1}; }
};

}  // namespace

TEST_CASE("app uses the Classic look by default and can switch to another") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  FlatDial dial(kCream);
  RetroLook retro(dial.view());
  GaugeApp app(source, canvas, clock);

  app.tick();
  CHECK(same(canvas.image().get(2, 2), kBlack));  // Classic: black dial

  app.setLook(retro);  // must drop the cached Classic dial
  clock.advance(33);
  app.tick();
  CHECK(same(canvas.image().get(2, 2), rgb565ToColor(kCream)));
}

TEST_CASE("app can be constructed with an explicit look") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  FlatDial dial(kCream);
  RetroLook retro(dial.view());
  GaugeApp app(source, canvas, clock, retro, GaugeApp::Options{});

  app.tick();
  CHECK(same(canvas.image().get(2, 2), rgb565ToColor(kCream)));
}

// ----------------------------------------------------- look command (sim) ---

TEST_CASE("parseLookCommand recognises 'look <name>'") {
  std::string name;
  CHECK(parseLookCommand("look retro", name));
  CHECK(name == "retro");
  CHECK(parseLookCommand("  look   classic \r\n", name));
  CHECK(name == "classic");

  CHECK_FALSE(parseLookCommand("look", name));
  CHECK_FALSE(parseLookCommand("look   ", name));
  CHECK_FALSE(parseLookCommand("looks retro", name));
  CHECK_FALSE(parseLookCommand("look two words", name));
  CHECK_FALSE(parseLookCommand("boost 5", name));
  CHECK_FALSE(parseLookCommand("", name));
}
