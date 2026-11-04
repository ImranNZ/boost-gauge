#pragma once

#include "render/IDrawSurface.h"
#include "render/IFrameCache.h"

namespace gauge {

/// A whole display target: something to draw on, a frame cache, and a way to
/// show the result. Backends (the LCD, the PC rasteriser) implement this; the
/// rest of the code asks only for the narrower interface it needs.
class ICanvas : public IDrawSurface, public IFrameCache {
 public:
  /// Push the finished frame to the display (no-op for offscreen canvases).
  virtual void present() = 0;
};

}  // namespace gauge
