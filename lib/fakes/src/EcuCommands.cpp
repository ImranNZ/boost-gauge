#include "EcuCommands.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "Units.h"

namespace gauge {
namespace {

constexpr float kMaxSensorKpa = 255.0f;  // PID 0B is one byte

std::string_view trim(std::string_view s) {
  const char* whitespace = " \t\r\n";
  const size_t first = s.find_first_not_of(whitespace);
  if (first == std::string_view::npos) return {};
  const size_t last = s.find_last_not_of(whitespace);
  return s.substr(first, last - first + 1);
}

// strtof rather than std::from_chars: the ESP32 toolchain's libstdc++ lacks the
// floating-point overload of from_chars.
bool parseFloat(std::string_view text, float& out) {
  char buffer[32];
  if (text.empty() || text.size() >= sizeof(buffer)) return false;
  std::memcpy(buffer, text.data(), text.size());
  buffer[text.size()] = '\0';

  char* end = nullptr;
  const float value = std::strtof(buffer, &end);
  if (end != buffer + text.size()) return false;  // not fully numeric
  out = value;
  return true;
}

}  // namespace

bool applyEcuCommand(FakeEcu& ecu, std::string_view line) {
  line = trim(line);

  if (line == "drive") {
    ecu.followDriveProfile();
    return true;
  }
  if (line == "offline" || line == "online") {
    ecu.setOnline(line == "online");
    return true;
  }

  constexpr std::string_view kBoost = "boost ";
  if (line.substr(0, kBoost.size()) == kBoost) {
    float psi = 0.0f;
    if (!parseFloat(trim(line.substr(kBoost.size())), psi)) return false;
    const float kpa = ecu.baroKpa() + psiToKpa(psi);
    ecu.holdMapKpa(std::clamp(kpa, 0.0f, kMaxSensorKpa));
    return true;
  }
  return false;
}

}  // namespace gauge
