#pragma once

#include <cstdint>

namespace gauge {

/// Frames per second, averaged over a window (default one second).
class FpsCounter {
 public:
  explicit FpsCounter(uint32_t windowMs = 1000) : windowMs_(windowMs) {}

  /// Call once per rendered frame.
  void frame(uint32_t nowMs) {
    if (!started_) {
      started_ = true;
      windowStartMs_ = nowMs;
    }
    ++count_;
    const uint32_t elapsed = nowMs - windowStartMs_;
    if (elapsed >= windowMs_) {
      fps_ = static_cast<float>(count_) * 1000.0f / static_cast<float>(elapsed);
      count_ = 0;
      windowStartMs_ = nowMs;
    }
  }

  /// Rate over the last completed window; 0 until the first one finishes.
  float fps() const { return fps_; }

 private:
  uint32_t windowMs_;
  uint32_t windowStartMs_ = 0;
  uint32_t count_ = 0;
  float fps_ = 0.0f;
  bool started_ = false;
};

}  // namespace gauge
