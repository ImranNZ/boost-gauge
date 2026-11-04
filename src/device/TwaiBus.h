#pragma once

#include "obd/ICanBus.h"

namespace device {

/// ICanBus on the ESP32-S3's built-in TWAI (CAN) controller at 500 kbit/s.
/// Needs an external transceiver (e.g. SN65HVD230) between the pins and the car.
class TwaiBus : public gauge::ICanBus {
 public:
  bool begin() override;
  bool send(const gauge::CanFrame& frame) override;
  bool receive(gauge::CanFrame& out, uint32_t timeoutMs) override;

 private:
  bool started_ = false;
};

}  // namespace device
