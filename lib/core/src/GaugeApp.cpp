#include "GaugeApp.h"

namespace gauge {

GaugeApp::GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock)
    : GaugeApp(source, canvas, clock, static_cast<const ILook*>(nullptr), Options{}) {}

GaugeApp::GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook& look,
                   Options options)
    : GaugeApp(source, canvas, clock, &look, options) {}

GaugeApp::GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook* look,
                   Options options)
    : source_(source),
      canvas_(canvas),
      clock_(clock),
      look_(look != nullptr ? look : &defaultLook_),
      options_(options),
      easer_(options.smoothingMs),
      peak_(options.peakHoldMs) {}

void GaugeApp::setLook(const ILook& look) {
  look_ = &look;
  canvas_.discardBackground();
}

void GaugeApp::tick() {
  const Sample sample = source_.latest();
  const uint32_t now = clock_.nowMs();

  // A repeated sample is not news: the source may be slower than the display.
  if (sample.seq != lastSeq_) {
    lastSeq_ = sample.seq;
    if (sample.reading.valid) {
      target_ = sample.reading.psi;
      lastValidMs_ = now;
      everValid_ = true;
    }
  }

  // Ease every frame, not every sample, so the needle moves smoothly between them.
  if (everValid_) {
    state_.psi = easer_.update(target_, now);
    state_.peakPsi = peak_.update(state_.psi, now);
  }
  state_.linked = everValid_ && (now - lastValidMs_) < options_.staleAfterMs;

  fps_.frame(now);
  state_.fps = options_.showFps ? static_cast<int>(fps_.fps() + 0.5f) : 0;

  look_->draw(canvas_, state_);
  canvas_.present();
}

}  // namespace gauge
