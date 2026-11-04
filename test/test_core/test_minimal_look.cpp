// The Minimal look: a glowing arc grown from zero, no needle.

#include "test_support.h"

using namespace support;

namespace {

const MinimalLook& defaultLook() {
  static const MinimalLook look(minimalFonts());
  return look;
}

SoftCanvas draw(const MinimalLook& look, float psi, float peak = 0.0f, bool linked = true,
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

TEST_CASE("minimal look: colour ramp is blue below zero, then teal, lime, amber, red") {
  CHECK(same(defaultLook().colorFor(-8.0f), Color{74, 163, 255}));
  CHECK(same(defaultLook().colorFor(0.0f), Color{47, 224, 176}));
  CHECK(same(defaultLook().colorFor(12.0f), Color{155, 224, 74}));
  CHECK(same(defaultLook().colorFor(17.0f), Color{255, 176, 32}));
  CHECK(same(defaultLook().colorFor(21.0f), Color{255, 77, 77}));
  CHECK(same(defaultLook().colorFor(25.0f), Color{255, 77, 77}));  // clamped at the top

  const Color halfway = defaultLook().colorFor(6.0f);  // between teal and lime
  CHECK(halfway.r > 47);
  CHECK(halfway.r < 155);
}

TEST_CASE("minimal look: draws the track and scale once, then restores them") {
  MinimalLook look(minimalFonts());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(5, 5), MinimalLook::kBackground));
  CHECK(same(canvas.image().get(120, 20), MinimalLook::kTrack));          // top of the empty track
  CHECK(countColor(canvas, Color{138, 147, 157}, 0, 0, 240, 240) > 100);  // scale labels

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  drawFrame(look, canvas, state);
  CHECK(same(canvas.image().get(120, 20), MinimalLook::kTrack));
}

TEST_CASE("minimal look: boost grows the arc clockwise from zero, in the ramp colour") {
  MinimalLook look(minimalFonts());
  const SoftCanvas canvas = draw(look, 10.0f);
  const Color arc = defaultLook().colorFor(10.0f);

  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), arc));  // between zero and the tip
  CHECK(same(pointOnNeedle(canvas, 15.0f, 100.0f), MinimalLook::kTrack));  // beyond the tip
  CHECK(same(pointOnNeedle(canvas, -8.0f, 100.0f), MinimalLook::kTrack));  // the vacuum side
}

TEST_CASE("minimal look: vacuum grows the arc anticlockwise from zero, in blue") {
  MinimalLook look(minimalFonts());
  const SoftCanvas canvas = draw(look, -8.0f);

  CHECK(same(pointOnNeedle(canvas, -4.0f, 100.0f), Color{74, 163, 255}));
  CHECK(same(pointOnNeedle(canvas, -13.0f, 100.0f), MinimalLook::kTrack));
  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
}

TEST_CASE("minimal look: a glow of the arc colour sits behind the arc") {
  MinimalLook look(minimalFonts());
  const SoftCanvas canvas = draw(look, 10.0f);
  const Color glow = blend(MinimalLook::kBackground, defaultLook().colorFor(10.0f), 26);
  CHECK(same(pointOnNeedle(canvas, 5.0f, 90.0f), glow));
}

TEST_CASE("minimal look: no arc while the pressure is essentially zero") {
  MinimalLook look(minimalFonts());
  const SoftCanvas canvas = draw(look, 0.01f);
  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
  CHECK(same(pointOnNeedle(canvas, -5.0f, 100.0f), MinimalLook::kTrack));
}

TEST_CASE("minimal look: peak dot only above zero and only while linked") {
  MinimalLook look(minimalFonts());
  CHECK(same(pointOnNeedle(draw(look, 2.0f, 15.0f), 15.0f, 100.0f), kWhite));
  CHECK_FALSE(same(pointOnNeedle(draw(look, 2.0f, 0.0f), 15.0f, 100.0f), kWhite));
  CHECK_FALSE(same(pointOnNeedle(draw(look, 2.0f, 15.0f, false), 15.0f, 100.0f), kWhite));
}

TEST_CASE("minimal look: readout and mode label follow the sign") {
  MinimalLook look(minimalFonts());
  const Color vacuum{74, 163, 255};
  const Color boost = defaultLook().colorFor(8.0f);

  const SoftCanvas inBoost = draw(look, 8.0f);
  CHECK(countColor(inBoost, boost, 80, 66, 160, 84) > 20);  // BOOST label
  CHECK(countColor(inBoost, vacuum, 80, 66, 160, 84) == 0);
  CHECK(countColor(inBoost, kWhite, 40, 100, 200, 140) > 100);  // the big number

  const SoftCanvas inVacuum = draw(look, -8.0f);
  CHECK(countColor(inVacuum, vacuum, 80, 66, 160, 84) > 20);  // VACUUM label
}

