#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "base/IClock.h"
#include "obd/IByteStream.h"
#include "obd/IObdClient.h"

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
    /// The adapter's bus protocol, as the digit after `ATSP`. "6" is ISO 15765-4 CAN, 11-bit IDs,
    /// 500 kbit/s (what the F56 uses, and the default). "0" lets the adapter search for whatever
    /// the car speaks. Others: 1 J1850 PWM, 2 J1850 VPW, 3 ISO 9141-2, 4 KWP slow, 5 KWP fast,
    /// 7 CAN 29-bit/500k, 8 CAN 11-bit/250k, 9 CAN 29-bit/250k. Must stay valid for the call.
    const char* protocol = "6";
  };

  Elm327Client(IByteStream& stream, IClock& clock);
  Elm327Client(IByteStream& stream, IClock& clock, const Options& options);

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
