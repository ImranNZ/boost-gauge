#pragma once

#include <Arduino.h>

#include "IClock.h"

namespace device {

class ArduinoClock : public gauge::IClock {
 public:
  uint32_t nowMs() const override { return millis(); }
  void delayMs(uint32_t ms) override { delay(ms); }
};

}  // namespace device
