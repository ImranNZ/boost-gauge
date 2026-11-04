#pragma once

#include <WiFi.h>

#include "obd/IByteStream.h"

namespace device {

/// IByteStream to a WiFi ELM327 dongle: join its access point, open a TCP
/// socket to the adapter. (While joined, the ESP32 has no other internet.)
class WifiElmStream : public gauge::IByteStream {
 public:
  bool connect() override;
  bool connected() const override;
  size_t write(const uint8_t* data, size_t len) override;
  int read() override;

 private:
  mutable WiFiClient client_;
};

}  // namespace device
