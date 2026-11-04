#pragma once

#include <string>

#include "FakeEcu.h"
#include "obd/IByteStream.h"

namespace gauge {

/// Behaves like an ELM327 adapter: echo, AT commands, "SEARCHING...", "NO DATA"
/// and the '>' prompt, backed by a FakeEcu.
class FakeElm327Stream : public IByteStream {
 public:
  explicit FakeElm327Stream(const FakeEcu& ecu) : ecu_(ecu) {}

  bool connect() override {
    connected_ = !refuseConnection_;
    return connected_;
  }
  bool connected() const override { return connected_; }
  size_t write(const uint8_t* data, size_t len) override;
  int read() override;

  void setRefuseConnection(bool refuse) { refuseConnection_ = refuse; }
  void dropLink() { connected_ = false; }

 private:
  void handleCommand(const std::string& command);

  const FakeEcu& ecu_;
  bool connected_ = false;
  bool refuseConnection_ = false;
  bool echo_ = true;
  bool searched_ = false;
  std::string line_;
  std::string out_;
  size_t outPos_ = 0;
};

}  // namespace gauge
