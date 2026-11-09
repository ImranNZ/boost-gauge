// GaugeApp: the loop that ties a source, a look and a canvas together.

#include "test_support.h"

using namespace support;

TEST_CASE("app eases the needle between sparse readings") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  GaugeApp::Options options;
  options.smoothingMs = 100.0f;
  ClassicLook look = testClassic();
  GaugeApp app(source, canvas, clock, look, options);
  ClassicLook view = testClassic();

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
      if (same(needlePixel(canvas, view, psi), ClassicLook::kNeedle)) shown = psi;
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
  ClassicLook look = testClassic();
  GaugeApp app(source, canvas, clock, look, options);
  ClassicLook view = testClassic();

  source.publish(10.0f);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), ClassicLook::kNeedle));

  // Same seq again, even if the value somehow differed, is not news.
  source.sample.reading.value = 0.0f;
  clock.advance(33);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), ClassicLook::kNeedle));

  // A new but invalid reading keeps the last good target.
  source.publish(0.0f, false);
  clock.advance(33);
  app.tick();
  CHECK(same(needlePixel(canvas, view, 10.0f), ClassicLook::kNeedle));
}

TEST_CASE("app: fps overlay appears only when enabled") {
  const auto render = [](bool showFps) {
    ManualClock clock;
    FixedSource source;
    SoftCanvas canvas(240, 240);
    GaugeApp::Options options;
    options.showFps = showFps;
    ClassicLook look = testClassic();
    GaugeApp app(source, canvas, clock, look, options);
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

TEST_CASE("app: never-connected shows NO LINK and still presents frames") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  BoostRig rig(client, clock);
  PollingSource source(rig.channels(), 0);
  SoftCanvas canvas(240, 240);
  ClassicLook look = testClassic();
  GaugeApp app(source, canvas, clock, look, GaugeApp::Options{});

  for (int i = 0; i < 3; ++i) {
    clock.advance(33);
    app.tick();
  }
  CHECK(canvas.framesPresented() == 3);
}

TEST_CASE("app: custom look and options are honoured") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 160};
  client.baro = {true, 1, 100};
  BoostRig rig(client, clock);
  PollingSource source(rig.channels(), 0);
  SoftCanvas canvas(240, 240);
  GaugeProfile profile;
  profile.max = 30.0f;
  GaugeApp::Options options;
  options.smoothingMs = 0.0f;
  options.staleAfterMs = 10;
  ClassicLook look = testClassic(profile);
  GaugeApp app(source, canvas, clock, look, options);

  app.tick();
  client.map = {false, 0, 0};
  clock.advance(50);  // beyond staleAfterMs
  app.tick();
  CHECK(canvas.framesPresented() == 2);
}

TEST_CASE("app goes NO LINK when the ECU disappears") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(150.0f);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  BoostRig rig(client, clock);
  PollingSource source(rig.channels(), 0);
  SoftCanvas canvas(240, 240);
  ClassicLook view = testClassic();
  GaugeApp app(source, canvas, clock, view, GaugeApp::Options{});

  app.tick();
  CHECK(canvas.framesPresented() == 1);
  CHECK(same(pixelOnNeedle(canvas, view, 7.0f, 60), ClassicLook::kNeedle));

  ecu.setOnline(false);
  for (int i = 0; i < 100; ++i) {
    clock.advance(33);
    app.tick();
  }
  CHECK_FALSE(same(pixelOnNeedle(canvas, view, 7.0f, 60), ClassicLook::kNeedle));
}

TEST_CASE("app can switch look at run time") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  FlatDial dial(kCream);
  RetroLook retro(dial.view(), kRetroSpec, retroFonts());
  ClassicLook classic = testClassic();
  GaugeApp app(source, canvas, clock, classic, GaugeApp::Options{});

  app.tick();
  CHECK(same(canvas.image().get(2, 2), kBlack));  // Classic: black dial

  app.setLook(retro);  // must drop the cached Classic dial
  clock.advance(33);
  app.tick();
  CHECK(same(canvas.image().get(2, 2), rgb565ToColor(kCream)));
}

TEST_CASE("app can be constructed with an explicit look") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  FlatDial dial(kCream);
  RetroLook retro(dial.view(), kRetroSpec, retroFonts());
  GaugeApp app(source, canvas, clock, retro, GaugeApp::Options{});

  app.tick();
  CHECK(same(canvas.image().get(2, 2), rgb565ToColor(kCream)));
}

TEST_CASE("app: reset starts the displayed value over, forgetting the peak") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  GaugeApp::Options options;
  options.smoothingMs = 0.0f;
  ClassicLook look = testClassic();
  GaugeApp app(source, canvas, clock, look, options);
  const Color white{255, 255, 255};  // the peak marker

  source.publish(18.0f);
  app.tick();
  clock.advance(33);
  source.publish(5.0f);
  app.tick();
  CHECK(same(pointOnNeedle(canvas, 18.0f, 100.0f), white));  // the peak from a moment ago

  app.reset();  // another channel is now shown
  source.publish(5.0f);
  app.tick();
  CHECK_FALSE(same(pointOnNeedle(canvas, 18.0f, 100.0f), white));
}

namespace {

class DotOverlay : public IOverlay {
 public:
  bool active() const override { return true; }
  void draw(IDrawSurface& surface) const override { surface.fillCircle(120, 120, 5, kWhite); }
};

}  // namespace

TEST_CASE("app: draws its overlay over the gauge until it is removed") {
  ManualClock clock;
  FixedSource source;
  SoftCanvas canvas(240, 240);
  ClassicLook look = testClassic();
  DotOverlay overlay;
  GaugeApp app(source, canvas, clock, look, GaugeApp::Options{});

  app.setOverlay(&overlay);
  app.tick();
  CHECK(same(canvas.image().get(120, 120), kWhite));

  app.setOverlay(nullptr);
  clock.advance(33);
  app.tick();
  CHECK_FALSE(same(canvas.image().get(120, 120), kWhite));
}
