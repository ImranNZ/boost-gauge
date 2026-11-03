#pragma once

#include <cstdint>

#include "IClock.h"
#include "IObdClient.h"

namespace gauge {

struct BoostReading {
  float psi = 0.0f;  // gauge pressure; negative = vacuum
  bool valid = false;
};

/// Turns raw PIDs into boost: MAP minus barometric pressure. Owns link
/// recovery so callers just keep calling sample().
class BoostSampler {
 public:
  struct Options {
    uint32_t baroRefreshMs = 60000;  // re-read ambient pressure this often
    uint32_t baroRetryMs = 10000;    // ...or this often while it is unavailable
    uint8_t failuresBeforeReconnect = 5;
    uint32_t reconnectIntervalMs = 2000;
  };

  BoostSampler(IObdClient& client, IClock& clock);
  BoostSampler(IObdClient& client, IClock& clock, Options options);

  BoostReading sample();

 private:
  void refreshBaroIfDue();
  void tryConnect();

  IObdClient& client_;
  IClock& clock_;
  Options options_;

  bool linked_ = false;
  bool everTried_ = false;
  uint32_t lastConnectAttemptMs_ = 0;
  uint8_t consecutiveFailures_ = 0;

  float baroKpa_;
  bool haveBaro_ = false;
  bool baroTried_ = false;
  uint32_t baroAtMs_ = 0;
};

}  // namespace gauge