TEST_CASE("minimal look: peak text only with a peak") {
  MinimalLook look(minimalFonts());
  const Color subtle{207, 213, 219};
  CHECK(countColor(draw(look, 2.0f, 12.0f), subtle, 60, 168, 180, 190) > 20);
  CHECK(countColor(draw(look, 2.0f, 0.0f), subtle, 60, 168, 180, 190) == 0);
  CHECK(countColor(draw(look, 2.0f, 12.0f, false), subtle, 60, 168, 180, 190) == 0);
}

TEST_CASE("minimal look: unlinked hides the arc and says NO LINK") {
  MinimalLook look(minimalFonts());
  const SoftCanvas canvas = draw(look, 10.0f, 12.0f, false);

  CHECK(same(pointOnNeedle(canvas, 5.0f, 100.0f), MinimalLook::kTrack));
  CHECK(countColor(canvas, Color{255, 77, 77}, 80, 66, 160, 84) > 20);  // the banner
  CHECK(countColor(canvas, kWhite, 40, 100, 200, 140) == 0);            // no live number
}

TEST_CASE("minimal look: fps label only for a positive value") {
  MinimalLook look(minimalFonts());
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
  MinimalLook look(minimalFonts(), GaugeProfile{}, badge);

  SoftCanvas canvas(240, 240);
  drawFrame(look, canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), MinimalLook::kBackground));  // transparent corner
}

TEST_CASE("minimal look: a bar profile lays the scale out in bar") {
  const GaugeProfile profile = GaugeProfile::bar();
  const GaugeScale scale = profile.scale();
  MinimalLook look(minimalFonts(), profile);
  const Color labelColour{138, 147, 157};

  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.value = 10.0f;  // 0.69 bar
  drawFrame(look, canvas, state);

  // Labels at the ends, zero and 1.0 bar: "0" and "1.0" are drawn through their centres.
  CHECK(same(pointOnNeedle(canvas, 0.0f, 82.0f, scale), labelColour));
  CHECK(same(pointOnNeedle(canvas, 1.0f, 82.0f, scale), labelColour));
  // The arc grows from zero to 0.69 bar, in the colour for 0.69 bar.
  CHECK(
      same(pointOnNeedle(canvas, 0.345f, 100.0f, scale), look.colorFor(profile.toDisplay(10.0f))));
  CHECK(same(pointOnNeedle(canvas, 1.2f, 100.0f, scale), MinimalLook::kTrack));
}

TEST_CASE("minimal look: the heat ramp is laid out relative to where the red zone starts") {
  const MinimalLook bar(minimalFonts(), GaugeProfile::bar());
  CHECK(same(bar.colorFor(-0.2f), Color{74, 163, 255}));
  CHECK(same(bar.colorFor(0.0f), Color{47, 224, 176}));
  CHECK(same(bar.colorFor(1.7f), Color{255, 77, 77}));  // beyond the last stop (1.05 x 1.4 bar)
  const Color lime = bar.colorFor(0.84f);               // 0.6 of the red zone's start
  CHECK(lime.r >= 153);
  CHECK(lime.r <= 157);
}

TEST_CASE("minimal look: scale labels cope with a range that starts at or below zero") {
  const auto drawn = [](GaugeProfile profile) {
    MinimalLook look(minimalFonts(), profile);
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    drawFrame(look, canvas, state);
    return canvas;
  };
  const Color labelColour{138, 147, 157};

  // 0 to 30 psi: the bottom of the scale is zero, so it is labelled once.
  const GaugeProfile fromZero{Unit::Psi, 0.0f, 30.0f, 5.0f, 20.0f, 25.0f};
  const SoftCanvas a = drawn(fromZero);
  CHECK(same(pointOnNeedle(a, 0.0f, 82.0f, fromZero.scale()), labelColour));

  // -10 to 0 psi (vacuum only): the top is zero too, so no extra label for it.
  const GaugeProfile vacuumOnly{Unit::Psi, -10.0f, 0.0f, 5.0f, 15.0f, 20.0f};
  const SoftCanvas b = drawn(vacuumOnly);
  CHECK(same(pointOnNeedle(b, 0.0f, 82.0f, vacuumOnly.scale()), labelColour));
  CHECK(same(pointOnNeedle(b, -10.0f, 82.0f, vacuumOnly.scale()), labelColour));  // "-10"
}

TEST_CASE("minimal look: a very wide range stops at a sensible number of labels") {
  const GaugeProfile wide{Unit::Psi, 0.0f, 1000.0f, 5.0f, 800.0f, 900.0f};
  MinimalLook look(minimalFonts(), wide);
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  drawFrame(look, canvas, state);
  CHECK(same(pointOnNeedle(canvas, 0.0f, 82.0f, wide.scale()), Color{138, 147, 157}));
}

TEST_CASE("minimal look: a channel with no vacuum label keeps its own label below zero") {
  const MinimalLook look(minimalFonts(), GaugeProfile::intakeAir());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.value = -5.0f;  // a cold day
  drawFrame(look, canvas, state);
  // The label is drawn in the colour for the value: blue below zero.
  CHECK(countColor(canvas, Color{74, 163, 255}, 80, 66, 160, 84) > 20);
}
