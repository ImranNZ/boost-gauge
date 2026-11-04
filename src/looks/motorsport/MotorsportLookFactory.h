#pragma once

// Builds the Motorsport look. Shared by the firmware and the PC sim.
//
// Like the Retro look, it picks up the locally generated badge (see
// RetroLookFactory.h) when src/looks/local/BadgeLight.h (the light variant, for dark dials) exists;
// the badge is brand artwork and is never committed.

#include "looks/MotorsportLook.h"
#include "looks/fonts/SairaBold70.h"
#include "looks/fonts/SairaSemiBold12.h"
#include "looks/fonts/SairaSemiBold14.h"

#if __has_include("looks/local/BadgeLight.h") && !defined(BOOST_GAUGE_NO_BADGE)
#include "looks/local/BadgeLight.h"
#define BOOST_GAUGE_MOTORSPORT_HAS_BADGE 1
#endif

namespace looks {

inline gauge::MotorsportLook::Fonts motorsportFonts() {
  return {fonts::sairaBold70(), fonts::sairaSemiBold12(), fonts::sairaSemiBold14()};
}

inline gauge::MotorsportLook makeMotorsport() {
#ifdef BOOST_GAUGE_MOTORSPORT_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge_light::kWidth, badge_light::kHeight, badge_light::kRgb565,
                   badge_light::kAlpha};
  overlay.x = 120 - badge_light::kWidth / 2;  // centred above the BOOST label
  overlay.y = 64 - badge_light::kHeight / 2;
  return gauge::MotorsportLook(motorsportFonts(), overlay);
#else
  return gauge::MotorsportLook(motorsportFonts());
#endif
}

}  // namespace looks
