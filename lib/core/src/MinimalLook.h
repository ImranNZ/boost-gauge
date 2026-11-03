#pragma once

#include <optional>

#include "Badge.h"
#include "Color.h"
#include "GaugeScale.h"
#include "GaugeState.h"
#include "IDrawSurface.h"
#include "ILook.h"

namespace gauge {

/// The "Minimal" look: no needle. A thin arc grows out from zero (blue for vacuum,
/// a green-to-red heat ramp for boost) around a large digital readout. Drawn
/// entirely in code from IDrawSurface primitives.
class MinimalLook : public ILook {
 public:
  static constexpr Color kBackground{5, 7, 10};
  static constexpr Color kTrack{27, 33, 40};

  MinimalLook() = default;
  explicit MinimalLook(Badge badge) : badge_(badge) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// The arc's colour at a pressure: blue below zero, then teal, lime, amber, red.
  static Color colorFor(float psi);

 private:
  void drawArc(IDrawSurface& c, float psi) const;
  void drawPeak(IDrawSurface& c, float peakPsi) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  GaugeScale scale_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
