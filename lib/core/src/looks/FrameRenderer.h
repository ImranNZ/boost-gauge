#pragma once

#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "render/ICanvas.h"
#include "render/ScaledSurface.h"

namespace gauge {

/// Draws one frame with a look: restore the cached dial (drawing and caching it
/// first if there is none), draw the moving parts, then show the frame. Looks are drawn for a
/// 240x240 screen; the renderer scales them to the canvas (see ScaledSurface).
class FrameRenderer {
 public:
  explicit FrameRenderer(ICanvas& canvas) : canvas_(canvas), surface_(canvas) {}

  void render(const ILook& look, const GaugeState& state);

  /// Drop the cached dial; call when the look changes.
  void invalidate() { canvas_.discardBackground(); }

 private:
  ICanvas& canvas_;
  ScaledSurface surface_;
};

}  // namespace gauge
