#pragma once

#include <cstdint>

#include "base/IClock.h"
#include "obd/IObdClient.h"
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
class BoostSampler {
 public:
  struct Options {
    LinkSupervisor::Options link;
    AmbientPressure::Options ambient;
  };

  BoostSampler(IObdClient& client, IClock& clock);
  BoostSampler(IObdClient& client, IClock& clock, Options options);

  BoostReading sample();

 private:
  IObdClient& client_;
  LinkSupervisor link_;
  AmbientPressure ambient_;
  uint32_t seenConnections_ = 0;
};

}  // namespace gauge
