#pragma once

#include <cstdint>

namespace gauge {

/// Time source. Abstracted so logic can run against a manual clock in tests.
class IClock {
 public:
  virtual ~IClock() = default;
  virtual uint32_t nowMs() const = 0;
  /// Block (or yield) for roughly `ms`. Fakes advance their own time instead.
  virtual void delayMs(uint32_t ms) = 0;
};

}  // namespace gauge
