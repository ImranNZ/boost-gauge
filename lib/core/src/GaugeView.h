#pragma once

#include "ICanvas.h"

namespace gauge {

struct GaugeConfig {
  float minPsi = -15.0f;
  float maxPsi = 25.0f;
  float majorStepPsi = 5.0f;
  int minorPerMajor = 2;       // minor tick every majorStep / minorPerMajor
  float amberFromPsi = 15.0f;
  float redFromPsi = 20.0f;
};

/// What the view needs to draw one frame.
struct GaugeState {
  float psi = 0.0f;
  float peakPsi = 0.0f;
  bool linked = false;
};

/// Draws the round boost gauge: 270 degree sweep opening at the bottom.
/// Pure presentation; knows nothing about OBD or hardware.
class GaugeView {
 public:
  static constexpr Color kBackground{0, 0, 0};
  static constexpr Color kNeedle{255, 74, 28};
  static constexpr float kStartDeg = 135.0f;  // bottom-left
  static constexpr float kSweepDeg = 270.0f;

  explicit GaugeView(GaugeConfig config = {}) : config_(config) {}

  void draw(ICanvas& canvas, const GaugeState& state) const;

  /// Screen angle (degrees, 0 = 3 o'clock, clockwise) for a pressure.
  float angleFor(float psi) const;

 private:
  void drawDial(ICanvas& c, int cx, int cy) const;  // the static artwork, cached
  void drawZones(ICanvas& c, int cx, int cy) const;
  void drawScale(ICanvas& c, int cx, int cy) const;
  void drawPeakMarker(ICanvas& c, int cx, int cy, float peakPsi) const;
  void drawNeedle(ICanvas& c, int cx, int cy, float psi) const;
  void drawReadout(ICanvas& c, int cx, const GaugeState& state) const;

  GaugeConfig config_;
};

}  // namespace gauge
