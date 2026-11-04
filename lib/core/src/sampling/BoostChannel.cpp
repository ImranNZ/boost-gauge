#include "sampling/BoostChannel.h"

#include "obd/Units.h"

namespace gauge {

BoostChannel::BoostChannel(IObdClient& client, IClock& clock)
    : BoostChannel(client, clock, AmbientPressure::Options{}) {}

BoostChannel::BoostChannel(IObdClient& client, IClock& clock,
                           const AmbientPressure::Options& ambient)
    : ambient_(client, clock, ambient) {}

void BoostChannel::onLinked(const PidSupport& support) {
  // Only a car that answered the discovery, and does not list a PID, is spared the request.
  ambient_.setAvailable(!(support.knows(obd::kPidBaroKpa) && !support.supports(obd::kPidBaroKpa)));
  ambient_.invalidate();  // ambient may have changed while we were away
  manifoldAvailable_ = !(support.knows(obd::kPidMapKpa) && !support.supports(obd::kPidMapKpa));
}

ReadStatus BoostChannel::read(IObdClient& client, float& value) {
  // A car that says it has no manifold pressure cannot show boost; asking would only fail.
  if (!manifoldAvailable_) return ReadStatus::Unavailable;

  const float ambientKpa = ambient_.kpa();

  ObdData map;
  if (!client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, map) || map.count < 1) {
    return ReadStatus::Failed;
  }
  value = boostPsi(static_cast<float>(map.bytes[0]), ambientKpa);
  return ReadStatus::Ok;
}

}  // namespace gauge
