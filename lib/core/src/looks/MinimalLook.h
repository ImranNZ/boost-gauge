#pragma once

#include <optional>

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
/// entirely in code from IDrawSurface primitives.
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

  explicit MinimalLook(const Fonts& fonts) : fonts_(fonts) {}
  MinimalLook(const Fonts& fonts, const Badge& badge) : fonts_(fonts), badge_(badge) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// The arc's colour at a pressure: blue below zero, then teal, lime, amber, red.
  static Color colorFor(float psi);

 private:
  void drawArc(IDrawSurface& c, float psi) const;
  void drawPeak(IDrawSurface& c, float peakPsi) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  Fonts fonts_;
  GaugeScale scale_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
