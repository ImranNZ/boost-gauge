#pragma once

#include "gauge/GaugeScale.h"
#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "render/Color.h"
#include "render/IDrawSurface.h"

namespace gauge {

struct ClassicLookConfig {
  float minPsi = -15.0f;
  float maxPsi = 25.0f;
  float majorStepPsi = 5.0f;
  int minorPerMajor = 2;  // minor tick every majorStep / minorPerMajor
  float amberFromPsi = 15.0f;
  float redFromPsi = 20.0f;
};

/// The "Classic" look: a dark dial drawn entirely in code, with coloured zones
/// and a big digital readout. Pure presentation; knows nothing about OBD or hardware.
class ClassicLook : public ILook {
 public:
  static constexpr Color kBackground{0, 0, 0};
  static constexpr Color kNeedle{255, 74, 28};
  static constexpr float kStartDeg = GaugeScale::kStartDeg;
  static constexpr float kSweepDeg = GaugeScale::kSweepDeg;

  explicit ClassicLook(const ClassicLookConfig& config = {}) : config_(config) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// Screen angle (degrees, 0 = 3 o'clock, clockwise) for a pressure.
  float angleFor(float psi) const;

 private:
  void drawZones(IDrawSurface& c, int cx, int cy) const;
  void drawScale(IDrawSurface& c, int cx, int cy) const;
  void drawPeakMarker(IDrawSurface& c, int cx, int cy, float peakPsi) const;
  void drawNeedle(IDrawSurface& c, int cx, int cy, float psi) const;
  void drawReadout(IDrawSurface& c, int cx, const GaugeState& state) const;

  ClassicLookConfig config_;
};

}  // namespace gauge
