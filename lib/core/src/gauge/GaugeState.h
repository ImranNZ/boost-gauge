#pragma once

#include <cstdint>

namespace gauge {

/// What a look needs to draw one frame.
struct GaugeState {
  float psi = 0.0f;
  float peakPsi = 0.0f;
  bool linked = false;
  int fps = 0;  // frames per second to show in the corner; 0 hides it
  uint32_t timeMs = 0;  // the frame's time, for looks that animate (e.g. a pulsing light)
};

}  // namespace gauge
