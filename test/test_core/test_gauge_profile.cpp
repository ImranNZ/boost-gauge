// GaugeProfile: the unit, the range and the zones a gauge is configured with.

#include "test_support.h"

using namespace support;

TEST_CASE("profile: the default is the original psi gauge") {
  const GaugeProfile profile;
  CHECK(profile.unit == Unit::Psi);
  CHECK(profile.min == doctest::Approx(-15.0f));
  CHECK(profile.max == doctest::Approx(25.0f));
  CHECK(profile.majorStep == doctest::Approx(5.0f));
  CHECK(profile.amberFrom == doctest::Approx(15.0f));
  CHECK(profile.redFrom == doctest::Approx(20.0f));

  const GaugeProfile preset = GaugeProfile::psi();
  CHECK(preset.unit == Unit::Psi);
  CHECK(preset.max == doctest::Approx(25.0f));
}

TEST_CASE("profile: the metric presets cover about the same pressures") {
  const GaugeProfile bar = GaugeProfile::bar();
  CHECK(bar.unit == Unit::Bar);
  CHECK(bar.min == doctest::Approx(-1.0f));
  CHECK(bar.max == doctest::Approx(1.7f));
  CHECK(bar.amberFrom == doctest::Approx(1.0f));
  CHECK(bar.redFrom == doctest::Approx(1.4f));

  const GaugeProfile kpa = GaugeProfile::kpa();
  CHECK(kpa.unit == Unit::Kpa);
  CHECK(kpa.min == doctest::Approx(-100.0f));
  CHECK(kpa.max == doctest::Approx(170.0f));
  CHECK(kpa.redFrom == doctest::Approx(140.0f));

  // 1.7 bar is 24.7 psi, 170 kPa is 24.7 psi: close to the psi gauge's 25.
  CHECK(barToPsi(bar.max) == doctest::Approx(24.66f).epsilon(0.01));
  CHECK(kpaToPsi(kpa.max) == doctest::Approx(24.66f).epsilon(0.01));
}

TEST_CASE("profile: boost in psi becomes a number in the profile's unit") {
  CHECK(GaugeProfile::psi().toDisplay(12.5f) == doctest::Approx(12.5f));
  CHECK(GaugeProfile::bar().toDisplay(kPsiPerBar) == doctest::Approx(1.0f));
  CHECK(GaugeProfile::bar().toDisplay(-7.25f) == doctest::Approx(-0.5f).epsilon(0.001));
  CHECK(GaugeProfile::kpa().toDisplay(1.0f) == doctest::Approx(kKpaPerPsi));
}

TEST_CASE("profile: the unit label, decimals and segment size follow the unit") {
  CHECK(std::string(GaugeProfile::psi().unitLabel()) == "PSI");
  CHECK(std::string(GaugeProfile::bar().unitLabel()) == "BAR");
  CHECK(std::string(GaugeProfile::kpa().unitLabel()) == "KPA");

  CHECK(GaugeProfile::psi().decimals() == 1);
  CHECK(GaugeProfile::bar().decimals() == 2);
  CHECK(GaugeProfile::kpa().decimals() == 0);
  CHECK(GaugeProfile::psi().labelDecimals() == 0);
  CHECK(GaugeProfile::bar().labelDecimals() == 1);
  CHECK(GaugeProfile::kpa().labelDecimals() == 0);

  CHECK(GaugeProfile::psi().segmentStep() == doctest::Approx(1.0f));
  CHECK(GaugeProfile::bar().segmentStep() == doctest::Approx(0.1f));
  CHECK(GaugeProfile::kpa().segmentStep() == doctest::Approx(10.0f));
}

TEST_CASE("profile: the dial scale spans the profile's range") {
  const GaugeScale scale = GaugeProfile::bar().scale();
  CHECK(scale.angleFor(-1.0f) == doctest::Approx(GaugeScale::kStartDeg));
  CHECK(scale.angleFor(1.7f) == doctest::Approx(GaugeScale::kStartDeg + GaugeScale::kSweepDeg));
}

TEST_CASE("units: bar and psi convert both ways") {
  CHECK(psiToBar(kPsiPerBar) == doctest::Approx(1.0f));
  CHECK(barToPsi(1.0f) == doctest::Approx(kPsiPerBar));
  CHECK(barToPsi(psiToBar(18.0f)) == doctest::Approx(18.0f));
}
