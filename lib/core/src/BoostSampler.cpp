#include "BoostSampler.h"

#include "Units.h"

namespace gauge {

BoostSampler::BoostSampler(IObdClient& client, IClock& clock)
    : BoostSampler(client, clock, Options{}) {}

BoostSampler::BoostSampler(IObdClient& client, IClock& clock, Options options)
    : client_(client), link_(client, clock, options.link), ambient_(client, clock, options.ambient) {}

BoostReading BoostSampler::sample() {
  if (!link_.ensureLinked()) return {};

  if (link_.connectionCount() != seenConnections_) {
    seenConnections_ = link_.connectionCount();
    ambient_.invalidate();  // ambient may have changed while we were away
  }
  const float ambientKpa = ambient_.kpa();

  ObdData map;
  if (!client_.readPid(obd::kModeCurrentData, obd::kPidMapKpa, map) || map.count < 1) {
    link_.noteFailure();
    return {};
  }
  link_.noteSuccess();
  return {boostPsi(static_cast<float>(map.bytes[0]), ambientKpa), true};
}

}  // namespace gauge
