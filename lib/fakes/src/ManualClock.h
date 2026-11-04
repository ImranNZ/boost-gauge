#pragma once

#include <cstdint>

#include "base/IClock.h"

namespace gauge {

/// Clock that only moves when told to. delayMs() advances it, so polling loops
/// with timeouts terminate deterministically.
class ManualClock : public IClock {
 public:
  uint32_t nowMs() const override { return nowMs_; }
  void delayMs(uint32_t ms) override { nowMs_ += ms; }
  void advance(uint32_t ms) { nowMs_ += ms; }

 private:
  uint32_t nowMs_ = 0;
};

}  // namespace gauge
