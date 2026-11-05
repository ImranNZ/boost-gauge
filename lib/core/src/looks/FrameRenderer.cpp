#include "looks/FrameRenderer.h"

namespace gauge {

void FrameRenderer::render(const ILook& look, const GaugeState& state, const IOverlay* overlay) {
  if (!canvas_.restoreBackground()) {
    look.drawDial(surface_);
    canvas_.saveBackground();
  }
  look.drawDynamic(surface_, state);
  if (overlay != nullptr && overlay->active()) overlay->draw(surface_);
  canvas_.present();
}

}  // namespace gauge
