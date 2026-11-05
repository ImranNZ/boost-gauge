// FrameRenderer: caching the static dial, drawing the moving parts, presenting.

#include "test_support.h"

using namespace support;

namespace {

/// A look that records what it was asked to do.
class RecordingLook : public ILook {
 public:
  void drawDial(IDrawSurface& surface) const override {
    ++dialDraws;
    surface.clear(Color{10, 20, 30});
  }
  void drawDynamic(IDrawSurface& surface, const GaugeState& state) const override {
    ++dynamicDraws;
    lastState = state;
    surface.fillCircle(5, 5, 1, kWhite);
  }

  mutable int dialDraws = 0;
  mutable int dynamicDraws = 0;
  mutable GaugeState lastState;
};

}  // namespace

TEST_CASE("frame renderer: draws the dial once, then restores it from the cache") {
  SoftCanvas canvas(20, 20);
  FrameRenderer renderer(canvas);
  RecordingLook look;

  renderer.render(look, GaugeState{});
  CHECK(look.dialDraws == 1);
  CHECK(look.dynamicDraws == 1);

  canvas.clear(kWhite);  // wreck the frame: the cached dial must come back
  renderer.render(look, GaugeState{});
  CHECK(look.dialDraws == 1);  // not redrawn
  CHECK(look.dynamicDraws == 2);
  CHECK(same(canvas.image().get(15, 15), Color{10, 20, 30}));
}

TEST_CASE("frame renderer: passes the state to the look and presents every frame") {
  SoftCanvas canvas(20, 20);
  FrameRenderer renderer(canvas);
  RecordingLook look;

  GaugeState state;
  state.value = 7.5f;
  state.linked = true;
  renderer.render(look, state);
  CHECK(look.lastState.value == doctest::Approx(7.5f));
  CHECK(look.lastState.linked);

  renderer.render(look, state);
  CHECK(canvas.framesPresented() == 2);
}

TEST_CASE("frame renderer: invalidate makes the next frame redraw the dial") {
  SoftCanvas canvas(20, 20);
  FrameRenderer renderer(canvas);
  RecordingLook look;

  renderer.render(look, GaugeState{});
  renderer.invalidate();
  renderer.render(look, GaugeState{});
  CHECK(look.dialDraws == 2);
}

TEST_CASE("frame renderer: a look is only ever given a drawing surface, not the cache") {
  // The look's interface takes IDrawSurface&. This is a compile-time property; the
  // test documents it by passing a surface that has no cache at all.
  struct SurfaceOnly : IDrawSurface {
    int width() const override { return 4; }
    int height() const override { return 4; }
    void clear(Color) override {}
    void fillCircle(int, int, int, Color) override {}
    void fillArc(int, int, int, int, float, float, Color) override {}
    void drawWedge(float, float, float, float, float, float, Color) override {}
    void drawLine(int, int, int, int, int, Color) override {}
    void drawText(int, int, const char*, int, TextAlign, Color) override {}
    void drawImage(const ImageView&, int, int) override {}
    void drawMask(const MaskView&, float, float, Color) override {}
  } surface;

  RecordingLook look;
  look.drawDial(surface);
  look.drawDynamic(surface, GaugeState{});
  CHECK(look.dialDraws == 1);
  CHECK(look.dynamicDraws == 1);
}

namespace {

/// An overlay that paints a white disc in the middle when active.
class DiscOverlay : public IOverlay {
 public:
  bool active() const override { return on; }
  void draw(IDrawSurface& surface) const override {
    surface.fillCircle(120, 120, 60, kWhite);
  }  // design space: the middle
  bool on = true;
};

}  // namespace

TEST_CASE("frame renderer: an active overlay is drawn over the look") {
  SoftCanvas canvas(20, 20);
  FrameRenderer renderer(canvas);
  RecordingLook look;
  DiscOverlay overlay;

  renderer.render(look, GaugeState{}, &overlay);
  CHECK(same(canvas.image().get(10, 10), kWhite));
  CHECK(look.dynamicDraws == 1);  // the look still draws underneath
}

TEST_CASE("frame renderer: an inactive overlay, or none, draws nothing extra") {
  SoftCanvas canvas(20, 20);
  FrameRenderer renderer(canvas);
  RecordingLook look;
  DiscOverlay overlay;
  overlay.on = false;

  renderer.render(look, GaugeState{}, &overlay);
  CHECK(same(canvas.image().get(10, 10), Color{10, 20, 30}));  // just the dial
  renderer.render(look, GaugeState{}, nullptr);
  CHECK(same(canvas.image().get(10, 10), Color{10, 20, 30}));
}
