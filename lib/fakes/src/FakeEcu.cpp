#include "FakeEcu.h"

#include <algorithm>
#include <cmath>

#include "obd/PidDecode.h"
#include "obd/Units.h"

namespace gauge {
namespace {

struct Keyframe {
  float seconds;
  float psi;
};

constexpr Keyframe kProfile[] = {
    {0.0f, -11.0f},                    // idle
    {3.0f, -11.0f},  {5.0f, 17.0f},    // floor it
    {8.0f, 17.5f},   {10.0f, -12.0f},  // lift off
    {14.0f, -5.0f},                    // cruise
    {17.0f, 6.0f},                     // light throttle
    {20.0f, -11.0f},                   // back to idle
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

  const auto anyIn = [this](int first, int last) {
    for (int p = first; p <= last && p <= 255; ++p) {
      if (supported(static_cast<uint8_t>(p))) return true;
    }
    return false;
  };

  // A support PID (00, 20, 40, ...): a 32-bit big-endian bitmap of the 32 PIDs after it, whose
  // last bit says a later range has anything. 00 always answers; the others only if they exist.
  if (pid % 32 == 0 && pid <= 0xC0) {
    if (pid != 0 && !anyIn(pid + 1, pid + 32)) return false;
    uint32_t word = 0;
    for (int k = 0; k < 31; ++k) {
      if (supported(static_cast<uint8_t>(pid + 1 + k))) word |= 1u << (31 - k);
    }
    if (anyIn(pid + 33, pid + 64)) word |= 1u;
    out.bytes = {static_cast<uint8_t>(word >> 24), static_cast<uint8_t>(word >> 16),
                 static_cast<uint8_t>(word >> 8), static_cast<uint8_t>(word)};
    out.count = 4;
    return true;
  }

  if (!supported(pid)) return false;
  switch (pid) {
    case obd::kPidMapKpa:
      out.bytes[0] = asByte(mapKpa());
      out.count = 1;
      return true;
    case obd::kPidBaroKpa:
      out.bytes[0] = asByte(baroKpa_);
      out.count = 1;
      return true;
    case obd::kPidCoolantTemp:
      out.bytes[0] = asByte(coolantC_ + 40.0f);
      out.count = 1;
      return true;
    case obd::kPidIntakeAirTemp:
      out.bytes[0] = asByte(intakeAirC_ + 40.0f);
      out.count = 1;
      return true;
    case obd::kPidControlModuleVolts: {
      const long millivolts = std::clamp(std::lround(volts_ * 1000.0f), 0L, 65535L);
      out.bytes[0] = static_cast<uint8_t>(millivolts >> 8);
      out.bytes[1] = static_cast<uint8_t>(millivolts & 0xFF);
      out.count = 2;
      return true;
    }
    default:
      return false;  // listed as supported but this fake has no data for it
  }
}

}  // namespace gauge
