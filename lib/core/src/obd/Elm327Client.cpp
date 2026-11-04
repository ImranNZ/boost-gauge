#include "obd/Elm327Client.h"

#include <cstdio>
#include <cstring>
#include <string_view>

#include "obd/Elm327Parse.h"

namespace gauge {

Elm327Client::Elm327Client(IByteStream& stream, IClock& clock)
    : Elm327Client(stream, clock, Options{}) {}

Elm327Client::Elm327Client(IByteStream& stream, IClock& clock, Options options)
    : stream_(stream), clock_(clock), options_(options) {}

bool Elm327Client::begin() {
  if (!stream_.connect()) return false;

  if (!transact("ATZ", options_.resetTimeoutMs)) return false;
  // Echo off, linefeeds off, headers off, ISO 15765-4 CAN 11-bit 500 kbaud.
  static constexpr const char* kSetup[] = {"ATE0", "ATL0", "ATH0", "ATSP6"};
  for (const char* command : kSetup) {
    if (!transact(command, options_.commandTimeoutMs)) return false;
  }

  ObdData probe;
  return query(obd::kModeCurrentData, obd::kPidSupported01To20, probe, options_.probeTimeoutMs);
}

bool Elm327Client::readPid(uint8_t mode, uint8_t pid, ObdData& out) {
  return query(mode, pid, out, options_.replyTimeoutMs);
}

bool Elm327Client::query(uint8_t mode, uint8_t pid, ObdData& out, uint32_t timeoutMs) {
  char command[8];
  std::snprintf(command, sizeof(command), "%02X%02X", mode, pid);
  if (!transact(command, timeoutMs)) return false;
  return parseObdReply(std::string_view(reply_.data(), replyLen_), mode, pid, out);
}

bool Elm327Client::transact(const char* command, uint32_t timeoutMs) {
  if (!stream_.connected()) return false;

  while (stream_.read() >= 0) {
    // Discard stale bytes from a previous, timed-out exchange.
  }
  stream_.write(reinterpret_cast<const uint8_t*>(command), std::strlen(command));
  stream_.write(reinterpret_cast<const uint8_t*>("\r"), 1);

  replyLen_ = 0;
  const uint32_t start = clock_.nowMs();
  while (clock_.nowMs() - start < timeoutMs) {
    const int c = stream_.read();
    if (c < 0) {
      clock_.delayMs(1);
      continue;
    }
    if (c == '>') return true;
    if (replyLen_ < reply_.size()) reply_[replyLen_++] = static_cast<char>(c);
  }
  return false;
}

}  // namespace gauge
