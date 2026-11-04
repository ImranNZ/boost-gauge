// The Motorsport look: a segmented bar and a shift-light rim.

#include "test_support.h"

using namespace support;

namespace {

const MotorsportLook& defaultLook() {
  static const MotorsportLook look(motorsportFonts());
  return look;
}

constexpr Color kOff = MotorsportLook::kSegmentOff;
constexpr Color kRed = MotorsportLook::kRed;
constexpr Color kAmber = MotorsportLook::kAmber;
constexpr Color kLabel{139, 147, 160};

SoftCanvas draw(const MotorsportLook& look, float psi, float peak = 0.0f, bool linked = true,
                int fps = 0, uint32_t timeMs = 0) {
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = linked;
  state.psi = psi;
  state.peakPsi = peak;
  state.fps = fps;
  state.timeMs = timeMs;
  drawFrame(look, canvas, state);
  return canvas;
}

/// The middle of segment `index`, `radius` pixels from the centre.
Color onSegment(const SoftCanvas& canvas, int index, float radius = 99.0f) {
  return pointOnNeedle(canvas, -15.0f + static_cast<float>(index) + 0.5f, radius);
}

}  // namespace

TEST_CASE("motorsport look: segment colours step blue, teal, lime, amber, red") {
  CHECK(same(defaultLook().segmentColor(0), Color{59, 139, 255}));
  CHECK(same(defaultLook().segmentColor(14), Color{59, 139, 255}));  // -0.5 psi
  CHECK(same(defaultLook().segmentColor(15), Color{25, 211, 162}));  // +0.5 psi
  CHECK(same(defaultLook().segmentColor(24), Color{25, 211, 162}));
  CHECK(same(defaultLook().segmentColor(25), Color{155, 224, 74}));  // 10.5 psi
  CHECK(same(defaultLook().segmentColor(29), Color{155, 224, 74}));
  CHECK(same(defaultLook().segmentColor(30), Color{255, 176, 32}));  // 15.5 psi
  CHECK(same(defaultLook().segmentColor(34), Color{255, 176, 32}));
  CHECK(same(defaultLook().segmentColor(35), kRed));  // 20.5 psi
  CHECK(same(defaultLook().segmentColor(39), kRed));
}

TEST_CASE("motorsport look: a segment lights once the pressure is past its middle") {
  CHECK(defaultLook().isLit(0, -14.5f));
  CHECK_FALSE(defaultLook().isLit(0, -14.6f));
  CHECK(defaultLook().isLit(39, 24.5f));
  CHECK_FALSE(defaultLook().isLit(39, 24.4f));
}

TEST_CASE("motorsport look: a pressure maps to its segment, clamped to the bar") {
  CHECK(defaultLook().segmentFor(-15.0f) == 0);
  CHECK(defaultLook().segmentFor(-40.0f) == 0);
  CHECK(defaultLook().segmentFor(0.0f) == 15);
  CHECK(defaultLook().segmentFor(17.3f) == 32);
  CHECK(defaultLook().segmentFor(60.0f) == 39);
}

TEST_CASE("motorsport look: draws the dim bar, rim and labels once, then restores them") {
  MotorsportLook look(motorsportFonts());
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = -20.0f;  // nothing lit
  drawFrame(look, canvas, state);

  CHECK(same(canvas.image().get(5, 5), MotorsportLook::kBackground));
  CHECK(same(onSegment(canvas, 20), kOff));
  CHECK(same(canvas.image().get(120, 3), MotorsportLook::kRimOff));
  CHECK(countColor(canvas, kLabel, 0, 0, 240, 240) > 150);  // scale labels, BOOST, PSI

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  drawFrame(look, canvas, state);
  CHECK(same(onSegment(canvas, 20), kOff));
}

TEST_CASE("motorsport look: lights segments up to the pressure, each in its zone colour") {
  MotorsportLook look(motorsportFonts());
  const SoftCanvas canvas = draw(look, 8.2f);

  CHECK(same(onSegment(canvas, 0), defaultLook().segmentColor(0)));  // vacuum blue
  CHECK(same(onSegment(canvas, 10), defaultLook().segmentColor(10)));
  CHECK(same(onSegment(canvas, 22), defaultLook().segmentColor(22)));  // last lit (7.5)
  CHECK(same(onSegment(canvas, 23), kOff));                            // 8.5 is above 8.2
  CHECK(same(onSegment(canvas, 39), kOff));
}

