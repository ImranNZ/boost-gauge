#pragma once

namespace gauge {

/// Remembers one finished frame so static artwork (the dial) is drawn only once.
class IFrameCache {
 public:
  virtual ~IFrameCache() = default;

  /// Remember the current frame.
  virtual void saveBackground() = 0;
  /// Replace the frame with the saved one. False if nothing is saved (or the
  /// backend cannot cache), in which case the caller must redraw it itself.
  virtual bool restoreBackground() = 0;
  /// Forget the saved frame (the artwork changed, e.g. a different look).
  virtual void discardBackground() = 0;
};

}  // namespace gauge
