#pragma once

#include "Color.h"
#include "GaugeScale.h"
#include "ILook.h"
#include "ImageView.h"

namespace gauge {

/// The "Retro 60s" look: a cream dial that is a pre-rendered picture (see
/// looks/retro/dial.svg and tools/make_look.py), with a slim red needle, a peak
/// flag and a chrome-ringed hub drawn on top. Purely analog; the artwork is
/// baked for a -15..25 psi scale.
class RetroLook : public DialCachingLook {
 public:
  /// An optional picture laid over the dial, e.g. a model badge. Top-left in pixels.
  struct Badge {
    ImageView image;
    int x = 0;
    int y = 0;
  };

  static constexpr Color kNeedleRed{200, 40, 30};
  static constexpr Color kInk{28, 26, 22};

  explicit RetroLook(ImageView dial);
  RetroLook(ImageView dial, Badge badge);

 protected:
  void drawDial(ICanvas& c) const override;
  void drawDynamic(ICanvas& c, const GaugeState& state) const override;

 private:
  void drawNeedle(ICanvas& c, float cx, float cy, float deg) const;
  void drawNeedleShape(ICanvas& c, float cx, float cy, float dx, float dy, Color color) const;
  void drawPeakFlag(ICanvas& c, float cx, float cy, float deg) const;
  void drawHub(ICanvas& c, int cx, int cy) const;

  ImageView dial_;
  Badge badge_;
  bool hasBadge_ = false;
  GaugeScale scale_;
};

}  // namespace gauge
