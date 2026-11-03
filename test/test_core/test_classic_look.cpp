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
