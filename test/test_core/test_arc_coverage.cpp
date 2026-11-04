// Anti-aliased arc coverage maths.

#include "test_support.h"

using namespace support;

TEST_CASE("wrapDegrees maps any angle into [0, 360)") {
  CHECK(wrapDegrees(0.0f) == doctest::Approx(0.0f));
  CHECK(wrapDegrees(370.0f) == doctest::Approx(10.0f));
  CHECK(wrapDegrees(-90.0f) == doctest::Approx(270.0f));
  CHECK(wrapDegrees(720.0f) == doctest::Approx(0.0f));
}

TEST_CASE("arcCoverage: radial edges ramp over one pixel") {
  // Ring 10..20, full circle sweep, pixels along +x (angle 0).
  const auto at = [](float r) { return arcCoverage(r, 0.0f, 10.0f, 20.0f, 0.0f, 359.9f); };
  CHECK(at(15.0f) == doctest::Approx(1.0f));    // middle of the ring
  CHECK(at(10.0f) == doctest::Approx(0.5f));    // on the inner edge
  CHECK(at(20.0f) == doctest::Approx(0.5f));    // on the outer edge
  CHECK(at(9.4f) == doctest::Approx(0.0f));     // well inside the hole
  CHECK(at(20.6f) == doctest::Approx(0.0f));    // well outside
  CHECK(at(10.25f) == doctest::Approx(0.75f));  // partway up the ramp
  CHECK(at(20.5f) == 0.0f);                     // exactly where the outer fade ends
  CHECK(at(9.5f) == 0.0f);                      // exactly where the inner fade ends
}

TEST_CASE("arcCoverage: centre pixel of a disc (inner radius 0) is covered") {
  CHECK(arcCoverage(0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 359.9f) == doctest::Approx(1.0f));
}

TEST_CASE("arcCoverage: angular limits are hard, including wrap-around") {
  // 90 degree sweep starting at 270 (12 o'clock) running clockwise to 3 o'clock.
  const auto at = [](float dx, float dy) {
    return arcCoverage(dx, dy, 10.0f, 20.0f, 270.0f, 90.0f);
  };
  CHECK(at(10.0f, -10.0f) > 0.0f);    // top-right: inside
  CHECK(at(-10.0f, -10.0f) == 0.0f);  // top-left: outside
  CHECK(at(10.0f, 10.0f) == 0.0f);    // bottom-right: outside

  // Sweep that crosses 0 degrees: start 350, sweep 20.
  CHECK(arcCoverage(15.0f, 0.0f, 10.0f, 20.0f, 350.0f, 20.0f) > 0.0f);
  CHECK(arcCoverage(0.0f, 15.0f, 10.0f, 20.0f, 350.0f, 20.0f) == 0.0f);
}
