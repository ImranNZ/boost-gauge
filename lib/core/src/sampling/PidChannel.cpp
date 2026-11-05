#include "sampling/PidChannel.h"

#include "obd/PidDecode.h"

namespace gauge {

float PidChannel::coolantC(const ObdData& data) { return obd::coolantTempC(data); }
float PidChannel::intakeAirC(const ObdData& data) { return obd::intakeAirTempC(data); }
float PidChannel::moduleVolts(const ObdData& data) { return obd::controlModuleVolts(data); }

void PidChannel::onLinked(const PidSupport& support) {
  // Only a car that answered the discovery, and does not list this PID, is spared the request.
  available_ = !(support.knows(pid_) && !support.supports(pid_));
}

ReadStatus PidChannel::read(IObdClient& client, float& value) {
  if (!available_) return ReadStatus::Unavailable;

  ObdData data;
  if (!client.readPid(obd::kModeCurrentData, pid_, data) || data.count < minBytes_) {
    return ReadStatus::Failed;
  }
  value = decode_(data);
  return ReadStatus::Ok;
}

}  // namespace gauge
