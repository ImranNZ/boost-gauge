#pragma once

#include <cstdint>

#include "BoostSampler.h"
#include "GaugeView.h"
#include "ICanvas.h"
#include "IClock.h"
#include "Smoothing.h"

namespace gauge {

/// Application loop body: sample -> smooth -> render. Call tick() forever.
class GaugeApp {
 public:
  struct Options {
    float smoothingAlpha = 0.5f;
    uint32_t peakHoldMs = 5000;
    uint32_t staleAfterMs = 1500;  // no valid sample for this long => "NO LINK"
  };

  GaugeApp(BoostSampler& sampler, ICanvas& canvas, IClock& clock);
  GaugeApp(BoostSampler& sampler, ICanvas& canvas, IClock& clock, GaugeView view,
           Options options);

  void tick();

 private:
  BoostSampler& sampler_;
  ICanvas& canvas_;
  IClock& clock_;
  GaugeView view_;
  Options options_;

  Ema smoother_;
  PeakHold peak_;
  GaugeState state_;
  bool everValid_ = false;
  uint32_t lastValidMs_ = 0;
};

}  // namespace gauge
