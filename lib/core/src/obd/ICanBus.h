#pragma once

#include <array>
#include <cstdint>

namespace gauge {

struct CanFrame {
  uint32_t id = 0;  // 11-bit identifier
  std::array<uint8_t, 8> data{};
  uint8_t len = 0;
};

/// Raw classic-CAN access (500 kbit/s on the OBD port).
class ICanBus {
 public:
  virtual ~ICanBus() = default;
  virtual bool begin() = 0;
  virtual bool send(const CanFrame& frame) = 0;
  /// Wait up to `timeoutMs` for any frame. False on timeout.
  virtual bool receive(CanFrame& out, uint32_t timeoutMs) = 0;
};

}  // namespace gauge
