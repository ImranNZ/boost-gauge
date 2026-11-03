#include "Elm327Parse.h"

#include <algorithm>

namespace gauge {
namespace {

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

bool parseLine(std::string_view line, uint8_t expectMode, uint8_t pid, ObdData& out) {
  std::array<uint8_t, 8> bytes{};
  size_t nibbles = 0;
  for (char c : line) {
    if (c == ' ') continue;
    const int v = hexValue(c);
    if (v < 0) return false;  // not a pure hex line (SEARCHING..., NO DATA, ?)
    if (nibbles >= bytes.size() * 2) break;
    if (nibbles % 2 == 0) {
      bytes[nibbles / 2] = static_cast<uint8_t>(v << 4);
    } else {
      bytes[nibbles / 2] |= static_cast<uint8_t>(v);
    }
    ++nibbles;
  }
  if (nibbles < 6 || nibbles % 2 != 0) return false;  // mode + pid + >=1 data byte
  if (bytes[0] != expectMode || bytes[1] != pid) return false;

  const size_t total = nibbles / 2;
  out.count = static_cast<uint8_t>(std::min<size_t>(total - 2, out.bytes.size()));
  for (size_t i = 0; i < out.count; ++i) out.bytes[i] = bytes[2 + i];
  return true;
}

}  // namespace

bool parseObdReply(std::string_view text, uint8_t mode, uint8_t pid, ObdData& out) {
  const uint8_t expectMode = static_cast<uint8_t>(mode + 0x40);
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find_first_of("\r\n", pos);
    if (end == std::string_view::npos) end = text.size();
    if (parseLine(text.substr(pos, end - pos), expectMode, pid, out)) return true;
    pos = end + 1;
  }
  return false;
}

}  // namespace gauge
