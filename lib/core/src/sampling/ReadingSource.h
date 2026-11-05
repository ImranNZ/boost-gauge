#pragma once

#include <cstddef>
#include <cstdint>

#include "sampling/ChannelSampler.h"
#include "sampling/Reading.h"

namespace gauge {

/// A reading plus a counter that changes whenever a new one arrives, so a
/// consumer polling faster than the producer can tell fresh from repeated.
struct Sample {
  Reading reading;
  uint16_t seq = 0;  // wraps; 0 before the first reading
};

/// Where the screen gets its data. The screen never waits on the car: it asks
/// for whatever is newest and carries on drawing.
class IReadingSource {
 public:
  virtual ~IReadingSource() = default;
  virtual Sample latest() = 0;
};

/// Synchronous source: every call lets the sampler do one read and blocks until the car answers.
/// Simple and deterministic (tests, the PC sim); the firmware uses the mailboxes. It reports one
/// channel; a call that read something else (or nothing) repeats the last reading, unchanged.
class PollingSource : public IReadingSource {
 public:
  PollingSource(ChannelSampler& sampler, size_t channel) : sampler_(sampler), channel_(channel) {}

  /// Follow another channel. What was last read is forgotten, and the sequence moves on so a
  /// consumer notices the change.
  void setChannel(size_t channel) {
    channel_ = channel;
    last_ = Reading{};
    ++seq_;
  }

  Sample latest() override {
    ChannelSampler::Step step;
    if (sampler_.step(step) && step.channel == channel_) {
      last_ = step.reading;
      ++seq_;
    }
    return {last_, seq_};
  }

 private:
  ChannelSampler& sampler_;
  size_t channel_;
  Reading last_;
  uint16_t seq_ = 0;
};

}  // namespace gauge
