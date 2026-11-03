#pragma once

namespace gauge {

/// What a look needs to draw one frame.
struct GaugeState {
  float psi = 0.0f;
  float peakPsi = 0.0f;
  bool linked = false;
  int fps = 0;  // frames per second to show in the corner; 0 hides it
};

}  // namespace gauge
