#pragma once

#include "gauge/GaugeState.h"
#include "render/IDrawSurface.h"

namespace gauge {

/// A complete visual style for the gauge. A look only describes drawing; whether
/// the static layer is cached is the FrameRenderer's business, not the look's.
class ILook {
 public:
  virtual ~ILook() = default;

  /// The static artwork (dial face, scale, labels). Runs only when the cached
  /// frame is missing, so it may be slow; it must not depend on the state.
  virtual void drawDial(IDrawSurface& surface) const = 0;
  /// Everything that changes frame to frame: needle, readouts, overlays.
  virtual void drawDynamic(IDrawSurface& surface, const GaugeState& state) const = 0;
};

}  // namespace gauge
