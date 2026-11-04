#include "looks/FrameRenderer.h"

namespace gauge {

void FrameRenderer::render(const ILook& look, const GaugeState& state) {
  if (!canvas_.restoreBackground()) {
    look.drawDial(canvas_);
    canvas_.saveBackground();
  }
  look.drawDynamic(canvas_, state);
  canvas_.present();
}

}  // namespace gauge
