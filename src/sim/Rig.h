#pragma once

#include <string_view>

#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "ManualClock.h"
#include "SoftCanvas.h"
#include "ThrottledSource.h"
#include "app/GaugeApp.h"
#include "gauge/ChannelProfile.h"
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
#include "sampling/ChannelSampler.h"
#include "sampling/ReadingSource.h"
#include "sampling/StandardChannels.h"

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
  gauge::StandardChannels channels{obdClient, clock};
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

  // What is shown, and what the code-drawn looks are drawn with (Retro and Modern are pictures of
  // a psi boost dial). Like the firmware, only the shown channel is registered with the sampler.
  gauge::ChannelKind channel = gauge::ChannelKind::Boost;
  gauge::GaugeProfile profile = gauge::defaultProfile(gauge::ChannelKind::Boost);
  const gauge::ILook* current = &retro;

  Rig() {
    // Every channel is on offer, so the car lists them all (the default ECU lists only boost's
    // PIDs).
    ecu.setSupported(gauge::obd::kPidCoolantTemp, true);
    ecu.setSupported(gauge::obd::kPidIntakeAirTemp, true);
    ecu.setSupported(gauge::obd::kPidControlModuleVolts, true);
    sampler.add(channels.get(channel));
  }

  bool showingBoost() const { return channel == gauge::ChannelKind::Boost; }

  /// Switch the look by name. False if there is no such look, or if it is a picture of a boost dial
  /// and the gauge is showing something else.
  bool selectLook(std::string_view name) {
    if (!showingBoost() && (name == "retro" || name == "modern")) return false;
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

  /// Switch the unit (psi, bar, kpa, c, f or v) the code-drawn looks show. False if there is no
  /// such unit, or the channel cannot be shown in it.
  bool selectUnit(std::string_view name) {
    gauge::Unit unit = gauge::Unit::Psi;
    gauge::GaugeProfile chosen;
    if (!gauge::parseUnit(name, unit) || !gauge::profileFor(channel, unit, chosen)) return false;
    profile = chosen;
    rebuildLooks();
    return true;
  }

  /// Show another channel (boost, coolant, intake or voltage), in its default unit. A picture of a
  /// boost dial cannot show it, so those looks give way to Classic. False if there is no such
  /// channel.
  bool selectChannel(std::string_view name) {
    gauge::ChannelKind kind = gauge::ChannelKind::Boost;
    if (!gauge::parseChannelKind(name, kind)) return false;

    channel = kind;
    profile = gauge::defaultProfile(kind);
    if (!showingBoost() && (current == &retro || current == &modern)) current = &classic;

    sampler.clear();
    sampler.add(channels.get(kind));
    polling.setChannel(0);
    link.reset();  // do not serve the old channel's last reading once more
    app.reset();   // and start the value, its peak and its easing over
    rebuildLooks();
    return true;
  }

  /// Rebuild the code-drawn looks from the profile, and have the app redraw the cached dial.
  void rebuildLooks() {
    classic = gauge::ClassicLook(profile);
    minimal = looks::makeMinimal(profile);
    motorsport = looks::makeMotorsport(profile);
    app.setLook(*current);
  }

  /// Run one app iteration and advance simulated time by one frame.
  void step() {
    app.tick();
    clock.advance(kFrameMs);
  }
};

}  // namespace sim
