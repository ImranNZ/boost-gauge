#pragma once

#include <array>
#include <cstdint>

#include "obd/IObdClient.h"

namespace gauge {

/// Which mode 01 PIDs a car implements, discovered the standard way. PID 00 returns a 32-bit
/// big-endian bitmap for PIDs 01-20 (the top bit is PID 01, the bottom bit is PID 20); PID 20
/// returns the bitmap for 21-40, and so on. A range's last bit says whether the next range
/// exists, so the chain stops as soon as a car says there is no more.
///
/// A car that does not answer a range leaves it unknown, which is not the same as unsupported:
/// callers should only conclude "not supported" when knows(pid) is true.
class PidSupport {
 public:
  /// Read the bitmaps from the car, replacing what was known. True if at least the first range
  /// (PIDs 01-20) was read; a chain that breaks later keeps what it got.
  bool discover(IObdClient& client);

  /// Whether the range holding `pid` was read (or is known to be empty), so supports() is reliable.
  bool knows(uint8_t pid) const;

  /// Whether the car reported `pid`. Only meaningful when knows(pid) is true.
  bool supports(uint8_t pid) const;

  /// How many PIDs the car reported.
  int count() const;

 private:
  static int rangeOf(uint8_t pid) { return (pid - 1) / 32; }
  void set(uint8_t pid) { bits_[pid / 8] |= static_cast<uint8_t>(1u << (pid % 8)); }

  std::array<uint8_t, 32> bits_{};
  uint32_t knownRanges_ = 0;  // bit r: range r (PIDs 32r+1 .. 32r+32) is known
};

}  // namespace gauge
