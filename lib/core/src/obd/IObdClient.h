#pragma once

#include <array>
#include <cstdint>

namespace gauge {

namespace obd {
constexpr uint8_t kModeCurrentData = 0x01;
constexpr uint8_t kPidSupported01To20 = 0x00;
constexpr uint8_t kPidMapKpa = 0x0B;   // intake manifold absolute pressure, 1 kPa/bit
constexpr uint8_t kPidBaroKpa = 0x33;  // barometric pressure, 1 kPa/bit
}  // namespace obd

/// Data bytes (A, B, C, D) of an OBD-II response, without mode/PID echo.
struct ObdData {
  std::array<uint8_t, 4> bytes{};
  uint8_t count = 0;
};

/// Request/response access to OBD-II PIDs, independent of the physical link.
class IObdClient {
 public:
  virtual ~IObdClient() = default;
  /// Bring the link up. Idempotent; call again to recover after a failure.
  virtual bool begin() = 0;
  virtual bool readPid(uint8_t mode, uint8_t pid, ObdData& out) = 0;
};

}  // namespace gauge
