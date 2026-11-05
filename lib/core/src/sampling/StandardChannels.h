#pragma once

#include "base/IClock.h"
#include "obd/IObdClient.h"
#include "sampling/BoostChannel.h"
#include "sampling/Channel.h"
#include "sampling/ChannelKind.h"
#include "sampling/PidChannel.h"

namespace gauge {

/// One of each channel the gauge can show, ready to hand to a ChannelSampler. Only the channel
/// that is added to the sampler is ever read, so holding the others costs nothing on the bus.
class StandardChannels {
 public:
  StandardChannels(IObdClient& client, IClock& clock)
      : boost_(client, clock),
        coolant_(PidChannel::coolant()),
        intakeAir_(PidChannel::intakeAir()),
        voltage_(PidChannel::voltage()) {}

  IChannel& get(ChannelKind kind) {
    if (kind == ChannelKind::Coolant) return coolant_;
    if (kind == ChannelKind::IntakeAir) return intakeAir_;
    if (kind == ChannelKind::Voltage) return voltage_;
    return boost_;
  }

 private:
  BoostChannel boost_;
  PidChannel coolant_;
  PidChannel intakeAir_;
  PidChannel voltage_;
};

}  // namespace gauge
