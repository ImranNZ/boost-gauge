#include "device/BleElmStream.h"

#include <cstring>

#include "config.h"

namespace device {
namespace {

constexpr uint16_t kUartServices[] = {0xFFF0, 0xFFE0, 0x18F0};
constexpr uint32_t kScanSeconds = 5;

}  // namespace

bool BleElmStream::connect() {
  if (!NimBLEDevice::getInitialized()) NimBLEDevice::init("boost-gauge");

  if (client_ != nullptr) {
    NimBLEDevice::deleteClient(client_);
    client_ = nullptr;
    tx_ = nullptr;
  }

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  NimBLEScanResults results = scan->start(kScanSeconds, false);

  for (int i = 0; i < results.getCount(); ++i) {
    NimBLEAdvertisedDevice found = results.getDevice(i);  // returned by value in 1.4.x
    if (found.getName().find(config::kBleNameHint) == std::string::npos) continue;

    NimBLEClient* client = NimBLEDevice::createClient();
    if (client->connect(&found) && bindUartService(*client)) {
      client_ = client;
      return true;
    }
    NimBLEDevice::deleteClient(client);
  }
  return false;
}

bool BleElmStream::bindUartService(NimBLEClient& client) {
  for (uint16_t uuid : kUartServices) {
    NimBLERemoteService* service = client.getService(NimBLEUUID(uuid));
    if (service == nullptr) continue;

    NimBLERemoteCharacteristic* notify = nullptr;
    NimBLERemoteCharacteristic* write = nullptr;
    for (NimBLERemoteCharacteristic* c : *service->getCharacteristics(true)) {
      if (notify == nullptr && c->canNotify()) notify = c;
      if (write == nullptr && (c->canWrite() || c->canWriteNoResponse())) write = c;
    }
    if (notify == nullptr || write == nullptr) continue;

    const bool subscribed =
        notify->subscribe(true, [this](NimBLERemoteCharacteristic*, uint8_t* data, size_t length,
                                       bool) { onNotify(data, length); });
    if (!subscribed) continue;

    tx_ = write;
    return true;
  }
  return false;
}

void BleElmStream::onNotify(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) rx_.push(data[i]);  // drops on overflow
}

bool BleElmStream::connected() const { return client_ != nullptr && client_->isConnected(); }

size_t BleElmStream::write(const uint8_t* data, size_t len) {
  if (!connected() || tx_ == nullptr) return 0;
  // Dongles accept ~20 bytes per write at the default MTU.
  constexpr size_t kChunk = 20;
  size_t sent = 0;
  while (sent < len) {
    const size_t n = len - sent < kChunk ? len - sent : kChunk;
    if (!tx_->writeValue(data + sent, n, false)) break;
    sent += n;
  }
  return sent;
}

int BleElmStream::read() { return rx_.pop(); }

}  // namespace device
