#pragma once

#include <optional>

#include "gauge/GaugeScale.h"
#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "looks/PointerSpec.h"
#include "render/Badge.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/IDrawSurface.h"
#include "render/ImageView.h"

namespace gauge {

/// The "Modern Mini" look: a chrome ring and black dial with amber and red bands,
/// as a pre-rendered picture (see looks/modern/), with an orange needle, a peak flag,
/// a chrome hub and the digital readout in its window drawn live on top.
class ModernLook : public ILook {
 public:
  using Spec = PointerSpec;

  /// The typefaces it needs: one for the number in the window (digits, "-" and ".")
  /// and a small condensed one for NO LINK and the fps label (capitals, digits, space).
  struct Fonts {
    Font readout;
    Font caption;
  };

  static constexpr Color kNeedle{255, 90, 31};
  static constexpr Color kFlag{244, 241, 234};

  ModernLook(ImageView dial, Spec spec, const Fonts& fonts)
      : dial_(dial), spec_(spec), scale_{spec.minPsi, spec.maxPsi}, fonts_(fonts) {}
  ModernLook(ImageView dial, Spec spec, const Fonts& fonts, const Badge& badge)
      : dial_(dial), spec_(spec), scale_{spec.minPsi, spec.maxPsi}, fonts_(fonts), badge_(badge) {}

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

 private:
  void drawNeedle(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawNeedleShape(IDrawSurface& c, float cx, float cy, float dx, float dy, Color color) const;
  void drawPeakFlag(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawHub(IDrawSurface& c, int cx, int cy) const;
  void drawReadout(IDrawSurface& c, const GaugeState& state) const;

  ImageView dial_;
  Spec spec_;
  GaugeScale scale_;
  Fonts fonts_;
  std::optional<Badge> badge_;
};

}  // namespace gauge
