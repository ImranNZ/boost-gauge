#pragma once

// Builds the Modern Mini look from the generated artwork. Shared by the firmware and
// the PC sim so both draw exactly the same pixels.
//
// Like the other looks it picks up the locally generated badge (see RetroLookFactory.h)
// when src/looks/local/BadgeLight.h (the light variant, for dark dials) exists; the badge is brand artwork and is never committed.

#include "gauge/GaugeScale.h"
#include "looks/ModernLook.h"
#include "looks/modern/ModernDial.h"
#include "looks/modern/ModernSpec.h"
#include "looks/fonts/BarlowCondensed11.h"
#include "looks/fonts/BarlowSemiBold21.h"

#if __has_include("looks/local/BadgeLight.h") && !defined(BOOST_GAUGE_NO_BADGE)
#include "looks/local/BadgeLight.h"
#define BOOST_GAUGE_MODERN_HAS_BADGE 1
#endif

// The picture and the code are generated from one spec; these catch the one thing
// the generator cannot: the firmware's own scale constants drifting away from it.
static_assert(looks::modern::kStartDeg == gauge::GaugeScale::kStartDeg,
              "dial artwork start angle != GaugeScale::kStartDeg: re-run tools/make_look.py modern");
static_assert(looks::modern::kSweepDeg == gauge::GaugeScale::kSweepDeg,
              "dial artwork sweep != GaugeScale::kSweepDeg: re-run tools/make_look.py modern");

namespace looks {

inline gauge::ImageView modernDial() {
  return {modern::kDialWidth, modern::kDialHeight, modern::kDialRgb565, nullptr};
}

inline gauge::ModernLook::Spec modernSpec() {
  return {modern::kMinPsi,         modern::kMaxPsi,          modern::kNeedleLength,
          modern::kTailLength,     modern::kFlagInnerRadius, modern::kFlagOuterRadius,
          modern::kHubRadius};
}

inline gauge::ModernLook::Fonts modernFonts() {
  return {fonts::barlowSemiBold21(), fonts::barlowCondensed11()};
}

inline gauge::ModernLook makeModern() {
#ifdef BOOST_GAUGE_MODERN_HAS_BADGE
  gauge::Badge overlay;
  overlay.image = {badge_light::kWidth, badge_light::kHeight, badge_light::kRgb565, badge_light::kAlpha};
  overlay.x = 120 - badge_light::kWidth / 2;  // centred above BOOST
  overlay.y = 88 - badge_light::kHeight / 2;
  return gauge::ModernLook(modernDial(), modernSpec(), modernFonts(), overlay);
#else
  return gauge::ModernLook(modernDial(), modernSpec(), modernFonts());
#endif
}

}  // namespace looks