TEST_CASE("motorsport look: at full scale every segment is lit") {
  MotorsportLook look(motorsportFonts());
  const SoftCanvas canvas = draw(look, 25.0f);
  for (int i : {0, 15, 25, 30, 39}) {
    CAPTURE(i);
    CHECK(same(onSegment(canvas, i), defaultLook().segmentColor(i)));
  }
}

TEST_CASE("motorsport look: peak outlines its segment in white") {
  MotorsportLook look(motorsportFonts());

  // Peak above the current pressure: an unlit segment, outlined, empty inside.
  const SoftCanvas above = draw(look, 2.0f, 17.3f);
  CHECK(same(onSegment(above, 32, 90.0f), kWhite));
  CHECK(same(onSegment(above, 32), kOff));

  // Peak at the current pressure: the outlined segment is lit inside.
  const SoftCanvas at = draw(look, 17.6f, 17.6f);
  CHECK(same(onSegment(at, 32, 90.0f), kWhite));
  CHECK(same(onSegment(at, 32), kAmber));

  // No outline without a peak, or without a link.
  CHECK_FALSE(same(onSegment(draw(look, 2.0f, 0.0f), 32, 90.0f), kWhite));
  CHECK_FALSE(same(onSegment(draw(look, 2.0f, 17.3f, false), 32, 90.0f), kWhite));
}

TEST_CASE("motorsport look: the rim pulses red past 20 psi and stays dim below") {
  MotorsportLook look(motorsportFonts());
  const uint32_t brightMs = 141;  // sin(ms / 90) is at its peak
  const uint32_t dimMs = 424;     // and at its trough

  CHECK(same(draw(look, 22.0f, 0.0f, true, 0, brightMs).image().get(120, 3), kRed));
  const Color dim = draw(look, 22.0f, 0.0f, true, 0, dimMs).image().get(120, 3);
  CHECK_FALSE(same(dim, kRed));
  CHECK_FALSE(same(dim, MotorsportLook::kRimOff));  // a darker red, between the two

  CHECK(same(draw(look, 18.0f, 0.0f, true, 0, brightMs).image().get(120, 3),
             MotorsportLook::kRimOff));
  CHECK(same(draw(look, 22.0f, 0.0f, false, 0, brightMs).image().get(120, 3),
             MotorsportLook::kRimOff));
}

TEST_CASE("motorsport look: the readout turns red-hot past 20 psi") {
  MotorsportLook look(motorsportFonts());
  const Color hot{255, 90, 77};

  const SoftCanvas calm = draw(look, 8.0f);
  CHECK(countColor(calm, kWhite, 50, 94, 190, 128) > 100);
  CHECK(countColor(calm, hot, 50, 94, 190, 128) == 0);

  const SoftCanvas boosting = draw(look, 22.0f);
  CHECK(countColor(boosting, hot, 50, 94, 190, 128) > 100);
  CHECK(countColor(boosting, kWhite, 50, 94, 190, 128) == 0);
}

TEST_CASE("motorsport look: peak text only with a peak") {
  MotorsportLook look(motorsportFonts());
  CHECK(countColor(draw(look, 2.0f, 12.0f), kAmber, 60, 160, 180, 180) > 20);
  CHECK(countColor(draw(look, 2.0f, 0.0f), kAmber, 60, 160, 180, 180) == 0);
  CHECK(countColor(draw(look, 2.0f, 12.0f, false), kAmber, 60, 160, 180, 180) == 0);
}

TEST_CASE("motorsport look: unlinked lights nothing and says NO LINK") {
  MotorsportLook look(motorsportFonts());
  const SoftCanvas canvas = draw(look, 10.0f, 12.0f, false);

  CHECK(same(onSegment(canvas, 5), kOff));
  CHECK(countColor(canvas, kRed, 80, 68, 160, 86) > 20);     // the banner
  CHECK(countColor(canvas, kWhite, 50, 94, 190, 128) == 0);  // no live number
}

