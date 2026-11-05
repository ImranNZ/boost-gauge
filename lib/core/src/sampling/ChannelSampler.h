#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "base/IClock.h"
#include "obd/IObdClient.h"
#include "obd/PidSupport.h"
#include "sampling/Channel.h"
#include "sampling/LinkSupervisor.h"
#include "sampling/Reading.h"

namespace gauge {

/// Reads several channels over one link. The link's health is LinkSupervisor's job and what each
/// channel needs is the channel's; this decides when each is read. Each time the link comes up it
/// asks the car which PIDs it implements (PidSupport) and tells the channels, so they can skip
/// what the car lacks. If the car does not answer that, nothing is assumed.
///
/// A channel is read when it is due, and when several are due the one that has waited longest goes
/// first (and if two have waited equally long, the one read least recently). A channel with a
/// period of 0 is always due, so a fast channel such as boost is read as often as the link allows
/// while the slow ones (about once a second) still get their turn.
class ChannelSampler {
 public:
  static constexpr size_t kMaxChannels = 8;

  struct Options {
    LinkSupervisor::Options link;
    /// Called, from whichever thread samples, each time the car's PIDs have been (re)discovered.
    /// For logging what the car reports; it must not touch the OBD client.
    void (*onDiscovered)(const PidSupport&) = nullptr;
  };

  /// One read: which channel (its index, in the order they were added) and what came back. An
  /// invalid reading means the read failed or the car does not offer that channel.
  struct Step {
    size_t channel = 0;
    Reading reading;
  };

  ChannelSampler(IObdClient& client, IClock& clock);
  ChannelSampler(IObdClient& client, IClock& clock, const Options& options);

  /// Add a channel to read; it must outlive the sampler. If the link is already up the channel is
  /// told what the car supports straight away. False if the sampler is full. Not safe to call
  /// from another thread while stepping: the firmware adds its channel before the task starts.
  bool add(IChannel& channel);
  /// Forget every channel (the simulator switches what it shows this way).
  void clear() { count_ = 0; }
  size_t channelCount() const { return count_; }

  /// Do one unit of work: bring the link up if needed, then read the channel that is due. False
  /// if nothing was attempted (the link is down, or nothing is due yet).
  bool step(Step& out);

  /// What the car reported at the last connection (empty until the link first comes up).
  const PidSupport& support() const { return support_; }

 private:
  IObdClient& client_;
  IClock& clock_;
  Options options_;
  PidSupport support_;
  LinkSupervisor link_;

  std::array<IChannel*, kMaxChannels> channels_{};
  std::array<uint32_t, kMaxChannels> dueAtMs_{};
  std::array<uint32_t, kMaxChannels> lastReadOrder_{};  // 0 = never; otherwise a read counter
  uint32_t readCount_ = 0;
  size_t count_ = 0;
  uint32_t seenConnections_ = 0;
};

}  // namespace gauge
