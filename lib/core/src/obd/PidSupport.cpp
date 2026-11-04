#include "obd/PidSupport.h"

namespace gauge {
namespace {

// The support PIDs are 00, 20, 40, 60, 80, A0 and C0: seven ranges of 32 PIDs, up to PID E0.
constexpr int kRanges = 7;

}  // namespace

bool PidSupport::discover(IObdClient& client) {
  bits_.fill(0);
  knownRanges_ = 0;

  for (int range = 0; range < kRanges; ++range) {
    const uint8_t base = static_cast<uint8_t>(range * 32);
    ObdData data;
    if (!client.readPid(obd::kModeCurrentData, base, data) || data.count < 4) {
      return knownRanges_ != 0;
    }
    knownRanges_ |= 1u << range;

    // Bit k counted from the top of the 32-bit word is PID base + 1 + k.
    for (int k = 0; k < 32; ++k) {
      if ((data.bytes[k / 8] >> (7 - k % 8)) & 1) set(static_cast<uint8_t>(base + 1 + k));
    }

    // The last bit is PID base + 32, the next support PID: clear means there are no more ranges,
    // so those are known to be empty rather than unknown.
    if ((data.bytes[3] & 1) == 0) {
      for (int later = range + 1; later < kRanges; ++later) knownRanges_ |= 1u << later;
      break;
    }
  }
  return true;
}

bool PidSupport::knows(uint8_t pid) const {
  if (pid == 0) return knownRanges_ != 0;  // PID 00 itself: every car answers it
  return (knownRanges_ >> rangeOf(pid)) & 1;
}

bool PidSupport::supports(uint8_t pid) const {
  if (pid == 0) return knownRanges_ != 0;
  return (bits_[pid / 8] >> (pid % 8)) & 1;
}

int PidSupport::count() const {
  int total = 0;
  for (uint8_t byte : bits_) {
    for (; byte != 0; byte &= static_cast<uint8_t>(byte - 1)) ++total;
  }
  return total;
}

}  // namespace gauge
