#pragma once

#include "base/IClock.h"
#include "sampling/AmbientPressure.h"
#include "sampling/Channel.h"

namespace gauge {

/// Boost: manifold absolute pressure (PID 0B) minus ambient pressure, in psi (negative is
/// vacuum). The one channel built from two PIDs: ambient pressure is AmbientPressure's job (a
/// slow, cached, throttled read), so this just combines the two. It is read as often as the link
/// allows, which is what keeps the needle smooth.
class BoostChannel : public IChannel {
 public:
  BoostChannel(IObdClient& client, IClock& clock);
  BoostChannel(IObdClient& client, IClock& clock, const AmbientPressure::Options& ambient);

  uint32_t periodMs() const override { return 0; }
  void onLinked(const PidSupport& support) override;
  ReadStatus read(IObdClient& client, float& value) override;

 private:
  AmbientPressure ambient_;
  bool manifoldAvailable_ = true;
};

}  // namespace gauge
