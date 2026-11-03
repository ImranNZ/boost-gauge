// Composition root: the only place that wires concrete classes together.
// Everything else depends on interfaces from lib/core.
//
// Two threads: a sampler task talks to the car and publishes into a mailbox;
// loop() (the render side) reads the latest reading and draws. They share only
// the mailbox, so a slow link cannot slow the display.

#include <Arduino.h>

#include "BoostSampler.h"
#include "ClassicLook.h"
#include "GaugeApp.h"
#include "MinimalLook.h"
#include "MotorsportLook.h"
#include "ReadingMailbox.h"
#include "SamplerLoop.h"
#include "config.h"
#include "device/ArduinoClock.h"
#include "device/LgfxCanvas.h"
#include "device/SamplerTask.h"
#include "device/Transport.h"
#include "looks/MinimalLookFactory.h"
#include "looks/MotorsportLookFactory.h"
#include "looks/RetroLookFactory.h"

namespace {

device::ArduinoClock clock;
device::LgfxCanvas canvas;

// Producer side (sampler task).
device::Transport transport(clock);
gauge::BoostSampler sampler(transport.client(), clock);
gauge::ReadingMailbox mailbox;
gauge::SamplerLoop samplerLoop(sampler, mailbox);
device::SamplerTask samplerTask(samplerLoop);

// Looks (the app keeps a reference, so they live for the whole run).
gauge::ClassicLook classicLook;
gauge::RetroLook retroLook = looks::makeRetro();
gauge::MinimalLook minimalLook = looks::makeMinimal();
gauge::MotorsportLook motorsportLook = looks::makeMotorsport();

const gauge::ILook& chosenLook() {
  switch (config::kLook) {
    case config::Look::Classic:
      return classicLook;
    case config::Look::Minimal:
      return minimalLook;
    case config::Look::Motorsport:
      return motorsportLook;
    case config::Look::Retro:
    default:
      return retroLook;
  }
}

// Consumer side (loop()).
gauge::GaugeApp app(mailbox, canvas, clock, chosenLook(),
                    [] {
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
  samplerTask.start();
}

void loop() { app.tick(); }
