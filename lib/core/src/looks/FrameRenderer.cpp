#include "looks/FrameRenderer.h"

namespace gauge {

void FrameRenderer::render(const ILook& look, const GaugeState& state) {
  if (!canvas_.restoreBackground()) {
    look.drawDial(surface_);
    canvas_.saveBackground();
  }
  look.drawDynamic(surface_, state);
  canvas_.present();
}

}  // namespace gauge
