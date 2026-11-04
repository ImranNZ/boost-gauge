#pragma once

#include <cstddef>
#include <cstdint>

namespace gauge {

/// Duplex byte pipe to an ELM327 adapter (BLE, WiFi/TCP, or a fake).
class IByteStream {
 public:
  virtual ~IByteStream() = default;
  /// Establish the link. May block. Safe to call again to reconnect.
  virtual bool connect() = 0;
  virtual bool connected() const = 0;
  virtual size_t write(const uint8_t* data, size_t len) = 0;
  /// Next received byte, or -1 if nothing is available. Never blocks.
  virtual int read() = 0;
};

}  // namespace gauge
