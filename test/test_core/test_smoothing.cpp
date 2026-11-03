// Easing, peak hold and the fps counter.

#include "test_support.h"

using namespace support;

TEST_CASE("easer: first value snaps, then closes ~63% per time constant") {
  Easer easer(100.0f);
  CHECK(easer.update(10.0f, 0) == doctest::Approx(10.0f));  // first value: no lag

  CHECK(easer.update(0.0f, 100) == doctest::Approx(10.0f * std::exp(-1.0f)).epsilon(0.001));
  CHECK(easer.value() == doctest::Approx(3.679f).epsilon(0.001));
}

TEST_CASE("easer: result does not depend on how often it is stepped") {
  Easer coarse(100.0f);
  Easer fine(100.0f);
  coarse.update(10.0f, 0);
  fine.update(10.0f, 0);

  coarse.update(0.0f, 200);
  for (uint32_t t = 10; t <= 200; t += 10) fine.update(0.0f, t);
  CHECK(coarse.value() == doctest::Approx(fine.value()).epsilon(0.001));
}

TEST_CASE("easer: zero time constant disables easing") {
  Easer easer(0.0f);
  easer.update(3.0f, 0);
  CHECK(easer.update(9.0f, 10) == doctest::Approx(9.0f));
}

TEST_CASE("easer: repeated call at the same instant changes nothing") {
  Easer easer(50.0f);
  easer.update(5.0f, 100);
  easer.update(0.0f, 150);
  const float v = easer.value();
  CHECK(easer.update(0.0f, 150) == doctest::Approx(v));
}

TEST_CASE("peak hold: rises, ties, expires") {
  PeakHold peak(1000);
  CHECK(peak.update(2.0f, 0) == doctest::Approx(2.0f));
  CHECK(peak.update(4.0f, 100) == doctest::Approx(4.0f));
  CHECK(peak.update(4.0f, 200) == doctest::Approx(4.0f));  // tie refreshes the timer
  CHECK(peak.update(1.0f, 1100) == doctest::Approx(4.0f));
  CHECK(peak.update(1.0f, 1200) == doctest::Approx(1.0f));
  CHECK(peak.value() == doctest::Approx(1.0f));
}

TEST_CASE("peak hold releases after the hold time") {
  PeakHold peak(1000);
  CHECK(peak.update(5.0f, 0) == doctest::Approx(5.0f));
  CHECK(peak.update(3.0f, 500) == doctest::Approx(5.0f));
  CHECK(peak.update(3.0f, 1500) == doctest::Approx(3.0f));
}

TEST_CASE("fps counter averages over a window") {
  FpsCounter fps(1000);
  CHECK(fps.fps() == doctest::Approx(0.0f));  // nothing yet

  for (uint32_t t = 0; t < 1000; t += 20) fps.frame(t);  // 50 frames, window not closed
  CHECK(fps.fps() == doctest::Approx(0.0f));

  fps.frame(1000);  // closes the first window: 51 frames in 1000 ms
  CHECK(fps.fps() == doctest::Approx(51.0f));

  for (uint32_t t = 1020; t <= 1500; t += 20) fps.frame(t);
  CHECK(fps.fps() == doctest::Approx(51.0f));  // holds the last full window
}
