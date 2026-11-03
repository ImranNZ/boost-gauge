#pragma once

#include "BoostSampler.h"
#include "CanObdClient.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "GaugeApp.h"
#include "ManualClock.h"
#include "ReadingSource.h"
#include "SoftCanvas.h"
#include "ThrottledSource.h"

namespace sim {

/// The whole gauge stack wired against mocks: fake ECU -> fake CAN bus ->
/// OBD client -> sampler -> app -> software canvas. Mirrors src/main.cpp.
struct Rig {
  static constexpr int kFrameMs = 33;  // ~30 fps of simulated time
  static constexpr int kScreen = 240;
  static constexpr int kSupersample = 4;  // anti-aliasing: draw 4x, box-filter down

  gauge::ManualClock clock;
  gauge::FakeEcu ecu{clock};
  gauge::FakeCanBus bus{ecu};
  gauge::CanObdClient obdClient{bus, clock};
  gauge::BoostSampler sampler{obdClient, clock};
  gauge::PollingSource polling{sampler};
  // Readings arrive at the link's rate, not the display's: ~CAN speed by default.
  gauge::ThrottledSource link{polling, clock, 40};
  gauge::SoftCanvas canvas{kScreen, kScreen, kSupersample};
  gauge::GaugeApp app{link, canvas, clock};

  /// Run one app iteration and advance simulated time by one frame.
  void step() {
    app.tick();
    clock.advance(kFrameMs);
  }
};

}  // namespace sim
