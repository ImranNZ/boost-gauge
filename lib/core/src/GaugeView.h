#pragma once

#include "GaugeScale.h"
#include "GaugeState.h"
#include "ICanvas.h"
#include "ILook.h"

namespace gauge {

struct GaugeConfig {
  float minPsi = -15.0f;
  float maxPsi = 25.0f;
  float majorStepPsi = 5.0f;
  int minorPerMajor = 2;       // minor tick every majorStep / minorPerMajor
  float amberFromPsi = 15.0f;
  float redFromPsi = 20.0f;
};

/// The "Classic" look: a dark dial drawn entirely in code, with coloured zones
/// and a big digital readout. Pure presentation; knows nothing about OBD or hardware.
class GaugeView : public DialCachingLook {
 public:
  static constexpr Color kBackground{0, 0, 0};
  static constexpr Color kNeedle{255, 74, 28};
  static constexpr float kStartDeg = GaugeScale::kStartDeg;
  static constexpr float kSweepDeg = GaugeScale::kSweepDeg;

  explicit GaugeView(GaugeConfig config = {}) : config_(config) {}

  /// Screen angle (degrees, 0 = 3 o'clock, clockwise) for a pressure.
  float angleFor(float psi) const;

 protected:
  void drawDial(ICanvas& c) const override;
  void drawDynamic(ICanvas& c, const GaugeState& state) const override;

 private:
  void drawZones(ICanvas& c, int cx, int cy) const;
  void drawScale(ICanvas& c, int cx, int cy) const;
  void drawPeakMarker(ICanvas& c, int cx, int cy, float peakPsi) const;
  void drawNeedle(ICanvas& c, int cx, int cy, float psi) const;
  void drawReadout(ICanvas& c, int cx, const GaugeState& state) const;

  GaugeConfig config_;
};

}  // namespace gauge
