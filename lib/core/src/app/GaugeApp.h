#pragma once

#include "base/IClock.h"
#include "gauge/GaugeModel.h"
#include "looks/FrameRenderer.h"
#include "looks/ILook.h"
#include "render/ICanvas.h"
#include "render/IOverlay.h"
#include "sampling/ReadingSource.h"

namespace gauge {

/// The render loop body: take the newest reading, update the model, draw a frame.
/// A thin coordinator; the logic lives in GaugeModel and the drawing in FrameRenderer.
/// Call tick() as fast as the display allows. It never waits on the car.
class GaugeApp {
 public:
  using Options = GaugeModel::Options;

  /// The look must outlive the app.
  GaugeApp(IReadingSource& source, ICanvas& canvas, IClock& clock, const ILook& look,
           Options options);
  GaugeApp(IReadingSource&, ICanvas&, IClock&, ILook&&, Options) = delete;  // would dangle

  /// Switch look. The cached dial belongs to the old one, so it is thrown away.
  void setLook(const ILook& look);
  void setLook(ILook&&) = delete;

  /// Start the displayed value over (peak hold, easing, link state): the channel it shows has
  /// changed.
  void reset();

  /// Draw `overlay` (a settings menu) over the gauge while it is active. It must outlive the app;
  /// nullptr removes it.
  void setOverlay(const IOverlay* overlay) { overlay_ = overlay; }

  void tick();

 private:
  IReadingSource& source_;
  IClock& clock_;
  const ILook* look_;
  const IOverlay* overlay_ = nullptr;
  Options options_;
  GaugeModel model_;
  FrameRenderer renderer_;
};

}  // namespace gauge
