#pragma once

#include "app/IInput.h"
#include "app/ISettingsStore.h"
#include "app/SettingsMenu.h"

namespace gauge {

/// Whatever shows the gauge in a new setup: switches the look, the channel being sampled and the
/// profile. Each composition root implements it for its own wiring.
class ISetupTarget {
 public:
  virtual ~ISetupTarget() = default;
  virtual void apply(const Setup& setup) = 0;
};

/// Joins the pieces: reads the input, feeds the menu, and when the owner finishes on a different
/// setup, saves it and has the target show it. Call tick() once per frame.
class SettingsController {
 public:
  SettingsController(IInput& input, SettingsMenu& menu, ISettingsStore& store, ISetupTarget& target)
      : input_(input), menu_(menu), store_(store), target_(target) {}

  void tick();

 private:
  IInput& input_;
  SettingsMenu& menu_;
  ISettingsStore& store_;
  ISetupTarget& target_;
};

}  // namespace gauge
