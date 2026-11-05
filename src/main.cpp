// Composition root: the only place that wires concrete classes together.
// Everything else depends on interfaces from lib/core.
//
// Two threads: a sampler task talks to the car and publishes into a mailbox;
// loop() (the render side) reads the latest reading and draws. They share only
// the mailbox, so a slow link cannot slow the display.

#include <Arduino.h>

#include "app/GaugeApp.h"
#include "config.h"
#include "device/ArduinoClock.h"
#include "device/LgfxCanvas.h"
#include "device/PidLog.h"
#include "device/SamplerTask.h"
#include "device/Transport.h"
#include "looks/ClassicLook.h"
#include "looks/MinimalLook.h"
#include "looks/ModernLook.h"
#include "looks/MotorsportLook.h"
#include "looks/minimal/MinimalLookFactory.h"
#include "looks/modern/ModernLookFactory.h"
#include "looks/motorsport/MotorsportLookFactory.h"
#include "looks/retro/RetroLookFactory.h"
#include "sampling/ChannelSampler.h"
#include "sampling/ReadingMailbox.h"
#include "sampling/SamplerLoop.h"
#include "sampling/StandardChannels.h"

namespace {

device::ArduinoClock clock;
device::LgfxCanvas canvas;

// Producer side (sampler task).
device::Transport transport(clock);
gauge::StandardChannels channels(transport.client(), clock);
gauge::ChannelSampler sampler(transport.client(), clock, [] {
  gauge::ChannelSampler::Options options;
  if (config::kLogPidSupport) options.onDiscovered = device::logPidSupport;
  return options;
}());
gauge::ChannelMailboxes mailboxes;
gauge::SamplerLoop samplerLoop(sampler, mailboxes);
device::SamplerTask samplerTask(samplerLoop);

// Looks (the app keeps a reference, so they live for the whole run).
gauge::ClassicLook classicLook(config::kProfile);
gauge::RetroLook retroLook = looks::makeRetro();
gauge::MinimalLook minimalLook = looks::makeMinimal(config::kProfile);
gauge::MotorsportLook motorsportLook = looks::makeMotorsport(config::kProfile);
gauge::ModernLook modernLook = looks::makeModern();

// Retro and Modern are pictures of a psi boost dial: they cannot show another channel.
static_assert(config::kChannel == gauge::ChannelKind::Boost ||
                  (config::kLook != config::Look::Retro && config::kLook != config::Look::Modern),
              "Retro and Modern only show boost: set config::kLook to Classic, Minimal or "
              "Motorsport, or config::kChannel to Boost");

const gauge::ILook& chosenLook() {
  switch (config::kLook) {
    case config::Look::Classic:
      return classicLook;
    case config::Look::Minimal:
      return minimalLook;
    case config::Look::Motorsport:
      return motorsportLook;
    case config::Look::Modern:
      return modernLook;
    case config::Look::Retro:
    default:
      return retroLook;
  }
}

// Consumer side (loop()).
gauge::GaugeApp app(mailboxes.source(0), canvas, clock, chosenLook(), [] {
  gauge::GaugeApp::Options options;
  options.showFps = config::kShowFps;
  return options;
}());

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!canvas.begin()) {
    Serial.println("display: frame buffer allocation failed");
    for (;;) delay(1000);
  }
  sampler.add(channels.get(config::kChannel));  // before the sampler task starts
  samplerTask.start();
}

void loop() { app.tick(); }
