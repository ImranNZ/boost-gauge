// The Retro 60s look.

#include "test_support.h"

using namespace support;

TEST_CASE("retro look: draws the dial picture once, then restores it") {
  FlatDial dial(kCream);
  RetroLook look(dial.view(), kRetroSpec);
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 5.0f;

  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), rgb565ToColor(kCream)));
}

TEST_CASE("retro look: needle follows the pressure, with a tail and a hub") {
  FlatDial dial(kCream);
  RetroLook look(dial.view(), kRetroSpec);

  for (float psi : {-12.0f, 0.0f, 9.0f, 20.0f}) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    state.psi = psi;
    drawFrame(look, canvas, state);
    CAPTURE(psi);
    CHECK(same(pointOnNeedle(canvas, psi, 60.0f), RetroLook::kNeedleRed));
    CHECK(same(pointOnNeedle(canvas, psi, -20.0f), RetroLook::kNeedleRed));  // counterweight tail
  }

  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(120, 120), Color{5, 5, 6}));  // hub cap
}

TEST_CASE("retro look: peak flag only above zero and only while linked") {
  FlatDial dial(kCream);
  RetroLook look(dial.view(), kRetroSpec);

  SoftCanvas withPeak(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 2.0f;
  state.peakPsi = 18.0f;
  drawFrame(look, withPeak, state);
  CHECK(same(pointOnNeedle(withPeak, 18.0f, 103.0f), RetroLook::kInk));

  SoftCanvas noPeak(240, 240);
  state.peakPsi = 0.0f;
  drawFrame(look, noPeak, state);
  CHECK_FALSE(same(pointOnNeedle(noPeak, 18.0f, 103.0f), RetroLook::kInk));
}

TEST_CASE("retro look: unlinked hides the needle and says NO LINK") {
  FlatDial dial(kCream);
  RetroLook look(dial.view(), kRetroSpec);
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = false;
  state.psi = 10.0f;
  state.peakPsi = 12.0f;
  drawFrame(look, canvas, state);

  CHECK_FALSE(same(pointOnNeedle(canvas, 10.0f, 60.0f), RetroLook::kNeedleRed));
  CHECK(countColor(canvas, Color{161, 29, 20}, 80, 130, 160, 160) > 20);  // the red banner
}

TEST_CASE("retro look: fps label only for a positive value") {
  FlatDial dial(kCream);
  RetroLook look(dial.view(), kRetroSpec);
  const auto lit = [&look](int fps) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    state.fps = fps;
    drawFrame(look, canvas, state);
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
  RetroLook look(dial.view(), kRetroSpec, badge);

  SoftCanvas canvas(240, 240);
  drawFrame(look, canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), rgb565ToColor(kCream)));  // transparent corner
}
