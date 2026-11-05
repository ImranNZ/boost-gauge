#include "gauge/ChannelProfile.h"

namespace gauge {

bool profileFor(ChannelKind kind, Unit unit, GaugeProfile& out) {
  if (kind == ChannelKind::Boost) {
    if (unit == Unit::Psi)
      out = GaugeProfile::psi();
    else if (unit == Unit::Bar)
      out = GaugeProfile::bar();
    else if (unit == Unit::Kpa)
      out = GaugeProfile::kpa();
    else
      return false;
    return true;
  }
  if (kind == ChannelKind::Coolant) {
    if (unit == Unit::Celsius)
      out = GaugeProfile::coolant();
    else if (unit == Unit::Fahrenheit)
      out = GaugeProfile::coolantF();
    else
      return false;
    return true;
  }
  if (kind == ChannelKind::IntakeAir) {
    if (unit == Unit::Celsius)
      out = GaugeProfile::intakeAir();
    else if (unit == Unit::Fahrenheit)
      out = GaugeProfile::intakeAirF();
    else
      return false;
    return true;
  }
  if (unit != Unit::Volt) return false;
  out = GaugeProfile::voltage();
  return true;
}

bool parseUnit(std::string_view name, Unit& unit) {
  if (name == "psi")
    unit = Unit::Psi;
  else if (name == "bar")
    unit = Unit::Bar;
  else if (name == "kpa")
    unit = Unit::Kpa;
  else if (name == "c")
    unit = Unit::Celsius;
  else if (name == "f")
    unit = Unit::Fahrenheit;
  else if (name == "v")
    unit = Unit::Volt;
  else
    return false;
  return true;
}

}  // namespace gauge
