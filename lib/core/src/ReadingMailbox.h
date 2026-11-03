#pragma once

#include <atomic>
#include <cstdint>

#include "ReadingSource.h"

namespace gauge {

/// Hands the latest reading from the sampler task to the render loop.
///
/// The whole state is one 32-bit word, so a single atomic load/store moves it
/// with no lock and no torn reads (lock-free on the ESP32). Packing:
///   bits 31..17  sequence (15 bits, wraps)
///   bit  16      valid
///   bits 15..0   psi in hundredths, signed (+-327 psi; sensor steps are ~0.15)
class ReadingMailbox : public IReadingSource {
 public:
  void publish(const BoostReading& reading);
  Sample latest() override;

 private:
  std::atomic<uint32_t> word_{0};  // only the single producer writes it
};

}  // namespace gauge
