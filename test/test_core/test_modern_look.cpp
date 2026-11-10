// The Modern Mini look: a picture dial with an orange needle and a live readout.

#include "test_support.h"

using namespace support;

namespace {

/// The geometry the real artwork is generated with (looks/modern/spec.json).
constexpr ModernLook::Spec kSpec{-15.0f, 25.0f, 86.0f, 18.0f, 100.0f, 108.0f, 9};

constexpr Color kDim{154, 160, 167};
constexpr Color kNoLinkRed{255, 90, 77};

ModernLook::Fonts fonts() { return {kNumberFont.font(), kLabelFont.font()}; }

SoftCanvas draw(const ModernLook& look, float psi, float peak = 0.0f, bool linked = true,
                int fps = 0) {
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = linked;
  state.value = psi;
  state.peakValue = peak;
  state.fps = fps;
  drawFrame(look, canvas, state);
  return canvas;
}

}  // namespace

TEST_CASE("modern look: draws the dial picture once, then restores it") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.value = 5.0f;

  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));
}

TEST_CASE("modern look: needle follows the pressure, with a tail and a hub") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());

  for (float psi : {-12.0f, 0.0f, 9.0f, 20.0f}) {
    const SoftCanvas canvas = draw(look, psi);
    CAPTURE(psi);
    CHECK(same(pointOnNeedle(canvas, psi, 60.0f), ModernLook::kNeedle));
    CHECK(same(pointOnNeedle(canvas, psi, -12.0f), ModernLook::kNeedle));  // the tail
  }

  const SoftCanvas canvas = draw(look, 0.0f);
  CHECK(same(canvas.image().get(120, 120), Color{21, 23, 26}));  // the hub's centre cap
}

TEST_CASE("modern look: needle casts a shadow down and to the right") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());
  const SoftCanvas canvas =
      draw(look, -15.0f + 45.0f / 6.75f);  // pointing left, level with the pivot

  // Left of the pivot, along the needle: orange. A couple of pixels below it: its shadow.
  CHECK(same(canvas.image().get(60, 120), ModernLook::kNeedle));
  CHECK(same(canvas.image().get(60, 123), Color{2, 2, 3}));
  CHECK(same(canvas.image().get(60, 135), rgb565ToColor(kCream)));  // and nothing further
}

TEST_CASE("modern look: peak flag only above zero and only while linked") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());

  CHECK(same(pointOnNeedle(draw(look, 2.0f, 18.0f), 18.0f, 104.0f), ModernLook::kFlag));
  CHECK_FALSE(same(pointOnNeedle(draw(look, 2.0f, 0.0f), 18.0f, 104.0f), ModernLook::kFlag));
  CHECK_FALSE(
      same(pointOnNeedle(draw(look, 2.0f, 18.0f, false), 18.0f, 104.0f), ModernLook::kFlag));
}

TEST_CASE("modern look: the number is drawn in the readout window") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());

  const SoftCanvas linked = draw(look, 7.0f);
  CHECK(countColor(linked, kWhite, 88, 140, 152, 190) > 100);

  // Unlinked: a dim placeholder instead, plus the NO LINK banner above the window.
  const SoftCanvas dead = draw(look, 7.0f, 12.0f, false);
  CHECK(countColor(dead, kWhite, 88, 140, 152, 190) == 0);
  CHECK(countColor(dead, kDim, 88, 140, 152, 190) > 100);
  CHECK(countColor(dead, kNoLinkRed, 80, 120, 160, 140) > 20);
  CHECK_FALSE(same(pointOnNeedle(dead, 7.0f, 60.0f), ModernLook::kNeedle));
}

TEST_CASE("modern look: fps label only for a positive value") {
  FlatDial dial(kCream);
  ModernLook look(dial.view(), kSpec, fonts());
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 0), kDim, 90, 200, 150, 220) == 0);
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 60), kDim, 90, 200, 150, 220) > 0);
}

TEST_CASE("modern look: optional badge is laid over the dial") {
  FlatDial dial(kCream);
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  const uint8_t alpha[] = {255, 255, 255, 0};  // last pixel fully transparent
  Badge badge;
  badge.picture = Picture(ImageView{2, 2, px, alpha});
  badge.x = 20;
  badge.y = 30;
  ModernLook look(dial.view(), kSpec, fonts(), badge);

  SoftCanvas canvas(240, 240);
  drawFrame(look, canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), rgb565ToColor(kCream)));  // transparent corner
}
