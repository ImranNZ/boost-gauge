// The Classic look.

#include "test_support.h"

using namespace support;

TEST_CASE("gauge angles: min bottom-left, max bottom-right, mid at top") {
  ClassicLook view;
  CHECK(view.angleFor(-15.0f) == doctest::Approx(135.0f));
  CHECK(view.angleFor(5.0f) == doctest::Approx(270.0f));
  CHECK(view.angleFor(25.0f) == doctest::Approx(405.0f));
  CHECK(view.angleFor(99.0f) == doctest::Approx(405.0f));  // clamped
}

TEST_CASE("needle is drawn along the angle for the value") {
  ClassicLook view;
  for (float psi : {-12.0f, 0.0f, 8.0f, 18.0f}) {
    SoftCanvas canvas(240, 240);
    drawFrame(view, canvas, {psi, 0.0f, true});
    CAPTURE(psi);
    CHECK(same(pixelOnNeedle(canvas, view, psi, 60), ClassicLook::kNeedle));
  }
}

TEST_CASE("no needle and no link banner when unlinked") {
  ClassicLook view;
  SoftCanvas canvas(240, 240);
  drawFrame(view, canvas, {10.0f, 0.0f, false});
  CHECK_FALSE(same(pixelOnNeedle(canvas, view, 10.0f, 60), ClassicLook::kNeedle));
}

TEST_CASE("classic look draws the fps label only for a positive value") {
  ClassicLook view;
  const auto lit = [&view](int fps) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.fps = fps;
    drawFrame(view, canvas, state);
    int n = 0;
    for (int x = 90; x < 150; ++x)
      for (int y = 224; y < 232; ++y)
        if (!same(canvas.image().get(x, y), kBlack)) ++n;
    return n;
  };
  CHECK(lit(0) == 0);
  CHECK(lit(60) > 0);
}

TEST_CASE("classic look: a bar profile moves the needle and the zones with it") {
  const GaugeProfile profile = GaugeProfile::bar();
  ClassicLook look(profile);
  SoftCanvas canvas(240, 240);
  GaugeState state;
  state.linked = true;
  state.psi = 10.0f;  // 0.69 bar
  drawFrame(look, canvas, state);

  CHECK(same(pixelOnNeedle(canvas, look, profile.toDisplay(10.0f), 60), ClassicLook::kNeedle));
  // The amber zone runs 1.0 to 1.4 bar and the red zone starts there (the zone ring is 112-119 px
  // out).
  CHECK(same(pixelOnNeedle(canvas, look, 1.2f, 115), Color{255, 176, 0}));
  CHECK(same(pixelOnNeedle(canvas, look, 1.6f, 115), Color{255, 48, 48}));
}

TEST_CASE("classic look: the unit label follows the profile") {
  const auto draw = [](const GaugeProfile& profile) {
    ClassicLook look(profile);
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.linked = true;
    state.psi = 10.0f;
    state.peakPsi = 12.0f;
    drawFrame(look, canvas, state);
    return canvas;
  };
  const auto differing = [](const SoftCanvas& a, const SoftCanvas& b) {
    int n = 0;
    for (int y = 166; y < 186; ++y)
      for (int x = 80; x < 160; ++x)
        if (!same(a.image().get(x, y), b.image().get(x, y))) ++n;
    return n;
  };
  const SoftCanvas psi = draw(GaugeProfile::psi());
  const SoftCanvas bar = draw(GaugeProfile::bar());
  const SoftCanvas kpa = draw(GaugeProfile::kpa());
  CHECK(differing(psi, bar) > 0);  // "PSI" vs "BAR" in the unit row
  CHECK(differing(bar, kpa) > 0);  // "BAR" vs "KPA"
}
