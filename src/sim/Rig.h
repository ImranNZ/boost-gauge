#pragma once

#include <string_view>

#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "ManualClock.h"
#include "MemorySettingsStore.h"
#include "ScriptedInput.h"
#include "SoftCanvas.h"
#include "ThrottledSource.h"
#include "app/GaugeApp.h"
#include "app/SettingsController.h"
#include "app/SettingsMenu.h"
#include "app/Setup.h"
#include "gauge/ChannelProfile.h"
#include "looks/ClassicLook.h"
#include "looks/MinimalLook.h"
#include "looks/ModernLook.h"
#include "looks/MotorsportLook.h"
#include "looks/RetroLook.h"
#include "looks/classic/ClassicLookFactory.h"
#include "looks/menu/SettingsMenuFactory.h"
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
struct Rig : gauge::ISetupTarget {
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
  gauge::SoftCanvas canvas;

  gauge::ClassicLook classic{looks::makeClassic()};
  gauge::RetroLook retro{looks::makeRetro()};
  gauge::MinimalLook minimal{looks::makeMinimal()};
  gauge::MotorsportLook motorsport{looks::makeMotorsport()};
  gauge::ModernLook modern{looks::makeModern()};
  gauge::GaugeApp app{link, canvas, clock, retro, gauge::GaugeApp::Options{}};

  // What is shown, and what the code-drawn looks are drawn with (Retro and Modern are pictures of
  // a psi boost dial). Like the firmware, only the shown channel is registered with the sampler.
  gauge::LookId look = gauge::LookId::Retro;
  gauge::ChannelKind channel = gauge::ChannelKind::Boost;
  gauge::GaugeProfile profile = gauge::defaultProfile(gauge::ChannelKind::Boost);

  // The settings menu: buttons -> menu -> store and this rig, which shows the choice.
  gauge::ScriptedInput buttons;
  gauge::MemorySettingsStore store;
  gauge::SettingsMenu menu{looks::makeSettingsMenu(gauge::Setup{})};
  gauge::SettingsController settings{buttons, menu, store, *this};

  /// A screen of any size: the looks are drawn for 240x240 and scaled to it (see ScaledSurface).
  explicit Rig(int width = kScreen, int height = kScreen) : canvas(width, height, kSupersample) {
    // Every channel is on offer, so the car lists them all (the default ECU lists only boost's
    // PIDs).
    ecu.setSupported(gauge::obd::kPidCoolantTemp, true);
    ecu.setSupported(gauge::obd::kPidIntakeAirTemp, true);
    ecu.setSupported(gauge::obd::kPidControlModuleVolts, true);
    sampler.add(channels.get(channel));
    app.setOverlay(&menu);
  }

  Rig(const Rig&) = delete;
  Rig& operator=(const Rig&) = delete;

  bool showingBoost() const { return channel == gauge::ChannelKind::Boost; }

  /// What is shown now.
  gauge::Setup setup() const { return gauge::Setup{look, channel, profile.unit}; }

  /// Show a setup: switch the look and the profile and, if the channel changed, read that one
  /// instead. (A boost-dial look cannot show another channel, so those give way to Classic.)
  void apply(const gauge::Setup& wanted) override {
    const gauge::Setup next = gauge::normalised(wanted);
    const bool channelChanged = next.channel != channel;
    look = next.look;
    channel = next.channel;
    profile = gauge::profileOf(next);
    menu.setCurrent(next);

    if (channelChanged) {
      sampler.clear();
      sampler.add(channels.get(channel));
      polling.setChannel(0);
      link.reset();  // do not serve the old channel's last reading once more
      app.reset();   // and start the value, its peak and its easing over
    }
    rebuildLooks();
  }

  /// Switch the look by name. False if there is no such look, or if it is a picture of a boost dial
  /// and the gauge is showing something else.
  bool selectLook(std::string_view name) {
    gauge::LookId wanted = gauge::LookId::Retro;
    if (!gauge::parseLookId(name, wanted)) return false;
    if (gauge::lookNeedsBoost(wanted) && !showingBoost()) return false;
    apply(gauge::Setup{wanted, channel, profile.unit});
    return true;
  }

  /// Switch the unit (psi, bar, kpa, c, f or v) the code-drawn looks show. False if there is no
  /// such unit, or the channel cannot be shown in it.
  bool selectUnit(std::string_view name) {
    gauge::Unit unit = gauge::Unit::Psi;
    gauge::GaugeProfile unused;
    if (!gauge::parseUnit(name, unit) || !gauge::profileFor(channel, unit, unused)) return false;
    apply(gauge::Setup{look, channel, unit});
    return true;
  }

  /// Show another channel (boost, coolant, intake or voltage), in its default unit. False if there
  /// is no such channel.
  bool selectChannel(std::string_view name) {
    gauge::ChannelKind kind = gauge::ChannelKind::Boost;
    if (!gauge::parseChannelKind(name, kind)) return false;
    apply(gauge::Setup{look, kind, gauge::defaultUnit(kind)});
    return true;
  }

  /// Press the gauge's button: the menu opens on a select, and so on.
  void press(gauge::InputEvent event) { buttons.push(event); }

  /// Open the menu on the given choice ("look", "gauge" or "unit"). False if there is none.
  bool openMenu(std::string_view field) {
    int selects = 0;
    if (field == "look")
      selects = 1;
    else if (field == "gauge")
      selects = 2;
    else if (field == "unit")
      selects = 3;
    else
      return false;
    for (int i = 0; i < selects; ++i) press(gauge::InputEvent::Select);
    return true;
  }

  const gauge::ILook& currentLook() const {
    switch (look) {
      case gauge::LookId::Classic:
        return classic;
      case gauge::LookId::Minimal:
        return minimal;
      case gauge::LookId::Motorsport:
        return motorsport;
      case gauge::LookId::Modern:
        return modern;
      case gauge::LookId::Retro:
      default:
        return retro;
    }
  }

  /// Rebuild the code-drawn looks from the profile, and have the app redraw the cached dial.
  void rebuildLooks() {
    classic = looks::makeClassic(profile);
    minimal = looks::makeMinimal(profile);
    motorsport = looks::makeMotorsport(profile);
    app.setLook(currentLook());
  }

  /// Run one app iteration and advance simulated time by one frame.
  void step() {
    settings.tick();
    app.tick();
    clock.advance(kFrameMs);
  }
};

}  // namespace sim