TEST_CASE("motorsport look: fps label only for a positive value") {
  MotorsportLook look(motorsportFonts());
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 0), kLabel, 90, 216, 150, 232) == 0);
  CHECK(countColor(draw(look, 2.0f, 0.0f, true, 60), kLabel, 90, 216, 150, 232) > 0);
}

TEST_CASE("motorsport look: optional badge is laid over the dial") {
  const uint16_t px[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  const uint8_t alpha[] = {255, 255, 255, 0};  // last pixel fully transparent
  Badge badge;
  badge.image = {2, 2, px, alpha};
  badge.x = 20;
  badge.y = 30;
  MotorsportLook look(motorsportFonts(), GaugeProfile{}, badge);

  SoftCanvas canvas(240, 240);
  drawFrame(look, canvas, GaugeState{});
  CHECK(same(canvas.image().get(20, 30), kWhite));
  CHECK(same(canvas.image().get(21, 31), MotorsportLook::kBackground));  // transparent corner
}

TEST_CASE("motorsport look: a bar profile sizes the bar, zones and shift light in bar") {
  const GaugeProfile profile = GaugeProfile::bar();
  const GaugeScale scale = profile.scale();
  MotorsportLook look(motorsportFonts(), profile);

  CHECK(look.segmentCount() == 27);                          // 2.7 bar in 0.1 bar steps
  CHECK(same(look.segmentColor(0), Color{59, 139, 255}));    // vacuum
  CHECK(same(look.segmentColor(10), Color{25, 211, 162}));   // 0.05 bar
  CHECK(same(look.segmentColor(17), Color{155, 224, 74}));   // 0.75 bar: past two thirds of amber
  CHECK(same(look.segmentColor(20), Color{255, 176, 32}));   // 1.05 bar: amber
  CHECK(same(look.segmentColor(24), MotorsportLook::kRed));  // 1.45 bar: red
  CHECK(look.segmentFor(0.05f) == 10);
  CHECK(look.isLit(10, 0.06f));
  CHECK_FALSE(look.isLit(10, 0.04f));

  // 10 psi is 0.69 bar: segments up to the one centred on 0.65 bar are lit.
  const auto onSegment = [&](const SoftCanvas& canvas, int i) {
    return pointOnNeedle(canvas, -1.0f + (static_cast<float>(i) + 0.5f) * 0.1f, 99.0f, scale);
  };
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 10.0f;
  drawFrame(look, canvas, state);
  CHECK(same(onSegment(canvas, 0), look.segmentColor(0)));
  CHECK(same(onSegment(canvas, 16), look.segmentColor(16)));
  CHECK(same(onSegment(canvas, 17), MotorsportLook::kSegmentOff));

  // The rim pulses past 1.4 bar (20.3 psi), not before.
  const auto rim = [&](float psi) {
    SoftCanvas c(240, 240);
    GaugeState s;
    s.linked = true;
    s.psi = psi;
    s.timeMs = 141;  // the pulse at its brightest
    drawFrame(look, c, s);
    return c.image().get(120, 3);
  };
  CHECK(same(rim(21.0f), MotorsportLook::kRed));
  CHECK(same(rim(18.0f), MotorsportLook::kRimOff));
}

TEST_CASE("motorsport look: the segment count stays sensible for any range") {
  const GaugeProfile huge{Unit::Psi, 0.0f, 100000.0f, 5.0f, 20.0f, 25.0f};
  const GaugeProfile tiny{Unit::Psi, 0.0f, 0.0001f, 5.0f, 20.0f, 25.0f};
  const MotorsportLook many(motorsportFonts(), huge);
  const MotorsportLook one(motorsportFonts(), tiny);
  CHECK(many.segmentCount() == 200);
  CHECK(one.segmentCount() == 1);
  CHECK(one.segmentFor(50.0f) == 0);
  CHECK(many.segmentFor(-5.0f) == 0);
}
