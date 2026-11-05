#include "app/Setup.h"

namespace gauge {
namespace {

constexpr uint32_t kMarker = 0x47;  // 'G'

}  // namespace

Setup normalised(Setup setup) {
  if (lookNeedsBoost(setup.look) && setup.channel != ChannelKind::Boost) {
    setup.look = LookId::Classic;
  }
  GaugeProfile unused;
  if (!profileFor(setup.channel, setup.unit, unused)) setup.unit = defaultUnit(setup.channel);
  return setup;
}

GaugeProfile profileOf(const Setup& setup) {
  GaugeProfile profile = defaultProfile(setup.channel);
  profileFor(setup.channel, setup.unit, profile);  // keeps the default if the unit does not fit
  return profile;
}

uint32_t encodeSetup(const Setup& setup) {
  return (kMarker << 24) | (static_cast<uint32_t>(setup.look) << 16) |
         (static_cast<uint32_t>(setup.channel) << 8) | static_cast<uint32_t>(setup.unit);
}

bool decodeSetup(uint32_t word, Setup& out) {
  const uint32_t look = (word >> 16) & 0xFF;
  const uint32_t channel = (word >> 8) & 0xFF;
  const uint32_t unit = word & 0xFF;
  if ((word >> 24) != kMarker || look >= static_cast<uint32_t>(kLookIdCount) ||
      channel >= kChannelKindCount || unit > static_cast<uint32_t>(Unit::Volt)) {
    return false;
  }
  Setup decoded;
  decoded.look = static_cast<LookId>(look);
  decoded.channel = static_cast<ChannelKind>(channel);
  decoded.unit = static_cast<Unit>(unit);
  out = normalised(decoded);
  return true;
}

}  // namespace gauge
