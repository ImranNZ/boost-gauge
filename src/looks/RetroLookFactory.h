#pragma once

// Builds the Retro look from the generated artwork. Shared by the firmware and
// the PC sim so both draw exactly the same pixels.
//
// The Cooper S badge is brand artwork, so it is NOT in the repository. If you
// generated it locally (tools/make_look.py badge ...), src/looks/local/Badge.h
// exists and is picked up here automatically; otherwise the dial is used without it.

#include "GaugeScale.h"
#include "RetroLook.h"
#include "looks/RetroDial.h"
#include "looks/RetroSpec.h"

#if __has_include("looks/local/Badge.h")
#include "looks/local/Badge.h"
#define BOOST_GAUGE_HAS_BADGE 1
#endif

// The picture and the code are generated from one spec; these catch the one thing
// the generator cannot: the firmware's own scale constants drifting away from it.
static_assert(looks::retro::kStartDeg == gauge::GaugeScale::kStartDeg,
              "dial artwork start angle != GaugeScale::kStartDeg: re-run tools/make_look.py retro");
static_assert(looks::retro::kSweepDeg == gauge::GaugeScale::kSweepDeg,
              "dial artwork sweep != GaugeScale::kSweepDeg: re-run tools/make_look.py retro");

namespace looks {

inline gauge::ImageView retroDial() {
  return {retro::kDialWidth, retro::kDialHeight, retro::kDialRgb565, nullptr};
}

inline gauge::RetroLook::Spec retroSpec() {
  return {retro::kMinPsi,         retro::kMaxPsi,          retro::kNeedleLength,
          retro::kTailLength,     retro::kFlagInnerRadius, retro::kFlagOuterRadius,
          retro::kHubRadius};
}

inline gauge::RetroLook makeRetro() {
#ifdef BOOST_GAUGE_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge::kWidth, badge::kHeight, badge::kRgb565, badge::kAlpha};
  overlay.x = 120 - badge::kWidth / 2;  // centred under the 5 psi numeral
  overlay.y = 80 - badge::kHeight / 2;
  return gauge::RetroLook(retroDial(), retroSpec(), overlay);
#else
  return gauge::RetroLook(retroDial(), retroSpec());
#endif
}

}  // namespace looks
