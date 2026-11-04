#include "obd/CanObdClient.h"

#include <algorithm>

namespace gauge {

CanObdClient::CanObdClient(ICanBus& bus, IClock& clock)
    : CanObdClient(bus, clock, Options{}) {}

CanObdClient::CanObdClient(ICanBus& bus, IClock& clock, Options options)
    : bus_(bus), clock_(clock), options_(options) {}

bool CanObdClient::begin() { return bus_.begin(); }

bool CanObdClient::readPid(uint8_t mode, uint8_t pid, ObdData& out) {
  CanFrame request;
  request.id = kRequestId;
  request.len = 8;
  request.data = {0x02, mode, pid, 0x00, 0x00, 0x00, 0x00, 0x00};
  if (!bus_.send(request)) return false;

  const uint8_t expectMode = static_cast<uint8_t>(mode + 0x40);
  const uint32_t start = clock_.nowMs();
  for (;;) {
    const uint32_t elapsed = clock_.nowMs() - start;
    if (elapsed >= options_.replyTimeoutMs) return false;

    CanFrame frame;
    if (!bus_.receive(frame, options_.replyTimeoutMs - elapsed)) return false;

    // Ignore unrelated bus traffic and other PIDs' responses.
    if (frame.id < kResponseIdMin || frame.id > kResponseIdMax) continue;
    if (frame.len < 3) continue;
    const uint8_t pci = frame.data[0];
    if (pci < 3 || pci > 7) continue;  // single frame carrying mode+pid+data
    if (frame.data[1] != expectMode || frame.data[2] != pid) continue;

    const size_t available = std::min<size_t>(pci - 2u, frame.len - 3u);
    out.count = static_cast<uint8_t>(std::min(available, out.bytes.size()));
    for (size_t i = 0; i < out.count; ++i) out.bytes[i] = frame.data[3 + i];
    return true;
  }
}

}  // namespace gauge
