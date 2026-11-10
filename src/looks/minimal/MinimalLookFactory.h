#pragma once

// Builds the Minimal look. Shared by the firmware and the PC sim.
//
// Like the Retro look, it picks up the locally generated badge (the light variant for dark dials;
// see RetroLookFactory.h) when src/looks/local/BadgeLight.h (the light variant, for dark dials)
// exists; the badge is brand artwork and is never committed.

#include "looks/Fonts.h"
#include "looks/MinimalLook.h"

#if __has_include("looks/local/BadgeLight.h") && !defined(BOOST_GAUGE_NO_BADGE)
#include "looks/local/BadgeLight.h"
#define BOOST_GAUGE_MINIMAL_HAS_BADGE 1
#endif

namespace looks {

inline gauge::MinimalLook::Fonts minimalFonts() {
  return {fonts::outfitLight62(), fonts::outfitMedium12()};
}

inline gauge::MinimalLook makeMinimal(const gauge::GaugeProfile& profile = {}) {
#ifdef BOOST_GAUGE_MINIMAL_HAS_BADGE
  gauge::Badge overlay;
  overlay.picture = gauge::Picture(badge_light::kFaces, badge_light::kFaceCount);
  overlay.x = 120 - badge_light::kWidth / 2;  // centred above the mode label
  overlay.y = 62 - badge_light::kHeight / 2;
  return gauge::MinimalLook(minimalFonts(), profile, overlay);
#else
  return gauge::MinimalLook(minimalFonts(), profile);
#endif
}

}  // namespace looks
