#pragma once

// Builds the Retro look from the generated artwork. Shared by the firmware and
// the PC sim so both draw exactly the same pixels.
//
// The Cooper S badge is brand artwork, so it is NOT in the repository. If you
// generated it locally (tools/make_look.py badge ...), src/looks/local/CooperSBadge.h
// exists and is picked up here automatically; otherwise the dial is used without it.

#include "RetroLook.h"
#include "looks/RetroDial.h"

#if __has_include("looks/local/CooperSBadge.h")
#include "looks/local/CooperSBadge.h"
#define BOOST_GAUGE_HAS_BADGE 1
#endif

namespace looks {

inline gauge::ImageView retroDial() {
  return {retro::kDialWidth, retro::kDialHeight, retro::kDialRgb565, nullptr};
}

inline gauge::RetroLook makeRetro() {
#ifdef BOOST_GAUGE_HAS_BADGE
  gauge::RetroLook::Badge overlay;
  overlay.image = {badge::kWidth, badge::kHeight, badge::kRgb565, badge::kAlpha};
  overlay.x = 120 - badge::kWidth / 2;  // centred under the 5 psi numeral
  overlay.y = 80 - badge::kHeight / 2;
  return gauge::RetroLook(retroDial(), overlay);
#else
  return gauge::RetroLook(retroDial());
#endif
}

}  // namespace looks
