#pragma once

#include <deque>
#include <vector>

#include "FakeEcu.h"
#include "ICanBus.h"

namespace gauge {

/// In-memory CAN bus with a FakeEcu on the other end. Responds to functional
/// OBD requests (0x7DF) with a single-frame reply from 0x7E8.
class FakeCanBus : public ICanBus {
 public:
  explicit FakeCanBus(const FakeEcu& ecu) : ecu_(ecu) {}

  bool begin() override {
    started_ = true;
    return true;
  }
  bool send(const CanFrame& frame) override;
  bool receive(CanFrame& out, uint32_t timeoutMs) override;

  /// Inject an unrelated frame, as if from other bus traffic.
  void injectNoise(const CanFrame& frame) { inbox_.push_back(frame); }
  const std::vector<CanFrame>& sent() const { return sent_; }

 private:
  const FakeEcu& ecu_;
  bool started_ = false;
  std::deque<CanFrame> inbox_;
  std::vector<CanFrame> sent_;
};

}  // namespace gauge
