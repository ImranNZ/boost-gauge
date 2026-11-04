#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "sampling/ChannelSampler.h"
#include "sampling/ReadingSource.h"

namespace gauge {

/// Hands the latest reading from the sampler task to the render loop.
///
/// The whole state is one 32-bit word, so a single atomic load/store moves it
/// with no lock and no torn reads (lock-free on the ESP32). Packing:
///   bits 31..17  sequence (15 bits, wraps)
///   bit  16      valid
///   bits 15..0   the value in hundredths, signed (+-327: psi, degrees or volts alike)
class ReadingMailbox : public IReadingSource {
 public:
  void publish(const Reading& reading);
  Sample latest() override;

 private:
  std::atomic<uint32_t> word_{0};  // only the single producer writes it
};

/// One mailbox per channel, indexed in the order the channels were added to the ChannelSampler.
/// The sampler task publishes into them and the render loop reads one: they share nothing else,
/// so the render path still never waits.
class ChannelMailboxes {
 public:
  /// Publish a channel's reading. A channel beyond the last slot is ignored.
  void publish(size_t channel, const Reading& reading) {
    if (channel < ChannelSampler::kMaxChannels) boxes_[channel].publish(reading);
  }

  /// Where the screen reads one channel (the last slot, if `channel` is beyond it).
  IReadingSource& source(size_t channel) {
    return boxes_[std::min(channel, ChannelSampler::kMaxChannels - 1)];
  }

 private:
  std::array<ReadingMailbox, ChannelSampler::kMaxChannels> boxes_;
};

}  // namespace gauge
