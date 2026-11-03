#pragma once

#include "SamplerLoop.h"

namespace device {

/// Runs SamplerLoop forever on its own FreeRTOS task (core 0, alongside the
/// WiFi/BLE stacks), so a slow or dead link can never stall the render loop
/// that runs on core 1.
class SamplerTask {
 public:
  explicit SamplerTask(gauge::SamplerLoop& loop) : loop_(loop) {}

  /// Create the task. Call once from setup().
  void start();

 private:
  static void entry(void* self);

  gauge::SamplerLoop& loop_;
};

}  // namespace device
