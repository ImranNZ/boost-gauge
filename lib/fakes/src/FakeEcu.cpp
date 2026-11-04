#include "FakeEcu.h"

#include <algorithm>
#include <cmath>

#include "obd/Units.h"

namespace gauge {
namespace {

struct Keyframe {
  float seconds;
  float psi;
};

constexpr Keyframe kProfile[] = {
    {0.0f, -11.0f},   // idle
    {3.0f, -11.0f},
    {5.0f, 17.0f},    // floor it
    {8.0f, 17.5f},
    {10.0f, -12.0f},  // lift off
    {14.0f, -5.0f},   // cruise
    {17.0f, 6.0f},    // light throttle
    {20.0f, -11.0f},  // back to idle
};

float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

}  // namespace

float driveProfileBoostPsi(uint32_t timeMs) {
  const float t = static_cast<float>(timeMs % 20000u) / 1000.0f;
  constexpr size_t kCount = sizeof(kProfile) / sizeof(kProfile[0]);
  for (size_t i = 1; i < kCount; ++i) {
    if (t <= kProfile[i].seconds) {
      const Keyframe& a = kProfile[i - 1];
      const Keyframe& b = kProfile[i];
      const float f = (t - a.seconds) / (b.seconds - a.seconds);
      return a.psi + (b.psi - a.psi) * smoothstep(f);
    }
  }
  return kProfile[kCount - 1].psi;
}

float FakeEcu::mapKpa() const {
  if (held_) return heldMapKpa_;
  return baroKpa_ + psiToKpa(driveProfileBoostPsi(clock_.nowMs()));
}

bool FakeEcu::answer(uint8_t mode, uint8_t pid, ObdData& out) const {
  if (!online_ || mode != obd::kModeCurrentData) return false;

  const auto asByte = [](float v) {
    return static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
  };

  switch (pid) {
    case obd::kPidSupported01To20:
      out.bytes = {0x00, 0x20, 0x00, 0x01};  // advertises 0x0B (and 0x20)
      out.count = 4;
      return true;
    case obd::kPidMapKpa:
      out.bytes[0] = asByte(mapKpa());
      out.count = 1;
      return true;
    case obd::kPidBaroKpa:
      out.bytes[0] = asByte(baroKpa_);
      out.count = 1;
      return true;
    default:
      return false;
  }
}

}  // namespace gauge
