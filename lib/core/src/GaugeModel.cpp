#include "GaugeModel.h"

namespace gauge {

GaugeModel::GaugeModel() : GaugeModel(Options{}) {}

GaugeModel::GaugeModel(Options options)
    : options_(options), easer_(options.smoothingMs), peak_(options.peakHoldMs) {}

void GaugeModel::update(const Sample& sample, uint32_t nowMs) {
  // A repeated sample is not news: the source may be slower than the display.
  if (sample.seq != lastSeq_) {
    lastSeq_ = sample.seq;
    if (sample.reading.valid) {
      target_ = sample.reading.psi;
      lastValidMs_ = nowMs;
      everValid_ = true;
    }
  }

  // Ease every frame, not every sample, so the needle moves smoothly between them.
  if (everValid_) {
    state_.psi = easer_.update(target_, nowMs);
    state_.peakPsi = peak_.update(state_.psi, nowMs);
  }
  state_.linked = everValid_ && (nowMs - lastValidMs_) < options_.staleAfterMs;

  state_.timeMs = nowMs;

  fps_.frame(nowMs);
  state_.fps = options_.showFps ? static_cast<int>(fps_.fps() + 0.5f) : 0;
}

}  // namespace gauge
