#pragma once

#include "app/Setup.h"

namespace gauge {

/// Somewhere the chosen setup survives a power cycle (flash, EEPROM). encodeSetup() and
/// decodeSetup() turn a Setup into the one word a store needs to keep.
class ISettingsStore {
 public:
  virtual ~ISettingsStore() = default;

  /// Read the saved setup into `out`. False, leaving `out` alone, if there is none.
  virtual bool load(Setup& out) = 0;
  virtual void save(const Setup& setup) = 0;
};

}  // namespace gauge
