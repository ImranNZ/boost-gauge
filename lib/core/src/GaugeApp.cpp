#include "GaugeApp.h"

namespace gauge {

GaugeApp::GaugeApp(BoostSampler& sampler, ICanvas& canvas, IClock& clock)
    : GaugeApp(sampler, canvas, clock, GaugeView{}, Options{}) {}

GaugeApp::GaugeApp(BoostSampler& sampler, ICanvas& canvas, IClock& clock, GaugeView view,
                   Options options)
    : sampler_(sampler),
      canvas_(canvas),
      clock_(clock),
      view_(view),
      options_(options),
      smoother_(options.smoothingAlpha),
      peak_(options.peakHoldMs) {}

void GaugeApp::tick() {
  const BoostReading reading = sampler_.sample();
  const uint32_t now = clock_.nowMs();

  if (reading.valid) {
    state_.psi = smoother_.update(reading.psi);
    state_.peakPsi = peak_.update(state_.psi, now);
    lastValidMs_ = now;
    everValid_ = true;
  }
  state_.linked = everValid_ && (now - lastValidMs_) < options_.staleAfterMs;

  view_.draw(canvas_, state_);
  canvas_.present();
}

}  // namespace gauge
