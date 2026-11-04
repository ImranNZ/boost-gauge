#pragma once

#include "obd/IObdClient.h"

namespace gauge {
namespace obd {

constexpr uint8_t kPidCoolantTemp = 0x05;         // engine coolant temperature, A - 40 degrees C
constexpr uint8_t kPidIntakeAirTemp = 0x0F;       // intake air temperature, A - 40 degrees C
constexpr uint8_t kPidControlModuleVolts = 0x42;  // control module voltage, (256 A + B) / 1000 V

/// Engine coolant temperature in degrees Celsius (-40 to 215).
constexpr float coolantTempC(const ObdData& data) {
  return static_cast<float>(data.bytes[0]) - 40.0f;
}

/// Intake air temperature in degrees Celsius (-40 to 215).
constexpr float intakeAirTempC(const ObdData& data) {
  return static_cast<float>(data.bytes[0]) - 40.0f;
}

/// Control module (battery) voltage in volts (0 to 65.535).
constexpr float controlModuleVolts(const ObdData& data) {
  return (256.0f * static_cast<float>(data.bytes[0]) + static_cast<float>(data.bytes[1])) / 1000.0f;
}

}  // namespace obd
}  // namespace gauge
