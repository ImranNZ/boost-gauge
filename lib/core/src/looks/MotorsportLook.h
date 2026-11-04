#pragma once

#include <optional>

#include "render/Badge.h"
#include "render/Color.h"
#include "render/Font.h"
#include "gauge/GaugeScale.h"
#include "gauge/GaugeState.h"
#include "render/IDrawSurface.h"
#include "looks/ILook.h"

namespace gauge {

/// The "Motorsport" look: a segmented bar (one segment per psi) that lights up
/// through blue, teal, lime, amber and red, with a bold readout and a rim that
/// pulses red past 20 psi like a shift light. Drawn entirely in code.
class MotorsportLook : public ILook {
 public:
  static constexpr Color kBackground{6, 7, 10};
  static constexpr Color kSegmentOff{27, 32, 38};
  static constexpr Color kRimOff{42, 47, 54};
  static constexpr Color kRed{255, 59, 48};
  static constexpr Color kAmber{255, 176, 32};

  /// One segment per psi across the -15..25 scale.
  static constexpr int kSegments = 40;
  /// Pressure above which the rim pulses.
  static constexpr float kShiftPsi = 20.0f;

  /// The typefaces it needs: a large bold (slanted) one for the number, a small one for
  /// the scale labels and a slightly larger one for BOOST, PSI and the peak. All three
  /// need capitals, digits, "-", "+", "." and a space.
  struct Fonts {
    Font number;
    Font scale;
    Font caption;
  };

  explicit MotorsportLook(Fonts fonts) : fonts_(fonts) {}
  MotorsportLook(Fonts fonts, Badge badge) : fonts_(fonts), badge_(badge) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// Colour of a lit segment (index 0 = -15 psi): blue for vacuum, then teal, lime, amber, red.
  static Color segmentColor(int index);
  /// Whether segment `index` is lit at this pressure.
  static bool isLit(int index, float psi);
  /// Index of the segment a pressure falls in, clamped to the bar.
  static int segmentFor(float psi);

 private:
  void drawSegment(IDrawSurface& c, int index, float inset, Color color) const;
  void drawBar(IDrawSurface& c, float psi) const;
  void drawPeak(IDrawSurface& c, float peakPsi, float psi) const;
  void drawRim(IDrawSurface& c, const GaugeState& state) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  Fonts fonts_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
