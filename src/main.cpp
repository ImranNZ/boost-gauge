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
#include "ReadingMailbox.h"
#include "SamplerLoop.h"
#include "config.h"
#include "device/ArduinoClock.h"
#include "device/LgfxCanvas.h"
#include "device/SamplerTask.h"
#include "device/Transport.h"
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

// Consumer side (loop()).
gauge::GaugeApp app(mailbox, canvas, clock,
                    config::kUseRetroLook ? static_cast<const gauge::ILook&>(retroLook)
                                          : static_cast<const gauge::ILook&>(classicLook),
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
