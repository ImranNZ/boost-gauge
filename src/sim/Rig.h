#pragma once

#include "BoostSampler.h"
#include "CanObdClient.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "GaugeApp.h"
#include "ManualClock.h"
#include "SoftCanvas.h"

namespace sim {

/// The whole gauge stack wired against mocks: fake ECU -> fake CAN bus ->
/// OBD client -> sampler -> app -> software canvas. Mirrors src/main.cpp.
struct Rig {
  static constexpr int kFrameMs = 33;  // ~30 fps of simulated time
  static constexpr int kScreen = 240;

  gauge::ManualClock clock;
  gauge::FakeEcu ecu{clock};
  gauge::FakeCanBus bus{ecu};
  gauge::CanObdClient obdClient{bus, clock};
  gauge::BoostSampler sampler{obdClient, clock};
  gauge::SoftCanvas canvas{kScreen, kScreen};
  gauge::GaugeApp app{sampler, canvas, clock};

  /// Run one app iteration and advance simulated time by one frame.
  void step() {
    app.tick();
    clock.advance(kFrameMs);
  }
};

}  // namespace sim
