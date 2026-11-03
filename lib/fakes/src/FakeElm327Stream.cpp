#include "FakeElm327Stream.h"

#include <cctype>
#include <cstdio>

namespace gauge {

size_t FakeElm327Stream::write(const uint8_t* data, size_t len) {
  if (!connected_) return 0;
  for (size_t i = 0; i < len; ++i) {
    const char c = static_cast<char>(data[i]);
    if (c == '\r') {
      handleCommand(line_);
      line_.clear();
    } else if (c != '\n') {
      line_ += c;
    }
  }
  return len;
}

int FakeElm327Stream::read() {
  if (!connected_ || outPos_ >= out_.size()) {
    out_.clear();
    outPos_ = 0;
    return -1;
  }
  return static_cast<unsigned char>(out_[outPos_++]);
}

void FakeElm327Stream::handleCommand(std::string command) {
  std::string cmd;
  for (char c : command) {
    if (c != ' ') cmd += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }

  if (echo_) out_ += command + "\r";

  if (cmd.rfind("AT", 0) == 0) {
    if (cmd == "ATZ") {
      echo_ = true;
      searched_ = false;
      out_ += "\r\rELM327 v1.5\r\r>";
      return;
    }
    if (cmd == "ATE0") echo_ = false;
    if (cmd == "ATE1") echo_ = true;
    out_ += "OK\r\r>";
    return;
  }

  unsigned mode = 0;
  unsigned pid = 0;
  if (cmd.size() == 4 && std::sscanf(cmd.c_str(), "%2x%2x", &mode, &pid) == 2) {
    ObdData data;
    if (!ecu_.answer(static_cast<uint8_t>(mode), static_cast<uint8_t>(pid), data)) {
      out_ += "NO DATA\r\r>";
      return;
    }
    if (!searched_) {
      out_ += "SEARCHING...\r";
      searched_ = true;
    }
    char hex[8];
    std::snprintf(hex, sizeof(hex), "%02X %02X", mode + 0x40u, pid);
    out_ += hex;
    for (uint8_t i = 0; i < data.count; ++i) {
      std::snprintf(hex, sizeof(hex), " %02X", data.bytes[i]);
      out_ += hex;
    }
    out_ += "\r\r>";
    return;
  }

  out_ += "?\r\r>";
}

}  // namespace gauge
