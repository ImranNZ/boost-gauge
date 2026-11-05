#pragma once

#include "app/ISettingsStore.h"

namespace gauge {

/// An ISettingsStore that keeps the setup in memory, so it lasts as long as the process. For tests
/// and the simulator; a board keeps it in flash.
class MemorySettingsStore : public ISettingsStore {
 public:
  bool load(Setup& out) override {
    if (!saved_) return false;
    out = setup_;
    return true;
  }

  void save(const Setup& setup) override {
    setup_ = setup;
    saved_ = true;
    ++saves_;
  }

  /// How many times save() has been called.
  int saves() const { return saves_; }

 private:
  Setup setup_;
  bool saved_ = false;
  int saves_ = 0;
};

}  // namespace gauge
