#pragma once

#include "gauge/GaugeScale.h"
#include "obd/Units.h"

namespace gauge {

/// The unit a gauge shows boost in. The pipeline always carries psi; a look converts for display.
enum class Unit { Psi, Bar, Kpa };

/// Everything about a gauge that depends on the car and the owner rather than on the look: the
/// unit, the scale's range, and where the amber and red zones start. Ranges are in `unit`.
///
/// The default is the original psi gauge (-15 to +25 psi, amber from 15, red from 20), which
/// suits a stock turbo like the F56's. `bar()` and `kpa()` are metric equivalents. Change the
/// values for a car with more or less boost.
///
/// The code-drawn looks (Classic, Minimal, Motorsport) follow the profile. The pre-rendered
/// dials (Retro, Modern) are pictures of a psi scale, drawn from looks/<name>/spec.json.
struct GaugeProfile {
  Unit unit = Unit::Psi;
  float min = -15.0f;       // bottom of the scale (vacuum)
  float max = 25.0f;        // top of the scale
  float majorStep = 5.0f;   // labelled graduations
  float amberFrom = 15.0f;  // the amber zone starts here
  float redFrom = 20.0f;    // the red zone (and the Motorsport shift light) starts here

  static constexpr GaugeProfile psi() { return GaugeProfile{}; }
  static constexpr GaugeProfile bar() {
    return GaugeProfile{Unit::Bar, -1.0f, 1.7f, 0.5f, 1.0f, 1.4f};
  }
  static constexpr GaugeProfile kpa() {
    return GaugeProfile{Unit::Kpa, -100.0f, 170.0f, 50.0f, 100.0f, 140.0f};
  }

  /// Boost in psi (what the sampler measures) as a number in this profile's unit.
  float toDisplay(float boostPsi) const {
    if (unit == Unit::Bar) return psiToBar(boostPsi);
    if (unit == Unit::Kpa) return psiToKpa(boostPsi);
    return boostPsi;
  }

  /// Upper-case label for the unit, as drawn on the dial.
  const char* unitLabel() const {
    if (unit == Unit::Bar) return "BAR";
    if (unit == Unit::Kpa) return "KPA";
    return "PSI";
  }

  /// Decimal places for the big readout and the peak value.
  int decimals() const {
    if (unit == Unit::Bar) return 2;
    if (unit == Unit::Kpa) return 0;
    return 1;
  }

  /// Decimal places for the numbers on the scale.
  int labelDecimals() const { return unit == Unit::Bar ? 1 : 0; }

  /// The width of one bar-graph segment: a fifth of a labelled step (1 psi for the default).
  float segmentStep() const { return majorStep / 5.0f; }

  /// Dial angle for a number in this profile's unit.
  GaugeScale scale() const { return GaugeScale{min, max}; }
};

}  // namespace gauge
