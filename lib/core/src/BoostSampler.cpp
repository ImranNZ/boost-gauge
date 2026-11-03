#include "BoostSampler.h"

#include "Units.h"

namespace gauge {

BoostSampler::BoostSampler(IObdClient& client, IClock& clock)
    : BoostSampler(client, clock, Options{}) {}

BoostSampler::BoostSampler(IObdClient& client, IClock& clock, Options options)
    : client_(client), clock_(clock), options_(options), baroKpa_(kStdAtmosphereKpa) {}

BoostReading BoostSampler::sample() {
  if (!linked_) {
    tryConnect();
    if (!linked_) return {};
  }

  refreshBaroIfDue();

  ObdData map;
  if (!client_.readPid(obd::kModeCurrentData, obd::kPidMapKpa, map) || map.count < 1) {
    if (++consecutiveFailures_ >= options_.failuresBeforeReconnect) linked_ = false;
    return {};
  }
  consecutiveFailures_ = 0;
  return {boostPsi(static_cast<float>(map.bytes[0]), baroKpa_), true};
}

void BoostSampler::tryConnect() {
  const uint32_t now = clock_.nowMs();
  if (everTried_ && now - lastConnectAttemptMs_ < options_.reconnectIntervalMs) return;
  everTried_ = true;
  lastConnectAttemptMs_ = now;
  linked_ = client_.begin();
  consecutiveFailures_ = 0;
  haveBaro_ = false;  // ambient may have changed while we were away
  baroTried_ = false;
}

void BoostSampler::refreshBaroIfDue() {
  const uint32_t now = clock_.nowMs();
  const uint32_t interval = haveBaro_ ? options_.baroRefreshMs : options_.baroRetryMs;
  if (baroTried_ && now - baroAtMs_ < interval) return;
  baroTried_ = true;
  baroAtMs_ = now;  // throttles retries when the ECU lacks this PID

  ObdData baro;
  if (client_.readPid(obd::kModeCurrentData, obd::kPidBaroKpa, baro) && baro.count >= 1) {
    baroKpa_ = static_cast<float>(baro.bytes[0]);
    haveBaro_ = true;
  }
  // else: keep the previous value (standard atmosphere on first run).
}

}  // namespace gauge
