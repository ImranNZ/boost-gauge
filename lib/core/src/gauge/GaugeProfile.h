#pragma once

#include "gauge/GaugeScale.h"
#include "obd/Units.h"

namespace gauge {

/// The unit a gauge shows its value in. The pipeline carries one canonical unit per kind of
/// reading (psi for pressure, degrees Celsius for a temperature, volts for a voltage); a look
/// converts for display.
enum class Unit { Psi, Bar, Kpa, Celsius, Fahrenheit, Volt };

/// Everything about a gauge that depends on what it shows and the owner rather than on the look:
/// the unit, the scale's range, where the amber and red zones start, and what it is called.
/// Ranges are in `unit`.
///
/// The default is the original psi boost gauge (-15 to +25 psi, amber from 15, red from 20), which
/// suits a stock turbo like the F56's. `bar()` and `kpa()` are metric equivalents, and there are
/// presets for the other channels (coolant, intake air, battery voltage). Change the values for a
/// car with more or less range. Zones assume that high is bad, so a voltage has none.
///
/// The code-drawn looks (Classic, Minimal, Motorsport) follow the profile. The pre-rendered
/// dials (Retro, Modern) are pictures of a psi scale, drawn from looks/<name>/spec.json.
struct GaugeProfile {
  Unit unit = Unit::Psi;
  float min = -15.0f;           // bottom of the scale (vacuum)
  float max = 25.0f;            // top of the scale
  float majorStep = 5.0f;       // labelled graduations
  float amberFrom = 15.0f;      // the amber zone starts here
  float redFrom = 20.0f;        // the red zone (and the Motorsport shift light) starts here
  const char* label = "BOOST";  // what it measures, in capitals, for looks that print it
  /// Printed instead of `label` while the value is below zero; null keeps `label`.
  const char* negativeLabel = "VACUUM";

  static constexpr GaugeProfile psi() { return GaugeProfile{}; }
  static constexpr GaugeProfile bar() {
    return GaugeProfile{Unit::Bar, -1.0f, 1.7f, 0.5f, 1.0f, 1.4f};
  }
  static constexpr GaugeProfile kpa() {
    return GaugeProfile{Unit::Kpa, -100.0f, 170.0f, 50.0f, 100.0f, 140.0f};
  }
  /// Engine coolant temperature.
  static constexpr GaugeProfile coolant() {
    return GaugeProfile{Unit::Celsius, 40.0f, 140.0f, 20.0f, 105.0f, 115.0f, "COOLANT", nullptr};
  }
  static constexpr GaugeProfile coolantF() {
    return GaugeProfile{Unit::Fahrenheit, 100.0f, 280.0f,    20.0f,
                        220.0f,           240.0f, "COOLANT", nullptr};
  }
  /// Intake air temperature.
  static constexpr GaugeProfile intakeAir() {
    return GaugeProfile{Unit::Celsius, -20.0f, 80.0f, 20.0f, 50.0f, 65.0f, "INTAKE", nullptr};
  }
  /// Battery (control module) voltage. No zones: the scale ends where "high" would be bad.
  static constexpr GaugeProfile voltage() {
    return GaugeProfile{Unit::Volt, 8.0f, 16.0f, 2.0f, 16.0f, 16.0f, "BATTERY", nullptr};
  }

  /// A reading in its canonical unit (what the sampler measures) as a number in this profile's
  /// unit.
  float toDisplay(float canonical) const {
    if (unit == Unit::Bar) return psiToBar(canonical);
    if (unit == Unit::Kpa) return psiToKpa(canonical);
    if (unit == Unit::Fahrenheit) return canonical * 1.8f + 32.0f;
    return canonical;
  }

  /// Upper-case label for the unit, as drawn on the dial.
  const char* unitLabel() const {
    if (unit == Unit::Bar) return "BAR";
    if (unit == Unit::Kpa) return "KPA";
    if (unit == Unit::Celsius) return "C";
    if (unit == Unit::Fahrenheit) return "F";
    if (unit == Unit::Volt) return "V";
    return "PSI";
  }

  /// Decimal places for the big readout and the peak value.
  int decimals() const {
    if (unit == Unit::Bar) return 2;
    if (unit == Unit::Kpa || unit == Unit::Celsius || unit == Unit::Fahrenheit) return 0;
    return 1;  // psi and volts
  }

  /// Decimal places for the numbers on the scale.
  int labelDecimals() const { return unit == Unit::Bar ? 1 : 0; }

  /// The width of one bar-graph segment: a fifth of a labelled step (1 psi for the default).
  float segmentStep() const { return majorStep / 5.0f; }

  /// Dial angle for a number in this profile's unit.
  GaugeScale scale() const { return GaugeScale{min, max}; }
};

}  // namespace gauge
