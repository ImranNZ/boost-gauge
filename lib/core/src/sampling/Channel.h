#pragma once

#include <cstdint>

#include "obd/IObdClient.h"
#include "obd/PidSupport.h"

namespace gauge {

/// How one attempt to read a channel went.
enum class ReadStatus {
  Ok,           // a value was read
  Failed,       // the car did not answer properly: this counts against the link
  Unavailable,  // the car does not offer this reading: nothing wrong with the link
};

/// Something the gauge can show that comes from the car: boost, a temperature, a voltage.
/// A channel knows which PIDs it needs, how to turn the answers into a value, and how often
/// that value is worth reading. The ChannelSampler decides when; the channel decides how.
class IChannel {
 public:
  virtual ~IChannel() = default;

  /// How long to leave between reads, in ms. 0 reads as often as the link allows.
  virtual uint32_t periodMs() const = 0;

  /// The link has just come up and the car's PIDs have been discovered: forget what was
  /// learned before and take note of what this car supports.
  virtual void onLinked(const PidSupport& support) = 0;

  /// Read the channel once. `value` is set only when the result is Ok.
  virtual ReadStatus read(IObdClient& client, float& value) = 0;
};

}  // namespace gauge
