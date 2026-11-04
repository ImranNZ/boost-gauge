#include "SimCommands.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "obd/Units.h"

namespace gauge {
namespace {

constexpr float kMaxSensorKpa = 255.0f;  // PID 0B is one byte
constexpr float kMinRateHz = 1.0f;
constexpr float kMaxRateHz = 100.0f;

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

/// If `line` is "<verb> <number>", return true and the number.
bool parseVerbNumber(std::string_view line, std::string_view verb, float& number) {
  if (line.substr(0, verb.size()) != verb) return false;
  line.remove_prefix(verb.size());
  if (line.empty() || line.front() != ' ') return false;
  return parseFloat(trim(line), number);
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

  float psi = 0.0f;
  if (parseVerbNumber(line, "boost", psi)) {
    const float kpa = ecu.baroKpa() + psiToKpa(psi);
    ecu.holdMapKpa(std::clamp(kpa, 0.0f, kMaxSensorKpa));
    return true;
  }
  return false;
}

namespace {

/// "<verb> <word>" with exactly one word after the verb.
bool parseVerbWord(std::string_view line, std::string_view verb, std::string& word) {
  line = trim(line);
  if (line.substr(0, verb.size()) != verb) return false;
  line.remove_prefix(verb.size());
  if (line.empty() || line.front() != ' ') return false;

  const std::string_view argument = trim(line);
  if (argument.empty() || argument.find(' ') != std::string_view::npos) return false;
  word.assign(argument);
  return true;
}

}  // namespace

bool parseLookCommand(std::string_view line, std::string& name) {
  return parseVerbWord(line, "look", name);
}

bool parseUnitCommand(std::string_view line, std::string& name) {
  return parseVerbWord(line, "unit", name);
}

bool applyRateCommand(ThrottledSource& source, std::string_view line) {
  float hz = 0.0f;
  if (!parseVerbNumber(trim(line), "rate", hz)) return false;
  if (hz < kMinRateHz || hz > kMaxRateHz) return false;
  source.setIntervalMs(static_cast<uint32_t>(1000.0f / hz + 0.5f));
  return true;
}

}  // namespace gauge
