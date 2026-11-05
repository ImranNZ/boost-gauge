// Channel kinds, the profile for each channel and unit, and the holder for the standard channels.

#include "gauge/ChannelProfile.h"
#include "sampling/StandardChannels.h"
#include "test_support.h"

using namespace support;

namespace {

constexpr ChannelKind kAllKinds[] = {ChannelKind::Boost, ChannelKind::Coolant,
                                     ChannelKind::IntakeAir, ChannelKind::Voltage};

}  // namespace

TEST_CASE("channel kind: every kind has a name that parses back to it") {
  for (ChannelKind kind : kAllKinds) {
    ChannelKind parsed = ChannelKind::Voltage;
    CAPTURE(channelName(kind));
    REQUIRE(parseChannelKind(channelName(kind), parsed));
    CHECK(parsed == kind);
  }
  CHECK(kChannelKindCount == 4);

  ChannelKind untouched = ChannelKind::Coolant;
  CHECK_FALSE(parseChannelKind("rpm", untouched));
  CHECK_FALSE(parseChannelKind("", untouched));
  CHECK(untouched == ChannelKind::Coolant);
  CHECK(std::string(channelName(ChannelKind::IntakeAir)) == "intake");
}

TEST_CASE("channel profile: each channel has a default unit and profile") {
  CHECK(defaultUnit(ChannelKind::Boost) == Unit::Psi);
  CHECK(defaultUnit(ChannelKind::Coolant) == Unit::Celsius);
  CHECK(defaultUnit(ChannelKind::IntakeAir) == Unit::Celsius);
  CHECK(defaultUnit(ChannelKind::Voltage) == Unit::Volt);

  for (ChannelKind kind : kAllKinds) {
    CAPTURE(channelName(kind));
    CHECK(defaultProfile(kind).unit == defaultUnit(kind));
  }
  CHECK(std::string(defaultProfile(ChannelKind::Boost).label) == "BOOST");
  CHECK(std::string(defaultProfile(ChannelKind::Coolant).label) == "COOLANT");
  CHECK(std::string(defaultProfile(ChannelKind::IntakeAir).label) == "INTAKE");
  CHECK(std::string(defaultProfile(ChannelKind::Voltage).label) == "BATTERY");
}

TEST_CASE("channel profile: the units each channel can be shown in") {
  struct Case {
    ChannelKind kind;
    Unit unit;
    const char* label;
  };
  const Case valid[] = {
      {ChannelKind::Boost, Unit::Psi, "BOOST"},
      {ChannelKind::Boost, Unit::Bar, "BOOST"},
      {ChannelKind::Boost, Unit::Kpa, "BOOST"},
      {ChannelKind::Coolant, Unit::Celsius, "COOLANT"},
      {ChannelKind::Coolant, Unit::Fahrenheit, "COOLANT"},
      {ChannelKind::IntakeAir, Unit::Celsius, "INTAKE"},
      {ChannelKind::IntakeAir, Unit::Fahrenheit, "INTAKE"},
      {ChannelKind::Voltage, Unit::Volt, "BATTERY"},
  };
  for (const Case& c : valid) {
    GaugeProfile profile = GaugeProfile::voltage();  // anything: it must be replaced
    CAPTURE(c.label);
    REQUIRE(profileFor(c.kind, c.unit, profile));
    CHECK(profile.unit == c.unit);
    CHECK(std::string(profile.label) == c.label);
  }

  // Pressure in degrees, a temperature in psi, volts in bar: refused, and `out` is left alone.
  const Case invalid[] = {
      {ChannelKind::Boost, Unit::Celsius, ""},      {ChannelKind::Coolant, Unit::Psi, ""},
      {ChannelKind::IntakeAir, Unit::Bar, ""},      {ChannelKind::Voltage, Unit::Psi, ""},
      {ChannelKind::Voltage, Unit::Fahrenheit, ""},
  };
  for (const Case& c : invalid) {
    GaugeProfile profile = GaugeProfile::coolant();
    CHECK_FALSE(profileFor(c.kind, c.unit, profile));
    CHECK(profile.unit == Unit::Celsius);
  }
}

TEST_CASE("channel profile: units are recognised by their short names") {
  const std::pair<const char*, Unit> names[] = {{"psi", Unit::Psi},      {"bar", Unit::Bar},
                                                {"kpa", Unit::Kpa},      {"c", Unit::Celsius},
                                                {"f", Unit::Fahrenheit}, {"v", Unit::Volt}};
  for (const auto& [name, expected] : names) {
    Unit unit = Unit::Psi;
    CAPTURE(name);
    REQUIRE(parseUnit(name, unit));
    CHECK(unit == expected);
  }
  Unit unit = Unit::Bar;
  CHECK_FALSE(parseUnit("kelvin", unit));
  CHECK_FALSE(parseUnit("", unit));
  CHECK(unit == Unit::Bar);
}

TEST_CASE("standard channels: one of each, with the periods the sampler schedules by") {
  ManualClock clock;
  StubObdClient client;
  StandardChannels channels(client, clock);

  CHECK(channels.get(ChannelKind::Boost).periodMs() == 0);
  CHECK(channels.get(ChannelKind::Coolant).periodMs() == 1000);
  CHECK(channels.get(ChannelKind::IntakeAir).periodMs() == 1000);
  CHECK(channels.get(ChannelKind::Voltage).periodMs() == 1000);

  // Distinct objects, and the same one each time.
  CHECK(&channels.get(ChannelKind::Coolant) != &channels.get(ChannelKind::IntakeAir));
  CHECK(&channels.get(ChannelKind::Coolant) != &channels.get(ChannelKind::Voltage));
  CHECK(&channels.get(ChannelKind::Boost) != &channels.get(ChannelKind::Coolant));
  CHECK(&channels.get(ChannelKind::Voltage) == &channels.get(ChannelKind::Voltage));
}
