#include "sampling/BoostSampler.h"

#include "obd/Units.h"

namespace gauge {

BoostSampler::BoostSampler(IObdClient& client, IClock& clock)
    : BoostSampler(client, clock, Options{}) {}

BoostSampler::BoostSampler(IObdClient& client, IClock& clock, Options options)
    : client_(client),
      options_(options),
      link_(client, clock, options.link),
      ambient_(client, clock, options.ambient) {}

BoostReading BoostSampler::sample() {
  if (!link_.ensureLinked()) return {};

  if (link_.connectionCount() != seenConnections_) {
    seenConnections_ = link_.connectionCount();
    support_.discover(client_);
    // Only a car that answered, and does not list barometric pressure, is spared the request.
    ambient_.setAvailable(
        !(support_.knows(obd::kPidBaroKpa) && !support_.supports(obd::kPidBaroKpa)));
    ambient_.invalidate();  // ambient may have changed while we were away
    if (options_.onDiscovered != nullptr) options_.onDiscovered(support_);
  }

  // A car that says it has no manifold pressure cannot show boost; asking would only fail.
  if (support_.knows(obd::kPidMapKpa) && !support_.supports(obd::kPidMapKpa)) return {};

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
