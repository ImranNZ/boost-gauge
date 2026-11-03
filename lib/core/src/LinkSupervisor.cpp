#include "LinkSupervisor.h"

namespace gauge {

LinkSupervisor::LinkSupervisor(IObdClient& client, IClock& clock)
    : LinkSupervisor(client, clock, Options{}) {}

LinkSupervisor::LinkSupervisor(IObdClient& client, IClock& clock, Options options)
    : client_(client), clock_(clock), options_(options) {}

bool LinkSupervisor::ensureLinked() {
  if (linked_) return true;

  const uint32_t now = clock_.nowMs();
  if (everTried_ && now - lastAttemptMs_ < options_.reconnectIntervalMs) return false;
  everTried_ = true;
  lastAttemptMs_ = now;

  consecutiveFailures_ = 0;
  linked_ = client_.begin();
  if (linked_) ++connections_;
  return linked_;
}

void LinkSupervisor::noteFailure() {
  if (++consecutiveFailures_ >= options_.failuresBeforeReconnect) linked_ = false;
}

}  // namespace gauge
