// GaugeModel: turns readings into the state the screen shows. No drawing involved.

#include "test_support.h"

using namespace support;

namespace {

Sample sample(float psi, uint16_t seq, bool valid = true) { return {{psi, valid}, seq}; }

GaugeModel::Options snapping() {
  GaugeModel::Options options;
  options.smoothingMs = 0.0f;  // no easing: the state equals the reading
  return options;
}

}  // namespace

TEST_CASE("gauge model: starts unlinked with nothing to show") {
  GaugeModel model;
  CHECK_FALSE(model.state().linked);
  CHECK(model.state().value == doctest::Approx(0.0f));
  CHECK(model.state().peakValue == doctest::Approx(0.0f));
  CHECK(model.state().fps == 0);
}

TEST_CASE("gauge model: the state carries the frame time") {
  GaugeModel model(snapping());
  model.update(sample(1.0f, 1), 1234);
  CHECK(model.state().timeMs == 1234);
}

TEST_CASE("gauge model: a valid reading links it and sets the value") {
  GaugeModel model(snapping());
  model.update(sample(8.0f, 1), 0);
  CHECK(model.state().linked);
  CHECK(model.state().value == doctest::Approx(8.0f));
}

TEST_CASE("gauge model: an invalid reading never links") {
  GaugeModel model(snapping());
  model.update(sample(8.0f, 1, false), 0);
  CHECK_FALSE(model.state().linked);
}

TEST_CASE("gauge model: a repeated sample (same seq) is not news") {
  GaugeModel model(snapping());
  model.update(sample(8.0f, 1), 0);
  model.update(sample(99.0f, 1), 100);  // same seq: ignored even though the value differs
  CHECK(model.state().value == doctest::Approx(8.0f));
}

TEST_CASE("gauge model: an invalid sample keeps the last good target") {
  GaugeModel model(snapping());
  model.update(sample(8.0f, 1), 0);
  model.update(sample(0.0f, 2, false), 100);
  CHECK(model.state().value == doctest::Approx(8.0f));
}

TEST_CASE("gauge model: goes stale after staleAfterMs without a valid reading") {
  GaugeModel::Options options = snapping();
  options.staleAfterMs = 1000;
  GaugeModel model(options);

  model.update(sample(5.0f, 1), 0);
  model.update(sample(5.0f, 1), 999);
  CHECK(model.state().linked);
  model.update(sample(5.0f, 1), 1000);
  CHECK_FALSE(model.state().linked);

  model.update(sample(6.0f, 2), 1100);  // a fresh reading brings it back
  CHECK(model.state().linked);
}

TEST_CASE("gauge model: eases toward the target between readings") {
  GaugeModel::Options options;
  options.smoothingMs = 100.0f;
  GaugeModel model(options);

  model.update(sample(0.0f, 1), 0);
  model.update(sample(10.0f, 2), 10);
  const float first = model.state().value;
  CHECK(first > 0.0f);
  CHECK(first < 10.0f);

  model.update(sample(10.0f, 2), 110);  // no new reading: it keeps closing in
  CHECK(model.state().value > first);
  CHECK(model.state().value < 10.0f);
}

TEST_CASE("gauge model: holds the peak, then lets it go") {
  GaugeModel::Options options = snapping();
  options.peakHoldMs = 1000;
  GaugeModel model(options);

  model.update(sample(12.0f, 1), 0);
  model.update(sample(4.0f, 2), 500);
  CHECK(model.state().peakValue == doctest::Approx(12.0f));  // held
  model.update(sample(4.0f, 2), 1500);
  CHECK(model.state().peakValue == doctest::Approx(4.0f));  // released
}

TEST_CASE("gauge model: reports fps only when asked to") {
  const auto fpsAfterTwoSeconds = [](bool show) {
    GaugeModel::Options options = snapping();
    options.showFps = show;
    GaugeModel model(options);
    for (uint32_t t = 0; t <= 2000; t += 20) model.update(sample(1.0f, 1), t);
    return model.state().fps;
  };
  CHECK(fpsAfterTwoSeconds(false) == 0);
  CHECK(fpsAfterTwoSeconds(true) == 50);
}
