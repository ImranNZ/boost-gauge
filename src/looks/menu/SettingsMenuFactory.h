#pragma once

// The typefaces for the on-gauge settings menu. Shared by the firmware and the PC sim.

#include "app/SettingsMenu.h"
#include "looks/Fonts.h"

namespace looks {

inline gauge::SettingsMenu::Fonts menuFonts() {
  return {fonts::barlowSemiBold18(), fonts::barlowSemiBold30()};
}

inline gauge::SettingsMenu makeSettingsMenu(const gauge::Setup& current) {
  return gauge::SettingsMenu(current, menuFonts());
}

}  // namespace looks
