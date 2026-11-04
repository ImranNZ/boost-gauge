#pragma once

#include <string>
#include <utility>
#include <vector>

#include "obd/IByteStream.h"

namespace gauge {

/// Adapter double that answers the Nth command with the Nth canned reply and
/// then goes silent. Lets tests force timeouts, errors and oversize replies.
class ScriptedByteStream : public IByteStream {
 public:
  explicit ScriptedByteStream(std::vector<std::string> replies) : replies_(std::move(replies)) {}

  bool connect() override {
    connected_ = true;
    return true;
  }
  bool connected() const override { return connected_; }

  size_t write(const uint8_t* data, size_t len) override {
    for (size_t i = 0; i < len; ++i) {
      const char c = static_cast<char>(data[i]);
      if (c != '\r') {
        line_ += c;
        continue;
      }
      commands_.push_back(line_);
      line_.clear();
      if (next_ < replies_.size()) out_ += replies_[next_++];
    }
    return len;
  }

  int read() override {
    if (outPos_ >= out_.size()) return -1;
    return static_cast<unsigned char>(out_[outPos_++]);
  }

  const std::vector<std::string>& commands() const { return commands_; }

 private:
  std::vector<std::string> replies_;
  std::vector<std::string> commands_;
  std::string line_;
  std::string out_;
  size_t outPos_ = 0;
  size_t next_ = 0;
  bool connected_ = false;
};

}  // namespace gauge
