#pragma once

// The one place that knows which link to the car this build uses, chosen by the
// -DTRANSPORT_* flag in platformio.ini. Each variant is a `device::Transport` that
// owns its adapter plus the OBD client built on it and exposes just the client, so
// the rest of the firmware (main.cpp included) never mentions a concrete transport.
//
// Adding a transport: write its adapter in src/device/, add a branch here and an
// environment in platformio.ini. Nothing else changes.

#include "base/IClock.h"
#include "obd/IObdClient.h"

#if defined(TRANSPORT_CAN)

#include "device/TwaiBus.h"
#include "obd/CanObdClient.h"

namespace device {
class Transport {
 public:
  explicit Transport(gauge::IClock& clock) : client_(bus_, clock) {}
  gauge::IObdClient& client() { return client_; }

 private:
  TwaiBus bus_;
  gauge::CanObdClient client_;
};
}  // namespace device

#elif defined(TRANSPORT_BLE)

#include "config.h"
#include "device/BleElmStream.h"
#include "obd/Elm327Client.h"

namespace device {
class Transport {
 public:
  explicit Transport(gauge::IClock& clock) : client_(stream_, clock, options()) {}
  gauge::IObdClient& client() { return client_; }

 private:
  static gauge::Elm327Client::Options options() {
    gauge::Elm327Client::Options options;
    options.protocol = config::kElmProtocol;
    return options;
  }

  BleElmStream stream_;
  gauge::Elm327Client client_;
};
}  // namespace device

#elif defined(TRANSPORT_WIFI)

#include "config.h"
#include "device/WifiElmStream.h"
#include "obd/Elm327Client.h"

namespace device {
class Transport {
 public:
  explicit Transport(gauge::IClock& clock) : client_(stream_, clock, options()) {}
  gauge::IObdClient& client() { return client_; }

 private:
  static gauge::Elm327Client::Options options() {
    gauge::Elm327Client::Options options;
    options.protocol = config::kElmProtocol;
    return options;
  }

  WifiElmStream stream_;
  gauge::Elm327Client client_;
};
}  // namespace device

#elif defined(TRANSPORT_DEMO)

#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "obd/CanObdClient.h"

namespace device {
/// No car: a simulated engine computer driving itself through a scripted cycle.
class Transport {
 public:
  explicit Transport(gauge::IClock& clock) : ecu_(clock), bus_(ecu_), client_(bus_, clock) {}
  gauge::IObdClient& client() { return client_; }

 private:
  gauge::FakeEcu ecu_;
  gauge::FakeCanBus bus_;
  gauge::CanObdClient client_;
};
}  // namespace device

#else
#error "Select a transport: -DTRANSPORT_CAN | _BLE | _WIFI | _DEMO (see platformio.ini)"
#endif
