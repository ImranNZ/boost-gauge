#pragma once

#include <cstdint>

#include "obd/PidDecode.h"
#include "sampling/Channel.h"

namespace gauge {

/// A channel that is one PID decoded by a pure function: coolant temperature, intake air
/// temperature, battery voltage. A car that says it does not implement the PID is never asked.
class PidChannel : public IChannel {
 public:
  using Decoder = float (*)(const ObdData&);

  /// `minBytes` is how many data bytes the decoder needs; a shorter answer is a failure.
  PidChannel(uint8_t pid, uint8_t minBytes, Decoder decode, uint32_t periodMs)
      : pid_(pid), minBytes_(minBytes), decode_(decode), periodMs_(periodMs) {}

  /// The standard slow channels: read about once a second.
  static PidChannel coolant() { return make(obd::kPidCoolantTemp, 1, &coolantC); }
  static PidChannel intakeAir() { return make(obd::kPidIntakeAirTemp, 1, &intakeAirC); }
  static PidChannel voltage() { return make(obd::kPidControlModuleVolts, 2, &moduleVolts); }

  uint32_t periodMs() const override { return periodMs_; }
  void onLinked(const PidSupport& support) override;
  ReadStatus read(IObdClient& client, float& value) override;

 private:
  static constexpr uint32_t kSlowPeriodMs = 1000;
  static PidChannel make(uint8_t pid, uint8_t bytes, Decoder decode) {
    return PidChannel(pid, bytes, decode, kSlowPeriodMs);
  }
  static float coolantC(const ObdData& data);
  static float intakeAirC(const ObdData& data);
  static float moduleVolts(const ObdData& data);

  uint8_t pid_;
  uint8_t minBytes_;
  Decoder decode_;
  uint32_t periodMs_;
  bool available_ = true;
};

}  // namespace gauge
