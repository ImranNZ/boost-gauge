#pragma once

#include <Arduino.h>

#include "obd/IObdClient.h"
#include "obd/PidSupport.h"

namespace device {

/// Print which mode 01 PIDs the car reported to the serial console, once per connection. Pass it
/// as BoostSampler::Options::onDiscovered (see config::kLogPidSupport). This is the "PID
/// scanner": the quick way to see what another car offers before trusting the gauge on it.
inline void logPidSupport(const gauge::PidSupport& support) {
  if (!support.knows(0x00)) {
    Serial.println("PID discovery: the car did not answer PID 00");
    return;
  }

  Serial.printf("PID discovery: %d mode 01 PIDs reported:", support.count());
  for (int pid = 1; pid <= 0xE0; ++pid) {
    if (support.supports(static_cast<uint8_t>(pid))) Serial.printf(" %02X", pid);
  }
  Serial.println();

  const auto describe = [&support](uint8_t pid) {
    if (!support.knows(pid)) return "unknown (that range was not answered)";
    return support.supports(pid) ? "reported" : "not reported";
  };
  Serial.printf("  0B manifold pressure: %s\n", describe(gauge::obd::kPidMapKpa));
  Serial.printf("  33 barometric pressure: %s\n", describe(gauge::obd::kPidBaroKpa));
}

}  // namespace device
