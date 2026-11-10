#pragma once

#include "gauge/GaugeScale.h"
#include "looks/ILook.h"
#include "looks/PointerSpec.h"
#include "render/Badge.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/IDrawSurface.h"
#include "render/Picture.h"

namespace gauge {

/// The "Retro 60s" look: a cream dial that is a pre-rendered picture (see
/// looks/retro/), with a slim red needle, a peak flag and a chrome-ringed hub
/// drawn on top. Purely analog.
class RetroLook : public ILook {
 public:
  /// The geometry the dial artwork was drawn with (generated from looks/retro/spec.json).
  using Spec = PointerSpec;

  static constexpr Color kNeedleRed{200, 40, 30};
  static constexpr Color kInk{28, 26, 22};

  /// The two typefaces it needs, for the NO LINK notice and the small frame-rate text
  /// (capitals, digits, "-", "+" and "."). The numerals are part of the dial picture.
  struct Fonts {
    Font notice;
    Font small;
  };

  RetroLook(Picture dial, Spec spec, const Fonts& fonts);
  RetroLook(Picture dial, Spec spec, const Fonts& fonts, const Badge& badge);

  void drawDial(IDrawSurface& c) const override;
  void drawDynamic(IDrawSurface& c, const GaugeState& state) const override;

 private:
  void drawNeedle(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawNeedleShape(IDrawSurface& c, float cx, float cy, float dx, float dy, Color color) const;
  void drawPeakFlag(IDrawSurface& c, float cx, float cy, float deg) const;
  void drawHub(IDrawSurface& c, int cx, int cy) const;

  Picture dial_;
  Spec spec_;
  Fonts fonts_;
  GaugeScale scale_;
  Badge badge_;
  bool hasBadge_ = false;
};

}  // namespace gauge
