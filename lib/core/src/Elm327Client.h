#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "IByteStream.h"
#include "IClock.h"
#include "IObdClient.h"

namespace gauge {

/// OBD client speaking the ELM327 AT/text protocol over any IByteStream.
/// The same class serves BLE and WiFi dongles.
class Elm327Client : public IObdClient {
 public:
  struct Options {
    uint32_t resetTimeoutMs = 2000;
    uint32_t commandTimeoutMs = 1000;
    uint32_t probeTimeoutMs = 6000;  // first ECU query triggers bus init
    uint32_t replyTimeoutMs = 500;
  };

  Elm327Client(IByteStream& stream, IClock& clock);
  Elm327Client(IByteStream& stream, IClock& clock, Options options);

  bool begin() override;
  bool readPid(uint8_t mode, uint8_t pid, ObdData& out) override;

 private:
  bool query(uint8_t mode, uint8_t pid, ObdData& out, uint32_t timeoutMs);
  /// Send `command` + CR and collect text until the '>' prompt. False on timeout.
  bool transact(const char* command, uint32_t timeoutMs);

  IByteStream& stream_;
  IClock& clock_;
  Options options_;
  std::array<char, 128> reply_{};
  size_t replyLen_ = 0;
};

}  // namespace gauge
