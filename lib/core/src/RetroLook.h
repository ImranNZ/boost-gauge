#pragma once

#include "Badge.h"
#include "Color.h"
#include "GaugeScale.h"
#include "IDrawSurface.h"
#include "ILook.h"
#include "ImageView.h"

namespace gauge {

/// The "Retro 60s" look: a cream dial that is a pre-rendered picture (see
/// looks/retro/), with a slim red needle, a peak flag and a chrome-ringed hub
/// drawn on top. Purely analog.
class RetroLook : public ILook {
 public:
  /// The geometry the dial artwork was drawn with. It is generated from
  /// looks/retro/spec.json, the single source for both the picture and this
  /// code, so the needle always lines up with the printed scale.
  struct Spec {
    float minPsi;            // scale the dial is graduated for
    float maxPsi;
    float needleLength;      // pixels from the pivot to the tip
    float tailLength;        // pixels from the pivot to the counterweight
    float flagInnerRadius;   // the peak flag sits between these two radii
    float flagOuterRadius;
    int hubRadius;
  };

  static constexpr Color kNeedleRed{200, 40, 30};
  static constexpr Color kInk{28, 26, 22};

  RetroLook(ImageView dial, Spec spec);
  RetroLook(ImageView dial, Spec spec, Badge badge);

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

 private:
  void drawNeedle(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawNeedleShape(IDrawSurface& c, float cx, float cy, float dx, float dy, Color color) const;
  void drawPeakFlag(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawHub(IDrawSurface& c, int cx, int cy) const;

  ImageView dial_;
  Spec spec_;
  GaugeScale scale_;
  Badge badge_;
  bool hasBadge_ = false;
};

}  // namespace gauge
