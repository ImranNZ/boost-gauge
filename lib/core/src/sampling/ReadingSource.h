#pragma once

#include <cstdint>

#include "sampling/BoostSampler.h"

namespace gauge {

/// A reading plus a counter that changes whenever a new one arrives, so a
/// consumer polling faster than the producer can tell fresh from repeated.
struct Sample {
  BoostReading reading;
  uint16_t seq = 0;  // wraps; 0 before the first reading
};

/// Where the screen gets its data. The screen never waits on the car: it asks
/// for whatever is newest and carries on drawing.
class IReadingSource {
 public:
  virtual ~IReadingSource() = default;
  virtual Sample latest() = 0;
};

/// Synchronous source: every call samples the car and blocks until it answers.
/// Simple and deterministic (tests, the PC sim); the firmware uses the mailbox.
class PollingSource : public IReadingSource {
 public:
  explicit PollingSource(BoostSampler& sampler) : sampler_(sampler) {}

  Sample latest() override {
    const BoostReading reading = sampler_.sample();
    return {reading, ++seq_};
  }

 private:
  BoostSampler& sampler_;
  uint16_t seq_ = 0;
};

}  // namespace gauge
