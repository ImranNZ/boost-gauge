#pragma once

#include <cstdint>

#include "gauge/ChannelProfile.h"
#include "looks/LookId.h"
#include "sampling/ChannelKind.h"

namespace gauge {

/// What the owner chooses on the gauge: the look, the reading shown, and the unit it is shown in.
/// A plain value, so it can be stored, compared and handed to whatever applies it.
struct Setup {
  LookId look = LookId::Retro;
  ChannelKind channel = ChannelKind::Boost;
  Unit unit = Unit::Psi;

  bool operator==(const Setup& other) const {
    return look == other.look && channel == other.channel && unit == other.unit;
  }
  bool operator!=(const Setup& other) const { return !(*this == other); }
};

/// `setup` with any impossible combination repaired: a boost-dial look with another channel
/// becomes Classic, and a unit the channel cannot be shown in becomes its default.
Setup normalised(Setup setup);

/// The gauge profile (range, zones, name) for a setup. Normalise it first.
GaugeProfile profileOf(const Setup& setup);

/// A setup as one 32-bit word, for a store that keeps one (flash, EEPROM). The top byte is a
/// marker, so an erased or foreign word is not mistaken for a setup.
uint32_t encodeSetup(const Setup& setup);

/// The reverse of encodeSetup(). False, leaving `out` alone, if the word is not a valid setup.
/// A valid but impossible combination is repaired (see normalised()).
bool decodeSetup(uint32_t word, Setup& out);

}  // namespace gauge
