#pragma once

#include <cstdint>

#include "FpsCounter.h"
#include "GaugeView.h"
#include "ICanvas.h"
#include "IClock.h"
#include "ILook.h"
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

  /// Uses the Classic look.
  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock);
  /// The look must outlive the app.
  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook& look,
           Options options);
  GaugeApp(IReadingSource&, ICanvas&, IClock&, ILook&&, Options) = delete;  // would dangle

  /// Switch look. The cached dial belongs to the old one, so it is thrown away.
  void setLook(const ILook& look);
  void setLook(ILook&&) = delete;

  void tick();

 private:
  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook* look,
           Options options);

  IReadingSource& source_;
  ICanvas& canvas_;
  IClock& clock_;
  GaugeView defaultLook_;
  const ILook* look_;
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
