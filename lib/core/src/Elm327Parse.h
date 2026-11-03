#pragma once

#include <cstdint>
#include <string_view>

#include "IObdClient.h"

namespace gauge {

/// Extract the data bytes for (mode, pid) from raw ELM327 reply text.
/// Tolerates echo, "SEARCHING...", spaces, and multiple ECU lines; returns
/// false for "NO DATA", errors, or garbage.
bool parseObdReply(std::string_view text, uint8_t mode, uint8_t pid, ObdData& out);

}  // namespace gauge
