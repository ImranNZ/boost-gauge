#pragma once

#include "gauge/GaugeProfile.h"
#include "gauge/GaugeScale.h"
#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/IDrawSurface.h"

namespace gauge {

/// The "Classic" look: a dark dial drawn entirely in code, with coloured zones
/// and a big digital readout. Pure presentation; knows nothing about OBD or hardware. The
/// scale, zones and unit come from a GaugeProfile.
class ClassicLook : public ILook {
 public:
  static constexpr Color kBackground{0, 0, 0};
  static constexpr Color kNeedle{255, 74, 28};
  static constexpr float kStartDeg = GaugeScale::kStartDeg;
  static constexpr float kSweepDeg = GaugeScale::kSweepDeg;

  /// A minor graduation between each pair of labelled ones.
  static constexpr int kMinorPerMajor = 2;

  /// The typefaces it needs: digits for the big readout; capitals, digits, "-", "+" and "." for the
  /// scale numbers, labels and the small frame-rate text.
  struct Fonts {
    Font number;
    Font label;
    Font small;
  };

  explicit ClassicLook(const Fonts& fonts, const GaugeProfile& profile = {})
      : fonts_(fonts), profile_(profile) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// Screen angle (degrees, 0 = 3 o'clock, clockwise) for a number in the profile's unit.
  float angleFor(float value) const;

 private:
  void drawZones(IDrawSurface& c, int cx, int cy) const;
  void drawScale(IDrawSurface& c, int cx, int cy) const;
  void drawPeakMarker(IDrawSurface& c, int cx, int cy, float peakValue) const;
  void drawNeedle(IDrawSurface& c, int cx, int cy, float value) const;
  void drawReadout(IDrawSurface& c, int cx, const GaugeState& state) const;

  Fonts fonts_;
  GaugeProfile profile_;
};

}  // namespace gauge
