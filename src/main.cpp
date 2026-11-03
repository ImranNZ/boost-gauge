// Composition root: the only place that knows which concrete classes are used.
// Everything else depends on interfaces from lib/core.
//
// Two threads: a sampler task talks to the car and publishes into a mailbox;
// loop() (the render side) reads the latest reading and draws. They share only
// the mailbox, so a slow link cannot slow the display.

#include <Arduino.h>

#include "BoostSampler.h"
#include "GaugeApp.h"
#include "ReadingMailbox.h"
#include "SamplerLoop.h"
#include "config.h"
#include "device/ArduinoClock.h"
#include "device/LgfxCanvas.h"
#include "device/SamplerTask.h"

#if defined(TRANSPORT_CAN)
#include "CanObdClient.h"
#include "device/TwaiBus.h"
#elif defined(TRANSPORT_BLE)
#include "Elm327Client.h"
#include "device/BleElmStream.h"
#elif defined(TRANSPORT_WIFI)
#include "Elm327Client.h"
#include "device/WifiElmStream.h"
#elif defined(TRANSPORT_DEMO)
#include "CanObdClient.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#else
#error "Select a transport: -DTRANSPORT_CAN | _BLE | _WIFI | _DEMO (see platformio.ini)"
#endif

namespace {

device::ArduinoClock clock;
device::LgfxCanvas canvas;

#if defined(TRANSPORT_CAN)
device::TwaiBus bus;
gauge::CanObdClient obdClient(bus, clock);
#elif defined(TRANSPORT_BLE)
device::BleElmStream stream;
gauge::Elm327Client obdClient(stream, clock);
#elif defined(TRANSPORT_WIFI)
device::WifiElmStream stream;
gauge::Elm327Client obdClient(stream, clock);
#elif defined(TRANSPORT_DEMO)
gauge::FakeEcu ecu(clock);
gauge::FakeCanBus bus(ecu);
gauge::CanObdClient obdClient(bus, clock);
#endif

// Producer side (sampler task).
gauge::BoostSampler sampler(obdClient, clock);
gauge::ReadingMailbox mailbox;
gauge::SamplerLoop samplerLoop(sampler, mailbox);
device::SamplerTask samplerTask(samplerLoop);

// Consumer side (loop()).
gauge::GaugeApp app(mailbox, canvas, clock, gauge::GaugeView{}, [] {
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
