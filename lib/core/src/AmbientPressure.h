#pragma once

#include <cstdint>

#include "IClock.h"
#include "IObdClient.h"
#include "Units.h"

namespace gauge {

/// Barometric pressure, read from the car and cached: it changes slowly, so it
/// is re-read now and then rather than on every sample, and an ECU that lacks
/// the PID is not asked on every frame. Until something is read it assumes
/// standard atmosphere.
class AmbientPressure {
 public:
  struct Options {
    uint32_t refreshMs = 60000;  // re-read this often once known
    uint32_t retryMs = 10000;    // ...or this often while it is unavailable
  };

  AmbientPressure(IObdClient& client, IClock& clock);
  AmbientPressure(IObdClient& client, IClock& clock, Options options);

  /// Ambient pressure in kPa, asking the car first if a read is due.
  float kpa();

  /// Forget what was learned (the car may have been somewhere else): read again
  /// at the next opportunity.
  void invalidate();

 private:
  IObdClient& client_;
  IClock& clock_;
  Options options_;

  float kpa_ = kStdAtmosphereKpa;
  bool known_ = false;
  bool tried_ = false;
  uint32_t triedAtMs_ = 0;
};

}  // namespace gauge
