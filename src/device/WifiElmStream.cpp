#include "device/WifiElmStream.h"

#include <Arduino.h>

#include "config.h"

namespace device {
namespace {
constexpr uint32_t kJoinTimeoutMs = 8000;
constexpr uint32_t kSocketTimeoutMs = 3000;
}  // namespace

bool WifiElmStream::connect() {
  client_.stop();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config::kWifiSsid, config::kWifiPassword);
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - start > kJoinTimeoutMs) return false;
      delay(50);
    }
  }
  client_.setTimeout(kSocketTimeoutMs / 1000);
  client_.setNoDelay(true);
  return client_.connect(config::kElmHost, config::kElmPort, kSocketTimeoutMs);
}

bool WifiElmStream::connected() const { return client_.connected(); }

size_t WifiElmStream::write(const uint8_t* data, size_t len) { return client_.write(data, len); }

int WifiElmStream::read() { return client_.read(); }  // -1 when nothing is waiting

}  // namespace device
