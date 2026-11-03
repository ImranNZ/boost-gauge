#pragma once

#include <cstdint>

#include "FpsCounter.h"
#include "GaugeView.h"
#include "ICanvas.h"
#include "IClock.h"
#include "ReadingSource.h"
#include "Smoothing.h"

namespace gauge {

/// Render loop body: take the newest reading, ease the needle toward it, draw.
/// Call tick() as fast as the display allows. It never waits on the car.
class GaugeApp {
 public:
  struct Options {
    float smoothingMs = 80.0f;     // needle easing time constant; 0 = no easing
    uint32_t peakHoldMs = 5000;
    uint32_t staleAfterMs = 1500;  // no valid reading for this long => "NO LINK"
    bool showFps = false;
  };

  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock);
  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, GaugeView view,
           Options options);

  void tick();

 private:
  IReadingSource& source_;
  ICanvas& canvas_;
  IClock& clock_;
  GaugeView view_;
  Options options_;

  Easer easer_;
  PeakHold peak_;
  FpsCounter fps_;
  GaugeState state_;
  float target_ = 0.0f;       // newest valid reading
  uint16_t lastSeq_ = 0;      // last reading we have looked at
  bool everValid_ = false;
  uint32_t lastValidMs_ = 0;
};

}  // namespace gauge
