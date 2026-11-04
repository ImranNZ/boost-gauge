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

/// The "Motorsport" look: a segmented bar that lights up through blue, teal, lime, amber and
/// red, with a bold readout and a rim that pulses red past the red zone like a shift light.
/// Drawn entirely in code. The scale, zones, segment size and unit come from a GaugeProfile
/// (one segment per psi, 40 across the default).
class MotorsportLook : public ILook {
 public:
  static constexpr Color kBackground{6, 7, 10};
  static constexpr Color kSegmentOff{27, 32, 38};
  static constexpr Color kRimOff{42, 47, 54};
  static constexpr Color kRed{255, 59, 48};
  static constexpr Color kAmber{255, 176, 32};

  /// The typefaces it needs: a large bold (slanted) one for the number, a small one for
  /// the scale labels and a slightly larger one for BOOST, PSI and the peak. All three
  /// need capitals, digits, "-", "+", "." and a space.
  struct Fonts {
    Font number;
    Font scale;
    Font caption;
  };

  explicit MotorsportLook(const Fonts& fonts, const GaugeProfile& profile = {});
  MotorsportLook(const Fonts& fonts, const GaugeProfile& profile, const Badge& badge);

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

  /// How many segments make up the bar.
  int segmentCount() const { return segments_; }
  /// Colour of a lit segment (index 0 is the bottom of the scale): blue for vacuum, then teal,
  /// lime, amber and red.
  Color segmentColor(int index) const;
  /// Whether segment `index` is lit at this number (in the profile's unit).
  bool isLit(int index, float value) const;
  /// Index of the segment a number falls in, clamped to the bar.
  int segmentFor(float value) const;

 private:
  void drawSegment(IDrawSurface& c, int index, float inset, Color color) const;
  float segmentStartDeg(int index) const;
  void drawBar(IDrawSurface& c, float value) const;
  void drawPeak(IDrawSurface& c, float peakValue, float value) const;
  void drawRim(IDrawSurface& c, const GaugeState& state) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  Fonts fonts_;
  GaugeProfile profile_;
  int segments_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
