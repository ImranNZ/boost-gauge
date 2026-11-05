#pragma once

#include "gauge/GaugeState.h"
#include "looks/ILook.h"
#include "render/ICanvas.h"
#include "render/IOverlay.h"
#include "render/ScaledSurface.h"

namespace gauge {

/// Draws one frame with a look: restore the cached dial (drawing and caching it
/// first if there is none), draw the moving parts, then the overlay if one is up, then show the
/// frame. Looks are drawn for a
/// 240x240 screen; the renderer scales them to the canvas (see ScaledSurface).
class FrameRenderer {
 public:
  explicit FrameRenderer(ICanvas& canvas) : canvas_(canvas), surface_(canvas) {}

  /// `overlay` (optional) is drawn over the look whenever it is active.
  void render(const ILook& look, const GaugeState& state, const IOverlay* overlay = nullptr);

  /// Drop the cached dial; call when the look changes.
  void invalidate() { canvas_.discardBackground(); }

 private:
  ICanvas& canvas_;
  ScaledSurface surface_;
};

}  // namespace gauge
