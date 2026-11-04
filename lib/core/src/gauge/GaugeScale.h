#pragma once

#include <algorithm>

namespace gauge {

/// Maps a pressure to a dial angle: a 270 degree sweep that opens at the bottom,
/// starting bottom-left and running clockwise (0 deg = 3 o'clock).
struct GaugeScale {
  static constexpr float kStartDeg = 135.0f;
  static constexpr float kSweepDeg = 270.0f;

  float minPsi = -15.0f;
  float maxPsi = 25.0f;

  float angleFor(float psi) const {
    const float clamped = std::clamp(psi, minPsi, maxPsi);
    return kStartDeg + (clamped - minPsi) / (maxPsi - minPsi) * kSweepDeg;
  }
};

}  // namespace gauge
