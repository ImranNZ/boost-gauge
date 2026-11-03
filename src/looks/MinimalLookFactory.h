#pragma once

// Builds the Minimal look. Shared by the firmware and the PC sim.
//
// Like the Retro look, it picks up the locally generated badge (see
// RetroLookFactory.h) when src/looks/local/Badge.h exists; the badge is brand
// artwork and is never committed.

#include "MinimalLook.h"

#if __has_include("looks/local/Badge.h")
#include "looks/local/Badge.h"
#define BOOST_GAUGE_MINIMAL_HAS_BADGE 1
#endif

namespace looks {

inline gauge::MinimalLook makeMinimal() {
#ifdef BOOST_GAUGE_MINIMAL_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge::kWidth, badge::kHeight, badge::kRgb565, badge::kAlpha};
  overlay.x = 120 - badge::kWidth / 2;  // centred above the mode label
  overlay.y = 44 - badge::kHeight / 2;
  return gauge::MinimalLook(overlay);
#else
  return gauge::MinimalLook();
#endif
}

}  // namespace looks
