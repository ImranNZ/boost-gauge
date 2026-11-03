#include <doctest.h>

#include <cmath>

#include "BoostSampler.h"
#include "CanObdClient.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "GaugeApp.h"
#include "GaugeView.h"
#include "ManualClock.h"
#include "Smoothing.h"
#include "SoftCanvas.h"

using namespace gauge;

namespace {

bool sameColor(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

Color pixelOnNeedle(const SoftCanvas& canvas, const GaugeView& view, float psi, int radius) {
  const float deg = view.angleFor(psi) * 3.14159265f / 180.0f;
  const int x = 120 + static_cast<int>(std::lround(std::cos(deg) * radius));
  const int y = 120 + static_cast<int>(std::lround(std::sin(deg) * radius));
  return canvas.image().get(x, y);
}

}  // namespace

TEST_CASE("gauge angles: min bottom-left, max bottom-right, mid at top") {
  GaugeView view;
  CHECK(view.angleFor(-15.0f) == doctest::Approx(135.0f));
  CHECK(view.angleFor(5.0f) == doctest::Approx(270.0f));
  CHECK(view.angleFor(25.0f) == doctest::Approx(405.0f));
  CHECK(view.angleFor(99.0f) == doctest::Approx(405.0f));  // clamped
}

TEST_CASE("needle is drawn along the angle for the value") {
  GaugeView view;
  for (float psi : {-12.0f, 0.0f, 8.0f, 18.0f}) {
    SoftCanvas canvas(240, 240);
    view.draw(canvas, {psi, 0.0f, true});
    CAPTURE(psi);
    CHECK(sameColor(pixelOnNeedle(canvas, view, psi, 60), GaugeView::kNeedle));
  }
}

TEST_CASE("no needle and no link banner when unlinked") {
  GaugeView view;
  SoftCanvas canvas(240, 240);
  view.draw(canvas, {10.0f, 0.0f, false});
  CHECK_FALSE(sameColor(pixelOnNeedle(canvas, view, 10.0f, 60), GaugeView::kNeedle));
}

TEST_CASE("peak hold releases after the hold time") {
  PeakHold peak(1000);
  CHECK(peak.update(5.0f, 0) == doctest::Approx(5.0f));
  CHECK(peak.update(3.0f, 500) == doctest::Approx(5.0f));
  CHECK(peak.update(3.0f, 1500) == doctest::Approx(3.0f));
}

TEST_CASE("app goes NO LINK when the ECU disappears") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(150.0f);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  BoostSampler sampler(client, clock);
  PollingSource source(sampler);
  SoftCanvas canvas(240, 240);
  GaugeView view;
  GaugeApp app(source, canvas, clock);

  app.tick();
  CHECK(canvas.framesPresented() == 1);
  CHECK(sameColor(pixelOnNeedle(canvas, view, 7.0f, 60), GaugeView::kNeedle));

  ecu.setOnline(false);
  for (int i = 0; i < 100; ++i) {
    clock.advance(33);
    app.tick();
  }
  CHECK_FALSE(sameColor(pixelOnNeedle(canvas, view, 7.0f, 60), GaugeView::kNeedle));
}
