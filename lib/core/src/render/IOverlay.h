#pragma once

#include "render/IDrawSurface.h"

namespace gauge {

/// Something drawn on top of the gauge when it is active (a settings menu). Drawn in the same
/// 240x240 design space as a look, after the look's moving parts.
class IOverlay {
 public:
  virtual ~IOverlay() = default;

  /// Whether it should be drawn this frame.
  virtual bool active() const = 0;
  virtual void draw(IDrawSurface& surface) const = 0;
};

}  // namespace gauge
