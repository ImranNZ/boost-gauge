#pragma once

#include <cstdint>

namespace gauge {

/// Exponential moving average. alpha in (0,1]; higher = more responsive.
class Ema {
 public:
  explicit Ema(float alpha) : alpha_(alpha) {}

  float update(float x) {
    if (!primed_) {
      value_ = x;
      primed_ = true;
    } else {
      value_ += alpha_ * (x - value_);
    }
    return value_;
  }
  float value() const { return value_; }

 private:
  float alpha_;
  float value_ = 0.0f;
  bool primed_ = false;
};

/// Remembers the highest value seen, releasing it `holdMs` after it was set.
class PeakHold {
 public:
  explicit PeakHold(uint32_t holdMs) : holdMs_(holdMs) {}

  float update(float x, uint32_t nowMs) {
    if (!primed_ || x >= peak_ || nowMs - peakAtMs_ >= holdMs_) {
      peak_ = x;
      peakAtMs_ = nowMs;
      primed_ = true;
    }
    return peak_;
  }
  float value() const { return peak_; }

 private:
  uint32_t holdMs_;
  float peak_ = 0.0f;
  uint32_t peakAtMs_ = 0;
  bool primed_ = false;
};

}  // namespace gauge
