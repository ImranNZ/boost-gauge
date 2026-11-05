#pragma once

#include <string_view>

#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "ManualClock.h"
#include "SoftCanvas.h"
#include "ThrottledSource.h"
#include "app/GaugeApp.h"
#include "looks/ClassicLook.h"
#include "looks/MinimalLook.h"
#include "looks/ModernLook.h"
#include "looks/MotorsportLook.h"
#include "looks/RetroLook.h"
#include "looks/minimal/MinimalLookFactory.h"
#include "looks/modern/ModernLookFactory.h"
#include "looks/motorsport/MotorsportLookFactory.h"
#include "looks/retro/RetroLookFactory.h"
#include "obd/CanObdClient.h"
#include "sampling/BoostChannel.h"
#include "sampling/ChannelSampler.h"
#include "sampling/ReadingSource.h"

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
  gauge::BoostChannel boost{obdClient, clock};
  gauge::ChannelSampler sampler{obdClient, clock};
  gauge::PollingSource polling{sampler, 0};
  // Readings arrive at the link's rate, not the display's: ~CAN speed by default.
  gauge::ThrottledSource link{polling, clock, 40};
  gauge::SoftCanvas canvas{kScreen, kScreen, kSupersample};

  gauge::ClassicLook classic;
  gauge::RetroLook retro{looks::makeRetro()};
  gauge::MinimalLook minimal{looks::makeMinimal()};
  gauge::MotorsportLook motorsport{looks::makeMotorsport()};
  gauge::ModernLook modern{looks::makeModern()};
  gauge::GaugeApp app{link, canvas, clock, retro, gauge::GaugeApp::Options{}};

  // What the code-drawn looks are drawn with; Retro and Modern are psi pictures.
  gauge::GaugeProfile profile;
  const gauge::ILook* current = &retro;

  Rig() { sampler.add(boost); }

  /// Switch the look by name. False if there is no such look.
  bool selectLook(std::string_view name) {
    if (name == "retro") {
      current = &retro;
    } else if (name == "classic") {
      current = &classic;
    } else if (name == "minimal") {
      current = &minimal;
    } else if (name == "motorsport") {
      current = &motorsport;
    } else if (name == "modern") {
      current = &modern;
    } else {
      return false;
    }
    app.setLook(*current);
    return true;
  }

  /// Switch the unit (psi, bar or kpa) the code-drawn looks show. False if there is no such unit.
  bool selectUnit(std::string_view name) {
    if (name == "psi") {
      profile = gauge::GaugeProfile::psi();
    } else if (name == "bar") {
      profile = gauge::GaugeProfile::bar();
    } else if (name == "kpa") {
      profile = gauge::GaugeProfile::kpa();
    } else {
      return false;
    }
    classic = gauge::ClassicLook(profile);
    minimal = looks::makeMinimal(profile);
    motorsport = looks::makeMotorsport(profile);
    app.setLook(*current);  // the looks changed in place: make the app redraw the cached dial
    return true;
  }

  /// Run one app iteration and advance simulated time by one frame.
  void step() {
    app.tick();
    clock.advance(kFrameMs);
  }
};

}  // namespace sim
