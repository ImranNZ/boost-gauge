#pragma once

#include <cstdint>

#include "IObdClient.h"

namespace gauge {

/// IObdClient whose answers are set directly by the test.
class StubObdClient : public IObdClient {
 public:
  struct Reply {
    bool ok = false;
    uint8_t count = 0;
    uint8_t value = 0;
  };

  bool beginResult = true;
  int beginCalls = 0;
  Reply map;
  Reply baro;

  bool begin() override {
    ++beginCalls;
    return beginResult;
  }

  bool readPid(uint8_t /*mode*/, uint8_t pid, ObdData& out) override {
    const Reply& r = (pid == obd::kPidMapKpa) ? map : baro;
    if (!r.ok) return false;
    out.count = r.count;
    out.bytes[0] = r.value;
    return true;
  }
};

}  // namespace gauge
