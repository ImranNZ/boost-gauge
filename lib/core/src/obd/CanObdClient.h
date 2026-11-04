#pragma once

#include <cstdint>

#include "base/IClock.h"
#include "obd/ICanBus.h"
#include "obd/IObdClient.h"

namespace gauge {

/// OBD-II over ISO 15765-4 (11-bit CAN), single-frame requests/responses only,
/// which covers all the mode 01 PIDs this gauge needs.
class CanObdClient : public IObdClient {
 public:
  struct Options {
    uint32_t replyTimeoutMs = 100;
  };

  CanObdClient(ICanBus& bus, IClock& clock);
  CanObdClient(ICanBus& bus, IClock& clock, Options options);

  bool begin() override;
  bool readPid(uint8_t mode, uint8_t pid, ObdData& out) override;

 private:
  static constexpr uint32_t kRequestId = 0x7DF;  // functional broadcast
  static constexpr uint32_t kResponseIdMin = 0x7E8;
  static constexpr uint32_t kResponseIdMax = 0x7EF;

  ICanBus& bus_;
  IClock& clock_;
  Options options_;
};

}  // namespace gauge
