#include "FakeCanBus.h"

namespace gauge {

bool FakeCanBus::send(const CanFrame& frame) {
  if (!started_) return false;
  sent_.push_back(frame);

  // Functional request, single frame: [02, mode, pid, ...]
  if (frame.id != 0x7DF || frame.len < 3 || frame.data[0] != 0x02) return true;

  const uint8_t mode = frame.data[1];
  const uint8_t pid = frame.data[2];
  ObdData data;
  if (!ecu_.answer(mode, pid, data)) return true;  // silence, like a real ECU

  CanFrame reply;
  reply.id = 0x7E8;
  reply.len = 8;
  reply.data[0] = static_cast<uint8_t>(2 + data.count);
  reply.data[1] = static_cast<uint8_t>(mode + 0x40);
  reply.data[2] = pid;
  for (uint8_t i = 0; i < data.count; ++i) reply.data[3 + i] = data.bytes[i];
  inbox_.push_back(reply);
  return true;
}

bool FakeCanBus::receive(CanFrame& out, uint32_t /*timeoutMs*/) {
  if (inbox_.empty()) return false;  // nothing will arrive; behave as a timeout
  out = inbox_.front();
  inbox_.pop_front();
  return true;
}

}  // namespace gauge
