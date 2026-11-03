#pragma once

// Builds the Minimal look. Shared by the firmware and the PC sim.
//
// Like the Retro look, it picks up the locally generated badge (see
// RetroLookFactory.h) when src/looks/local/Badge.h exists; the badge is brand
// artwork and is never committed.

#include "MinimalLook.h"
#include "looks/fonts/OutfitLight62.h"
#include "looks/fonts/OutfitMedium12.h"

#if __has_include("looks/local/Badge.h")
#include "looks/local/Badge.h"
#define BOOST_GAUGE_MINIMAL_HAS_BADGE 1
#endif

namespace looks {

inline gauge::MinimalLook::Fonts minimalFonts() {
  return {fonts::outfitLight62(), fonts::outfitMedium12()};
}

inline gauge::MinimalLook makeMinimal() {
#ifdef BOOST_GAUGE_MINIMAL_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge::kWidth, badge::kHeight, badge::kRgb565, badge::kAlpha};
  overlay.x = 120 - badge::kWidth / 2;  // centred above the mode label
  overlay.y = 62 - badge::kHeight / 2;
  return gauge::MinimalLook(minimalFonts(), overlay);
#else
  return gauge::MinimalLook(minimalFonts());
#endif
}

}  // namespace looks
