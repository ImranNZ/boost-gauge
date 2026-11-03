#pragma once

#include <cstdint>

#include "FpsCounter.h"
#include "GaugeState.h"
#include "ReadingSource.h"
#include "Smoothing.h"

namespace gauge {

/// Turns raw readings into what the screen shows: the eased needle position,
/// the held peak, whether the link is alive, and the frame rate. Pure logic: no
/// drawing, no clock of its own (time is passed in), so it is trivially testable.
class GaugeModel {
 public:
  struct Options {
    float smoothingMs = 80.0f;     // needle easing time constant; 0 = no easing
    uint32_t peakHoldMs = 5000;
    uint32_t staleAfterMs = 1500;  // no valid reading for this long => "NO LINK"
    bool showFps = false;
  };

  GaugeModel();
  explicit GaugeModel(Options options);

  /// Fold in the newest sample (a repeat of the last one is ignored) and advance
  /// the easing to `nowMs`. Call once per frame.
  void update(const Sample& sample, uint32_t nowMs);

  const GaugeState& state() const { return state_; }

 private:
  Options options_;
  Easer easer_;
  PeakHold peak_;
  FpsCounter fps_;
  GaugeState state_;
  float target_ = 0.0f;   // newest valid reading
  uint16_t lastSeq_ = 0;  // last sample we have looked at
  bool everValid_ = false;
  uint32_t lastValidMs_ = 0;
};

}  // namespace gauge
