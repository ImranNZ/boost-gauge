#pragma once

#include <cstdint>

#include "base/IClock.h"
#include "obd/IObdClient.h"

namespace gauge {

/// Keeps a link to the car alive: connects when it is down (no more often than
/// the retry interval) and declares it down again after a run of failures.
/// It makes no requests of its own; callers report how their requests went.
class LinkSupervisor {
 public:
  struct Options {
    uint8_t failuresBeforeReconnect = 5;
    uint32_t reconnectIntervalMs = 2000;
  };

  LinkSupervisor(IObdClient& client, IClock& clock);
  LinkSupervisor(IObdClient& client, IClock& clock, Options options);

  /// True if the link is up, connecting first if it is down and a retry is due.
  bool ensureLinked();

  void noteSuccess() { consecutiveFailures_ = 0; }
  void noteFailure();

  /// How many times the link has been (re)established. A change means "start
  /// over": whatever was learned about the car may no longer hold.
  uint32_t connectionCount() const { return connections_; }

 private:
  IObdClient& client_;
  IClock& clock_;
  Options options_;

  bool linked_ = false;
  bool everTried_ = false;
  uint32_t lastAttemptMs_ = 0;
  uint8_t consecutiveFailures_ = 0;
  uint32_t connections_ = 0;
};

}  // namespace gauge
