#include "device/TwaiBus.h"

#include <Arduino.h>
#include <driver/twai.h>

#include <algorithm>

#include "config.h"

namespace device {

bool TwaiBus::begin() {
  if (started_) return true;

  const twai_general_config_t general =
      TWAI_GENERAL_CONFIG_DEFAULT(static_cast<gpio_num_t>(config::kCanTx),
                                  static_cast<gpio_num_t>(config::kCanRx), TWAI_MODE_NORMAL);
  const twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&general, &timing, &filter) != ESP_OK) return false;
  if (twai_start() != ESP_OK) {
    twai_driver_uninstall();
    return false;
  }
  started_ = true;
  return true;
}

bool TwaiBus::send(const gauge::CanFrame& frame) {
  if (!started_) return false;
  twai_message_t message = {};
  message.identifier = frame.id;
  message.data_length_code = std::min<uint8_t>(frame.len, 8);
  std::copy_n(frame.data.begin(), message.data_length_code, message.data);
  return twai_transmit(&message, pdMS_TO_TICKS(20)) == ESP_OK;
}

bool TwaiBus::receive(gauge::CanFrame& out, uint32_t timeoutMs) {
  if (!started_) return false;
  twai_message_t message;
  if (twai_receive(&message, pdMS_TO_TICKS(timeoutMs)) != ESP_OK) return false;

  out.id = message.identifier;
  out.len = std::min<uint8_t>(message.data_length_code, 8);
  std::copy_n(message.data, out.len, out.data.begin());
  return true;
}

}  // namespace device
