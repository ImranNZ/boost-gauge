#include "sampling/ReadingMailbox.h"

#include <algorithm>
#include <cmath>

namespace gauge {
namespace {

constexpr uint32_t kValidBit = 1u << 16;
constexpr int kSeqShift = 17;
constexpr uint32_t kSeqMask = 0x7FFFu;

}  // namespace

void ReadingMailbox::publish(const Reading& reading) {
  const long centi = std::clamp(std::lround(reading.value * 100.0f), -32768L, 32767L);
  const uint32_t valueBits = static_cast<uint16_t>(static_cast<int16_t>(centi));

  const uint32_t seq = ((word_.load(std::memory_order_relaxed) >> kSeqShift) + 1u) & kSeqMask;
  const uint32_t word = (seq << kSeqShift) | (reading.valid ? kValidBit : 0u) | valueBits;
  word_.store(word, std::memory_order_release);
}

Sample ReadingMailbox::latest() {
  const uint32_t word = word_.load(std::memory_order_acquire);
  Sample sample;
  sample.seq = static_cast<uint16_t>(word >> kSeqShift);
  sample.reading.valid = (word & kValidBit) != 0;
  sample.reading.value = static_cast<float>(static_cast<int16_t>(word & 0xFFFFu)) / 100.0f;
  return sample;
}

}  // namespace gauge
