// The Minimal look: a glowing arc grown from zero, no needle.

#include "test_support.h"

using namespace support;

namespace {

SoftCanvas draw(const MinimalLook& look, float psi, float peak = 0.0f, bool linked = true,
                int fps = 0) {
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = linked;
  state.psi = psi;
  state.peakPsi = peak;
  state.fps = fps;
  drawFrame(look, canvas, state);
  return canvas;
}

}  // namespace

TEST_CASE("minimal look: colour ramp is blue below zero, then teal, lime, amber, red") {
  CHECK(same(MinimalLook::colorFor(-8.0f), Color{74, 163, 255}));
  CHECK(same(MinimalLook::colorFor(0.0f), Color{47, 224, 176}));
  CHECK(same(MinimalLook::colorFor(12.0f), Color{155, 224, 74}));
  CHECK(same(MinimalLook::colorFor(17.0f), Color{255, 176, 32}));
  CHECK(same(MinimalLook::colorFor(21.0f), Color{255, 77, 77}));
  CHECK(same(MinimalLook::colorFor(25.0f), Color{255, 77, 77}));  // clamped at the top

  const Color halfway = MinimalLook::colorFor(6.0f);  // between teal and lime
  CHECK(halfway.r > 47);
  CHECK(halfway.r < 155);
}

TEST_CASE("minimal look: draws the track and scale once, then restores them") {
  MinimalLook look;
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), MinimalLook::kBackground));
  CHECK(same(canvas.image().get(120, 20), MinimalLook::kTrack));  // top of the empty track
  CHECK(countColor(canvas, Color{138, 147, 157}, 0, 0, 240, 240) > 100);  // scale labels

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(120, 20), MinimalLook::kTrack));
}

TEST_CASE("minimal look: boost grows the arc clockwise from zero, in the ramp colour") {
  MinimalLook look;
  const SoftCanvas canvas = draw(look, 10.0f);
  const Color arc = MinimalLook::colorFor(10.0f);

  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), arc));    // between zero and the tip
  CHECK(same(pointOnNeedle(canvas, 15.0f, 100.0f), MinimalLook::kTrack));  // beyond the tip
  CHECK(same(pointOnNeedle(canvas, -8.0f, 100.0f), MinimalLook::kTrack));  // the vacuum side
}

TEST_CASE("minimal look: vacuum grows the arc anticlockwise from zero, in blue") {
  MinimalLook look;
  const SoftCanvas canvas = draw(look, -8.0f);

  CHECK(same(pointOnNeedle(canvas, -4.0f, 100.0f), Color{74, 163, 255}));
  CHECK(same(pointOnNeedle(canvas, -13.0f, 100.0f), MinimalLook::kTrack));
  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
}

TEST_CASE("minimal look: a glow of the arc colour sits behind the arc") {
  MinimalLook look;
  const SoftCanvas canvas = draw(look, 10.0f);
  const Color glow = blend(MinimalLook::kBackground, MinimalLook::colorFor(10.0f), 26);
  CHECK(same(pointOnNeedle(canvas, 5.0f, 90.0f), glow));
}

TEST_CASE("minimal look: no arc while the pressure is essentially zero") {
  MinimalLook look;
  const SoftCanvas canvas = draw(look, 0.01f);
  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
  CHECK(same(pointOnNeedle(canvas, -5.0f, 100.0f), MinimalLook::kTrack));
}

TEST_CASE("minimal look: peak dot only above zero and only while linked") {
  MinimalLook look;
  CHECK(same(pointOnNeedle(draw(look, 2.0f, 15.0f), 15.0f, 100.0f), kWhite));
  CHECK_FALSE(same(pointOnNeedle(draw(look, 2.0f, 0.0f), 15.0f, 100.0f), kWhite));
  CHECK_FALSE(same(pointOnNeedle(draw(look, 2.0f, 15.0f, false), 15.0f, 100.0f), kWhite));
}

TEST_CASE("minimal look: readout and mode label follow the sign") {
  MinimalLook look;
  const Color vacuum{74, 163, 255};
  const Color boost = MinimalLook::colorFor(8.0f);

  const SoftCanvas inBoost = draw(look, 8.0f);
  CHECK(countColor(inBoost, boost, 80, 66, 160, 84) > 20);    // BOOST label
  CHECK(countColor(inBoost, vacuum, 80, 66, 160, 84) == 0);
  CHECK(countColor(inBoost, kWhite, 40, 86, 200, 126) > 100);  // the big number

  const SoftCanvas inVacuum = draw(look, -8.0f);
  CHECK(countColor(inVacuum, vacuum, 80, 66, 160, 84) > 20);   // VACUUM label
}

TEST_CASE("minimal look: peak text only with a peak") {
  MinimalLook look;
  const Color subtle{207, 213, 219};
  CHECK(countColor(draw(look, 2.0f, 12.0f), subtle, 60, 190, 180, 215) > 20);
  CHECK(countColor(draw(look, 2.0f, 0.0f), subtle, 60, 190, 180, 215) == 0);
  CHECK(countColor(draw(look, 2.0f, 12.0f, false), subtle, 60, 190, 180, 215) == 0);
}

TEST_CASE("minimal look: unlinked hides the arc and says NO LINK") {
  MinimalLook look;
  const SoftCanvas canvas = draw(look, 10.0f, 12.0f, false);

  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
  CHECK(countColor(canvas, Color{255, 77, 77}, 80, 66, 160, 84) > 20);  // the banner
  CHECK(countColor(canvas, kWhite, 40, 86, 200, 126) == 0);             // no live number
}

TEST_CASE("minimal look: fps label only for a positive value") {
  MinimalLook look;
  const Color dim{138, 147, 157};
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 0), dim, 90, 220, 150, 236) == 0);
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 60), dim, 90, 220, 150, 236) > 0);
}

TEST_CASE("minimal look: optional badge is laid over the dial") {
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  const uint8_t alpha[] = {255, 255, 255, 0};  // last pixel fully transparent
  Badge badge;
  badge.image = {2, 2, px, alpha};
  badge.x = 20;
  badge.y = 30;
  MinimalLook look(badge);

  SoftCanvas canvas(240, 240);
  drawFrame(look, canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), MinimalLook::kBackground));  // transparent corner
}
