#pragma once

#include "sampling/ChannelSampler.h"
#include "sampling/ReadingMailbox.h"

namespace gauge {

/// One iteration of the producer side: ask the car, publish the answer. A
/// platform task calls step() in a loop on its own thread, so a slow or dead
/// link only slows this loop, never the screen.
class SamplerLoop {
 public:
  SamplerLoop(ChannelSampler& sampler, ChannelMailboxes& mailboxes)
      : sampler_(sampler), mailboxes_(mailboxes) {}

  /// True if the car was asked something (and the answer published).
  bool step() {
    ChannelSampler::Step step;
    if (!sampler_.step(step)) return false;
    mailboxes_.publish(step.channel, step.reading);
    return true;
  }

 private:
  ChannelSampler& sampler_;
  ChannelMailboxes& mailboxes_;
};

}  // namespace gauge
