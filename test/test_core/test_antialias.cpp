// Anti-aliasing, wedge strokes, arc coverage maths and the cached dial layer.

#include <doctest.h>

#include <cmath>

#include "ArcCoverage.h"
#include "GaugeView.h"
#include "SoftCanvas.h"

using namespace gauge;

namespace {

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
bool partial(Color c) { return c.r > 0 && c.r < 255; }

int columnHeight(const SoftCanvas& canvas, int x) {
  int n = 0;
  for (int y = 0; y < canvas.height(); ++y)
    if (!same(canvas.image().get(x, y), kBlack)) ++n;
  return n;
}

}  // namespace

// --------------------------------------------------------------- coverage --

TEST_CASE("wrapDegrees maps any angle into [0, 360)") {
  CHECK(wrapDegrees(0.0f) == doctest::Approx(0.0f));
  CHECK(wrapDegrees(370.0f) == doctest::Approx(10.0f));
  CHECK(wrapDegrees(-90.0f) == doctest::Approx(270.0f));
  CHECK(wrapDegrees(720.0f) == doctest::Approx(0.0f));
}

TEST_CASE("arcCoverage: radial edges ramp over one pixel") {
  // Ring 10..20, full circle sweep, pixels along +x (angle 0).
  const auto at = [](float r) { return arcCoverage(r, 0.0f, 10.0f, 20.0f, 0.0f, 359.9f); };
  CHECK(at(15.0f) == doctest::Approx(1.0f));   // middle of the ring
  CHECK(at(10.0f) == doctest::Approx(0.5f));   // on the inner edge
  CHECK(at(20.0f) == doctest::Approx(0.5f));   // on the outer edge
  CHECK(at(9.4f) == doctest::Approx(0.0f));    // well inside the hole
  CHECK(at(20.6f) == doctest::Approx(0.0f));   // well outside
  CHECK(at(10.25f) == doctest::Approx(0.75f)); // partway up the ramp
  CHECK(at(20.5f) == 0.0f);                    // exactly where the outer fade ends
  CHECK(at(9.5f) == 0.0f);                     // exactly where the inner fade ends
}

TEST_CASE("arcCoverage: centre pixel of a disc (inner radius 0) is covered") {
  CHECK(arcCoverage(0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 359.9f) == doctest::Approx(1.0f));
}

TEST_CASE("arcCoverage: angular limits are hard, including wrap-around") {
  // 90 degree sweep starting at 270 (12 o'clock) running clockwise to 3 o'clock.
  const auto at = [](float dx, float dy) { return arcCoverage(dx, dy, 10.0f, 20.0f, 270.0f, 90.0f); };
  CHECK(at(10.0f, -10.0f) > 0.0f);   // top-right: inside
  CHECK(at(-10.0f, -10.0f) == 0.0f); // top-left: outside
  CHECK(at(10.0f, 10.0f) == 0.0f);   // bottom-right: outside

  // Sweep that crosses 0 degrees: start 350, sweep 20.
  CHECK(arcCoverage(15.0f, 0.0f, 10.0f, 20.0f, 350.0f, 20.0f) > 0.0f);
  CHECK(arcCoverage(0.0f, 15.0f, 10.0f, 20.0f, 350.0f, 20.0f) == 0.0f);
}

// ----------------------------------------------------------------- wedge ---

TEST_CASE("softcanvas wedge: tapers from the base radius to the tip radius") {
  SoftCanvas c(100, 40);
  c.drawWedge(10.0f, 20.0f, 90.0f, 20.0f, 6.0f, 1.0f, kWhite);

  const int nearBase = columnHeight(c, 14);
  const int nearTip = columnHeight(c, 86);
  CHECK(nearBase > nearTip);
  CHECK(nearTip >= 1);
  CHECK(same(c.image().get(50, 20), kWhite));
  CHECK(same(c.image().get(95, 20), kBlack));  // past the tip
}

