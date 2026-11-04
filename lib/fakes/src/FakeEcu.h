#pragma once

#include <array>
#include <cstdint>

#include "base/IClock.h"
#include "obd/IObdClient.h"

namespace gauge {

/// Scripted drive cycle (20 s loop): idle vacuum, a full-boost pull, lift-off,
/// then light cruise. Returns gauge psi for a time since start.
float driveProfileBoostPsi(uint32_t timeMs);

/// Pretend engine control unit: answers mode 01 PIDs from a drive profile or a
/// pinned value. Shared by every fake transport so they all "drive" alike.
class FakeEcu {
 public:
  explicit FakeEcu(const IClock& clock) : clock_(clock) {}

  void holdMapKpa(float kpa) {
    held_ = true;
    heldMapKpa_ = kpa;
  }
  void followDriveProfile() { held_ = false; }
  void setBaroKpa(float kpa) { baroKpa_ = kpa; }
  float baroKpa() const { return baroKpa_; }
  /// Which mode 01 PIDs this ECU implements (default: manifold and barometric pressure). The
  /// support bitmaps (PID 00, 20, ...) are generated from this, so tests can model a car that
  /// lacks one.
  void setSupported(uint8_t pid, bool supported) {
    if (supported) {
      supported_[pid / 8] |= static_cast<uint8_t>(1u << (pid % 8));
    } else {
      supported_[pid / 8] &= static_cast<uint8_t>(~(1u << (pid % 8)));
    }
  }
  bool supported(uint8_t pid) const { return (supported_[pid / 8] >> (pid % 8)) & 1; }
  void setOnline(bool online) { online_ = online; }
  bool online() const { return online_; }

  float mapKpa() const;

  /// Fill `out` and return true if this ECU supports (mode, pid).
  bool answer(uint8_t mode, uint8_t pid, ObdData& out) const;

 private:
  const IClock& clock_;
  bool held_ = false;
  float heldMapKpa_ = 0.0f;
  float baroKpa_ = 101.0f;
  bool online_ = true;
  // Bit n set: PID n answers. Starts with 0B and 33.
  std::array<uint8_t, 32> supported_ = {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08};
};

}  // namespace gauge
