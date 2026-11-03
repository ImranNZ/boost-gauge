#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace gauge {

/// Lock-free single-producer/single-consumer byte ring. Used to hand bytes
/// from a radio callback (producer) to the polling loop (consumer).
/// Capacity is N-1 bytes; N must be a power of two.
template <size_t N>
class SpscRing {
  static_assert(N >= 2 && (N & (N - 1)) == 0, "N must be a power of two");

 public:
  bool push(uint8_t byte) {
    const size_t head = head_.load(std::memory_order_relaxed);
    const size_t next = (head + 1) & kMask;
    if (next == tail_.load(std::memory_order_acquire)) return false;  // full
    buf_[head] = byte;
    head_.store(next, std::memory_order_release);
    return true;
  }

  /// Oldest byte, or -1 when empty.
  int pop() {
    const size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return -1;
    const uint8_t byte = buf_[tail];
    tail_.store((tail + 1) & kMask, std::memory_order_release);
    return byte;
  }

 private:
  static constexpr size_t kMask = N - 1;
  std::array<uint8_t, N> buf_{};
  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};
};

}  // namespace gauge
