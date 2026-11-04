#pragma once

#include <optional>

#include "gauge/GaugeProfile.h"
#include "gauge/GaugeScale.h"
#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "render/Badge.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/IDrawSurface.h"

namespace gauge {

/// The "Minimal" look: no needle. A thin arc grows out from zero (blue for vacuum,
/// a green-to-red heat ramp for boost) around a large digital readout. Drawn
/// entirely in code from IDrawSurface primitives. The scale, zones and unit come from a
/// GaugeProfile.
class MinimalLook : public ILook {
 public:
  static constexpr Color kBackground{5, 7, 10};
  static constexpr Color kTrack{27, 33, 40};

  /// The two typefaces it needs: a large light one for the number, and a small
  /// medium-weight one for the labels (capitals, digits, "-" and ".").
  struct Fonts {
    Font number;
    Font label;
  };

  explicit MinimalLook(const Fonts& fonts, const GaugeProfile& profile = {})
      : fonts_(fonts), profile_(profile), scale_(profile.scale()) {}
  MinimalLook(const Fonts& fonts, const GaugeProfile& profile, const Badge& badge)
      : fonts_(fonts), profile_(profile), scale_(profile.scale()), badge_(badge) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// The arc's colour at a number in the profile's unit: blue below zero, then teal, lime,
  /// amber and red. The ramp is laid out relative to where the red zone starts.
  Color colorFor(float value) const;

 private:
  void drawArc(IDrawSurface& c, float psi) const;
  void drawPeak(IDrawSurface& c, float peakValue) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  Fonts fonts_;
  GaugeProfile profile_;
  GaugeScale scale_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
