#pragma once

#include <cstdint>

namespace gauge {

/// What a look needs to draw one frame. `value` is the channel's reading in its canonical unit
/// (psi for boost, degrees Celsius for a temperature, volts for a voltage); a look converts it
/// for display with its GaugeProfile.
struct GaugeState {
  float value = 0.0f;
  float peakValue = 0.0f;
  bool linked = false;
  int fps = 0;          // frames per second to show in the corner; 0 hides it
  uint32_t timeMs = 0;  // the frame's time, for looks that animate (e.g. a pulsing light)
};

}  // namespace gauge
