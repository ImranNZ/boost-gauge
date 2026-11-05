#pragma once

#include <cstdint>

#include "base/IClock.h"
#include "sampling/ReadingSource.h"

namespace gauge {

/// Wraps a source so fresh readings arrive at most once per interval, the way a
/// real link does (CAN ~25 Hz, a dongle ~10 Hz). Between readings the same sample
/// (same seq) is returned, exactly like the mailbox does while the sampler is busy.
class ThrottledSource : public IReadingSource {
 public:
  ThrottledSource(IReadingSource& inner, const IClock& clock, uint32_t intervalMs)
      : inner_(inner), clock_(clock), intervalMs_(intervalMs) {}

  void setIntervalMs(uint32_t intervalMs) { intervalMs_ = intervalMs; }
  uint32_t intervalMs() const { return intervalMs_; }

  /// Forget the cached sample: the next call fetches a fresh one (for when the inner source
  /// switched to something else, so the old reading is not served once more).
  void reset() { primed_ = false; }

  Sample latest() override {
    const uint32_t now = clock_.nowMs();
    if (!primed_ || now - lastFetchMs_ >= intervalMs_) {
      cached_ = inner_.latest();
      lastFetchMs_ = now;
      primed_ = true;
    }
    return cached_;
  }

 private:
  IReadingSource& inner_;
  const IClock& clock_;
  uint32_t intervalMs_;
  uint32_t lastFetchMs_ = 0;
  bool primed_ = false;
  Sample cached_;
};

}  // namespace gauge
