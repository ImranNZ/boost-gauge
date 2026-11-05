#pragma once

#include <string_view>

#include "gauge/GaugeProfile.h"
#include "sampling/ChannelKind.h"

namespace gauge {

/// The unit a channel is shown in unless told otherwise: psi, then Celsius, Celsius and volts.
constexpr Unit defaultUnit(ChannelKind kind) {
  if (kind == ChannelKind::Coolant || kind == ChannelKind::IntakeAir) return Unit::Celsius;
  if (kind == ChannelKind::Voltage) return Unit::Volt;
  return Unit::Psi;
}

/// The profile for a channel in its default unit.
constexpr GaugeProfile defaultProfile(ChannelKind kind) {
  if (kind == ChannelKind::Coolant) return GaugeProfile::coolant();
  if (kind == ChannelKind::IntakeAir) return GaugeProfile::intakeAir();
  if (kind == ChannelKind::Voltage) return GaugeProfile::voltage();
  return GaugeProfile::psi();
}

/// The profile for a channel shown in `unit`. False if that channel cannot be shown in that unit
/// (pressure in degrees, say); `out` is then left alone.
bool profileFor(ChannelKind kind, Unit unit, GaugeProfile& out);

/// Recognise a unit by its short lower-case name: psi, bar, kpa, c, f or v. False if it is none.
bool parseUnit(std::string_view name, Unit& unit);

}  // namespace gauge
