#pragma once

// Builds the Classic look. Shared by the firmware and the PC sim.

#include "looks/ClassicLook.h"
#include "looks/fonts/BarlowSemiBold10.h"
#include "looks/fonts/BarlowSemiBold18.h"
#include "looks/fonts/BarlowSemiBold40.h"

namespace looks {

inline gauge::ClassicLook::Fonts classicFonts() {
  return {fonts::barlowSemiBold40(), fonts::barlowSemiBold18(), fonts::barlowSemiBold10()};
}

inline gauge::ClassicLook makeClassic(const gauge::GaugeProfile& profile = {}) {
  return gauge::ClassicLook(classicFonts(), profile);
}

}  // namespace looks