TEST_CASE("softcanvas wedge: accepts sub-pixel coordinates") {
  SoftCanvas a(60, 60);
  SoftCanvas b(60, 60);
  a.drawWedge(10.0f, 30.0f, 50.0f, 30.0f, 2.0f, 2.0f, kWhite);
  b.drawWedge(10.0f, 30.4f, 50.0f, 30.4f, 2.0f, 2.0f, kWhite);  // 0.4 px lower
  CHECK(columnHeight(a, 30) > 0);
  CHECK(columnHeight(b, 30) > 0);
}

// ----------------------------------------------------------- supersampling --

TEST_CASE("supersampling: logical size is unchanged and interiors stay exact") {
  SoftCanvas c(40, 40, 4);
  CHECK(c.width() == 40);
  CHECK(c.height() == 40);
  CHECK(c.image().width() == 40);
  CHECK(c.image().height() == 40);

  c.fillCircle(20, 20, 10, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));  // fully covered: exact colour
  CHECK(same(c.image().get(2, 2), kBlack));    // untouched
}

TEST_CASE("supersampling: edges blend instead of stepping") {
  SoftCanvas hard(40, 40, 1);
  SoftCanvas smooth(40, 40, 4);
  hard.fillCircle(20, 20, 10, kWhite);
  smooth.fillCircle(20, 20, 10, kWhite);

  int hardPartial = 0, smoothPartial = 0;
  for (int y = 0; y < 40; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (partial(hard.image().get(x, y))) ++hardPartial;
      if (partial(smooth.image().get(x, y))) ++smoothPartial;
    }
  }
  CHECK(hardPartial == 0);
  CHECK(smoothPartial > 20);
}

TEST_CASE("supersampling: arcs, lines and text render at scale") {
  SoftCanvas c(60, 60, 3);
  c.fillArc(30, 30, 15, 25, 0.0f, 90.0f, kWhite);
  CHECK(same(c.image().get(30 + 20, 30 + 1), kWhite));   // inside the ring, near 3 o'clock
  CHECK(same(c.image().get(30 - 20, 30), kBlack));

  c.clear(kBlack);
  c.drawLine(5, 30, 55, 30, 4, kWhite);
  CHECK(same(c.image().get(30, 30), kWhite));

  c.clear(kBlack);
  c.drawText(10, 10, "0", 2, TextAlign::Left, kWhite);
  CHECK(columnHeight(c, 14) > 0);
}

TEST_CASE("supersampling: factor below 1 is treated as 1") {
  SoftCanvas c(10, 10, 0);
  c.fillCircle(5, 5, 2, kWhite);
  CHECK(same(c.image().get(5, 5), kWhite));
  CHECK(c.image().width() == 10);
}

// -------------------------------------------------------- cached background --

TEST_CASE("background: restore reports nothing saved, then restores exactly") {
  for (int k : {1, 4}) {
    SoftCanvas c(20, 20, k);
    CAPTURE(k);
    CHECK_FALSE(c.restoreBackground());

    c.clear(Color{10, 20, 30});
    c.saveBackground();
    c.clear(kWhite);
    CHECK(same(c.image().get(5, 5), kWhite));

    CHECK(c.restoreBackground());
    CHECK(same(c.image().get(5, 5), Color{10, 20, 30}));
    CHECK(c.restoreBackground());  // reusable every frame
  }
}

TEST_CASE("gauge view draws the dial once, then restores it") {
  GaugeView view;
  SoftCanvas c(240, 240);

  view.draw(c, {0.0f, 0.0f, false});  // first frame: draws and saves the dial
  const Color dialPixel = c.image().get(120, 8);  // on the top tick/zone ring
  CHECK_FALSE(same(dialPixel, kBlack));

  c.clear(kWhite);                     // wreck the frame
  view.draw(c, {0.0f, 0.0f, false});  // must restore, not redraw from scratch
  CHECK(same(c.image().get(120, 8), dialPixel));
  CHECK(same(c.image().get(0, 0), kBlack));  // dial background, not our white
}
