#pragma once

#include <cmath>
#include <cstdint>

namespace gauge {

/// Moves a displayed value toward a target with an exponential approach, so a
/// needle glides between sparse readings. Frame-rate independent: the value
/// closes ~63% of the gap every `timeConstantMs`, however often it is called.
/// A time constant of 0 (or less) disables easing: the value snaps.
class Easer {
 public:
  explicit Easer(float timeConstantMs) : tau_(timeConstantMs) {}

  float update(float target, uint32_t nowMs) {
    if (!primed_ || tau_ <= 0.0f) {
      value_ = target;  // first value, or easing off: no lag
      primed_ = true;
    } else {
      const float dt = static_cast<float>(nowMs - lastMs_);
      value_ += (target - value_) * (1.0f - std::exp(-dt / tau_));
    }
    lastMs_ = nowMs;
    return value_;
  }

  float value() const { return value_; }

 private:
  float tau_;
  float value_ = 0.0f;
  uint32_t lastMs_ = 0;
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
