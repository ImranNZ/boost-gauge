#pragma once

#include <cstdint>

#include "base/IClock.h"
#include "obd/IObdClient.h"
#include "obd/PidSupport.h"
#include "sampling/AmbientPressure.h"
#include "sampling/LinkSupervisor.h"

namespace gauge {

struct BoostReading {
  float psi = 0.0f;  // gauge pressure; negative = vacuum
  bool valid = false;
};

/// Turns raw PIDs into boost: manifold pressure minus ambient. A coordinator:
/// the link's health is LinkSupervisor's job and ambient pressure is
/// AmbientPressure's, so callers just keep calling sample().
///
/// Each time the link comes up it asks the car which PIDs it implements (PidSupport) and adapts:
/// a car that lacks barometric pressure is not asked for it, and one that lacks manifold
/// pressure cannot show boost, which is reported as no reading without hammering the bus. If
/// the car does not answer the discovery, nothing is assumed and it behaves as if it were
/// not there.
class BoostSampler {
 public:
  struct Options {
    LinkSupervisor::Options link;
    AmbientPressure::Options ambient;
    /// Called, from whichever thread samples, each time the car's PIDs have been (re)discovered.
    /// For logging what the car reports; it must not touch the OBD client.
    void (*onDiscovered)(const PidSupport&) = nullptr;
  };

  BoostSampler(IObdClient& client, IClock& clock);
  BoostSampler(IObdClient& client, IClock& clock, Options options);

  BoostReading sample();

  /// What the car reported at the last connection (empty until the link first comes up).
  const PidSupport& support() const { return support_; }

 private:
  IObdClient& client_;
  Options options_;
  PidSupport support_;
  LinkSupervisor link_;
  AmbientPressure ambient_;
  uint32_t seenConnections_ = 0;
};

}  // namespace gauge
