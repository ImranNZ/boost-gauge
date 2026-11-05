#include "app/GaugeApp.h"

namespace gauge {

GaugeApp::GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook& look,
                   Options options)
    : source_(source),
      clock_(clock),
      look_(&look),
      options_(options),
      model_(options),
      renderer_(canvas) {}

void GaugeApp::setLook(const ILook& look) {
  look_ = &look;
  renderer_.invalidate();
}

void GaugeApp::reset() {
  model_ = GaugeModel(options_);
  renderer_.invalidate();
}

void GaugeApp::tick() {
  model_.update(source_.latest(), clock_.nowMs());
  renderer_.render(*look_, model_.state());
}

}  // namespace gauge
