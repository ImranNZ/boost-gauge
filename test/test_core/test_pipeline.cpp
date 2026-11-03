// The sampler -> mailbox -> screen pipeline: decoupling, easing and the fps counter.

#include <doctest.h>

#include <cmath>

#include "BoostSampler.h"
#include "FpsCounter.h"
#include "GaugeApp.h"
#include "ManualClock.h"
#include "ReadingMailbox.h"
#include "ReadingSource.h"
#include "SamplerLoop.h"
#include "Smoothing.h"
#include "SoftCanvas.h"
#include "StubObdClient.h"

using namespace gauge;

namespace {

constexpr Color kBlack{0, 0, 0};

bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

/// Source whose sample is set directly by the test.
class FixedSource : public IReadingSource {
 public:
  Sample latest() override { return sample; }
  void publish(float psi, bool valid = true) { sample = {{psi, valid}, ++seq}; }
  Sample sample;
  uint16_t seq = 0;
};

/// The needle tip pixel row/col at a given psi, to read back where it points.
Color needlePixel(const SoftCanvas& canvas, const GaugeView& view, float psi) {
  const float rad = view.angleFor(psi) * 3.14159265f / 180.0f;
  return canvas.image().get(120 + static_cast<int>(std::lround(std::cos(rad) * 60)),
                            120 + static_cast<int>(std::lround(std::sin(rad) * 60)));
}

}  // namespace

// ----------------------------------------------------------------- easer ---

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

// ------------------------------------------------------------------- fps ---

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

// --------------------------------------------------------------- mailbox ---

TEST_CASE("mailbox: empty until published, then carries value and a new seq") {
  ReadingMailbox mailbox;
  Sample empty = mailbox.latest();
  CHECK(empty.seq == 0);
  CHECK_FALSE(empty.reading.valid);

  mailbox.publish({12.34f, true});
  Sample a = mailbox.latest();
  CHECK(a.seq == 1);
  CHECK(a.reading.valid);
  CHECK(a.reading.psi == doctest::Approx(12.34f).epsilon(0.001));

  mailbox.publish({-9.87f, true});
  Sample b = mailbox.latest();
  CHECK(b.seq == 2);
  CHECK(b.reading.psi == doctest::Approx(-9.87f).epsilon(0.001));

  CHECK(mailbox.latest().seq == 2);  // reading it does not consume it
}

TEST_CASE("mailbox: invalid flag, clamping and sequence wrap") {
  ReadingMailbox mailbox;
  mailbox.publish({0.0f, false});
  CHECK_FALSE(mailbox.latest().reading.valid);

  mailbox.publish({1000.0f, true});  // beyond +-327.67: clamps rather than wrapping
  CHECK(mailbox.latest().reading.psi == doctest::Approx(327.67f).epsilon(0.001));
  mailbox.publish({-1000.0f, true});
  CHECK(mailbox.latest().reading.psi == doctest::Approx(-327.68f).epsilon(0.001));

  // 15-bit sequence wraps back through 0 without breaking the payload.
  for (int i = 0; i < 32768; ++i) mailbox.publish({5.0f, true});
  const Sample wrapped = mailbox.latest();
  CHECK(wrapped.seq == (3 + 32768) % 32768);
  CHECK(wrapped.reading.psi == doctest::Approx(5.0f));
}

TEST_CASE("polling source: every call is a fresh sample") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 150};
  client.baro = {true, 1, 100};
  BoostSampler sampler(client, clock);
  PollingSource source(sampler);

  const Sample a = source.latest();
  const Sample b = source.latest();
  CHECK(a.reading.valid);
  CHECK(b.seq == a.seq + 1);
}

TEST_CASE("sampler loop publishes each step into the mailbox") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 150};
  client.baro = {true, 1, 100};
  BoostSampler sampler(client, clock);
  ReadingMailbox mailbox;
  SamplerLoop loop(sampler, mailbox);

  CHECK(mailbox.latest().seq == 0);
  loop.step();
  CHECK(mailbox.latest().seq == 1);
  CHECK(mailbox.latest().reading.valid);
  CHECK(mailbox.latest().reading.psi == doctest::Approx(7.25f).epsilon(0.01));

  client.map = {false, 0, 0};
  loop.step();
  CHECK_FALSE(mailbox.latest().reading.valid);
}

// ------------------------------------------------------------- gauge app ---

TEST_CASE("app eases the needle between sparse readings") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  GaugeApp::Options options;
  options.smoothingMs = 100.0f;
  GaugeApp app(source, canvas, clock, GaugeView{}, options);
  GaugeView view;

  source.publish(0.0f);
  app.tick();  // snaps to 0

  source.publish(20.0f);  // one new reading, then a long gap with no more data
  float previous = -1.0f;
  int movedFrames = 0;
  for (int i = 0; i < 12; ++i) {
    clock.advance(33);
    app.tick();
    // Find where the needle is by probing candidate angles every 0.5 psi.
    float shown = -1000.0f;
    for (float psi = 0.0f; psi <= 20.0f; psi += 0.5f) {
      if (same(needlePixel(canvas, view, psi), GaugeView::kNeedle)) shown = psi;
    }
    if (shown > previous) ++movedFrames;
    previous = shown;
  }
  CHECK(movedFrames >= 6);  // kept moving across frames that had no new data
  CHECK(previous > 12.0f);  // and ended up well on its way to the target
}

TEST_CASE("app ignores repeated samples and invalid readings") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  GaugeApp::Options options;
  options.smoothingMs = 0.0f;
  GaugeApp app(source, canvas, clock, GaugeView{}, options);
  GaugeView view;

  source.publish(10.0f);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), GaugeView::kNeedle));

  // Same seq again, even if the value somehow differed, is not news.
  source.sample.reading.psi = 0.0f;
  clock.advance(33);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), GaugeView::kNeedle));

  // A new but invalid reading keeps the last good target.
  source.publish(0.0f, false);
  clock.advance(33);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), GaugeView::kNeedle));
}

TEST_CASE("app: fps overlay appears only when enabled") {
  const auto render = [](bool showFps) {
    ManualClock clock;
    FixedSource source;
    SoftCanvas canvas(240, 240);
    GaugeApp::Options options;
    options.showFps = showFps;
    GaugeApp app(source, canvas, clock, GaugeView{}, options);
    source.publish(5.0f);
    for (int i = 0; i < 40; ++i) {  // ~1.3 s at 33 ms: the first fps window completes
      clock.advance(33);
      app.tick();
    }
    int lit = 0;
    for (int x = 90; x < 150; ++x)
      for (int y = 224; y < 232; ++y)
        if (!same(canvas.image().get(x, y), kBlack)) ++lit;
    return lit;
  };
  CHECK(render(true) > 0);
  CHECK(render(false) == 0);
}

TEST_CASE("view draws the fps label only for a positive value") {
  GaugeView view;
  const auto lit = [&view](int fps) {
    SoftCanvas canvas(240, 240);
    GaugeState state;
    state.fps = fps;
    view.draw(canvas, state);
    int n = 0;
    for (int x = 90; x < 150; ++x)
      for (int y = 224; y < 232; ++y)
        if (!same(canvas.image().get(x, y), kBlack)) ++n;
    return n;
  };
  CHECK(lit(0) == 0);
  CHECK(lit(60) > 0);
}
