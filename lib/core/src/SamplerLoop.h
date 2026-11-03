#pragma once

#include "BoostSampler.h"
#include "ReadingMailbox.h"

namespace gauge {

/// One iteration of the producer side: ask the car, publish the answer. A
/// platform task calls step() in a loop on its own thread, so a slow or dead
/// link only slows this loop, never the screen.
class SamplerLoop {
 public:
  SamplerLoop(BoostSampler& sampler, ReadingMailbox& mailbox)
      : sampler_(sampler), mailbox_(mailbox) {}

  void step() { mailbox_.publish(sampler_.sample()); }

 private:
  BoostSampler& sampler_;
  ReadingMailbox& mailbox_;
};

}  // namespace gauge
