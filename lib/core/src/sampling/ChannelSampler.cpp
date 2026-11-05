#include "sampling/ChannelSampler.h"

namespace gauge {

ChannelSampler::ChannelSampler(IObdClient& client, IClock& clock)
    : ChannelSampler(client, clock, Options{}) {}

ChannelSampler::ChannelSampler(IObdClient& client, IClock& clock, const Options& options)
    : client_(client), clock_(clock), options_(options), link_(client, clock, options.link) {}

bool ChannelSampler::add(IChannel& channel) {
  if (count_ == kMaxChannels) return false;
  channels_[count_++] = &channel;
  return true;
}

bool ChannelSampler::step(Step& out) {
  if (!link_.ensureLinked()) return false;

  if (link_.connectionCount() != seenConnections_) {
    seenConnections_ = link_.connectionCount();
    support_.discover(client_);
    for (size_t i = 0; i < count_; ++i) {
      channels_[i]->onLinked(support_);
      dueAtMs_[i] = 0;  // read everything once, straight away
    }
    if (options_.onDiscovered != nullptr) options_.onDiscovered(support_);
  }

  // The due channel that has waited longest, and of those the one read least recently. Times are
  // compared as differences so the 32-bit millisecond clock may wrap.
  const uint32_t now = clock_.nowMs();
  bool found = false;
  size_t pick = 0;
  for (size_t i = 0; i < count_; ++i) {
    if (static_cast<int32_t>(now - dueAtMs_[i]) < 0) continue;  // not due yet
    const int32_t waited = found ? static_cast<int32_t>(dueAtMs_[i] - dueAtMs_[pick]) : 0;
    if (!found || waited < 0 || (waited == 0 && lastReadOrder_[i] < lastReadOrder_[pick])) {
      found = true;
      pick = i;
    }
  }
  if (!found) return false;

  dueAtMs_[pick] = now + channels_[pick]->periodMs();
  lastReadOrder_[pick] = ++readCount_;
  out.channel = pick;
  out.reading = {};

  float value = 0.0f;
  const ReadStatus status = channels_[pick]->read(client_, value);
  if (status == ReadStatus::Ok) {
    link_.noteSuccess();
    out.reading = {value, true};
  } else if (status == ReadStatus::Failed) {
    link_.noteFailure();
  }
  // Unavailable: nothing is wrong with the link, the car just does not offer this channel.
  return true;
}

}  // namespace gauge
