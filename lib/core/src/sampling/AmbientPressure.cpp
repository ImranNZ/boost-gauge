#include "sampling/AmbientPressure.h"

namespace gauge {

AmbientPressure::AmbientPressure(IObdClient& client, IClock& clock)
    : AmbientPressure(client, clock, Options{}) {}

AmbientPressure::AmbientPressure(IObdClient& client, IClock& clock, Options options)
    : client_(client), clock_(clock), options_(options) {}

void AmbientPressure::setAvailable(bool available) {
  available_ = available;
  if (!available) {
    kpa_ = kStdAtmosphereKpa;
    known_ = false;
  }
}

float AmbientPressure::kpa() {
  if (!available_) return kpa_;

  const uint32_t now = clock_.nowMs();
  const uint32_t interval = known_ ? options_.refreshMs : options_.retryMs;
  if (tried_ && now - triedAtMs_ < interval) return kpa_;

  tried_ = true;
  triedAtMs_ = now;  // throttles retries when the ECU lacks this PID

  ObdData reply;
  if (client_.readPid(obd::kModeCurrentData, obd::kPidBaroKpa, reply) && reply.count >= 1) {
    kpa_ = static_cast<float>(reply.bytes[0]);
    known_ = true;
  }
  // else: keep the previous value (standard atmosphere on first run).
  return kpa_;
}

void AmbientPressure::invalidate() {
  known_ = false;
  tried_ = false;
}

}  // namespace gauge
