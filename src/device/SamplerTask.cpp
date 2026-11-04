#include "device/SamplerTask.h"

#include <Arduino.h>

namespace device {
namespace {
constexpr uint32_t kStackBytes = 8192;  // BLE/WiFi calls run on this stack
constexpr UBaseType_t kPriority = 1;    // same as loop(); core 0 is otherwise radio work
constexpr BaseType_t kCore = 0;         // loop() runs on core 1
}  // namespace

void SamplerTask::start() {
  xTaskCreatePinnedToCore(&SamplerTask::entry, "sampler", kStackBytes, this, kPriority, nullptr,
                          kCore);
}

void SamplerTask::entry(void* self) {
  auto* task = static_cast<SamplerTask*>(self);
  for (;;) {
    task->loop_.step();
    vTaskDelay(1);  // always yield: a link that fails instantly must not starve the idle task
  }
}

}  // namespace device
