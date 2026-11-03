#pragma once

// Builds the Motorsport look. Shared by the firmware and the PC sim.
//
// Like the Retro look, it picks up the locally generated badge (see
// RetroLookFactory.h) when src/looks/local/Badge.h exists; the badge is brand
// artwork and is never committed.

#include "MotorsportLook.h"

#if __has_include("looks/local/Badge.h")
#include "looks/local/Badge.h"
#define BOOST_GAUGE_MOTORSPORT_HAS_BADGE 1
#endif

namespace looks {

inline gauge::MotorsportLook makeMotorsport() {
#ifdef BOOST_GAUGE_MOTORSPORT_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge::kWidth, badge::kHeight, badge::kRgb565, badge::kAlpha};
  overlay.x = 120 - badge::kWidth / 2;  // centred above the BOOST label
  overlay.y = 48 - badge::kHeight / 2;
  return gauge::MotorsportLook(overlay);
#else
  return gauge::MotorsportLook();
#endif
}

}  // namespace looks
