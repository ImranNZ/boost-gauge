#pragma once

#include <NimBLEDevice.h>

#include "IByteStream.h"
#include "SpscRing.h"

namespace device {

/// IByteStream to a BLE ELM327 dongle. Finds the dongle by advertised name,
/// then uses whichever UART-style service it exposes (FFF0, FFE0 or 18F0):
/// a notify characteristic for replies and a write characteristic for commands.
class BleElmStream : public gauge::IByteStream {
 public:
  bool connect() override;
  bool connected() const override;
  size_t write(const uint8_t* data, size_t len) override;
  int read() override;

 private:
  bool bindUartService(NimBLEClient& client);
  void onNotify(const uint8_t* data, size_t len);

  NimBLEClient* client_ = nullptr;
  NimBLERemoteCharacteristic* tx_ = nullptr;  // we write commands here
  gauge::SpscRing<512> rx_;                   // filled from the BLE task
};

}  // namespace device
