#pragma once

#include "GaugeState.h"
#include "ICanvas.h"

namespace gauge {

/// A complete visual style for the gauge. The app hands it a state each frame
/// and it draws; it knows nothing about OBD or hardware.
class ILook {
 public:
  virtual ~ILook() = default;
  virtual void draw(ICanvas& canvas, const GaugeState& state) const = 0;
};

/// Base for looks with a static dial and a few moving parts: the dial is drawn
/// once and cached by the canvas, and only the moving parts are redrawn per frame.
class DialCachingLook : public ILook {
 public:
  void draw(ICanvas& canvas, const GaugeState& state) const final {
    if (!canvas.restoreBackground()) {
      drawDial(canvas);
      canvas.saveBackground();
    }
    drawDynamic(canvas, state);
  }

 protected:
  /// The static artwork. Runs only when the cache is empty.
  virtual void drawDial(ICanvas& canvas) const = 0;
  /// Needle, readouts, overlays: everything that changes frame to frame.
  virtual void drawDynamic(ICanvas& canvas, const GaugeState& state) const = 0;
};

}  // namespace gauge
