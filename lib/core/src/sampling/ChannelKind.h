#pragma once

#include <cstddef>
#include <string_view>

namespace gauge {

/// The readings the gauge knows how to show.
enum class ChannelKind { Boost, Coolant, IntakeAir, Voltage };

constexpr size_t kChannelKindCount = 4;

/// The short lower-case name used in settings and on the simulator's command line: "boost",
/// "coolant", "intake" or "voltage".
const char* channelName(ChannelKind kind);

/// The reverse of channelName(). False if `name` is not one of them.
bool parseChannelKind(std::string_view name, ChannelKind& kind);

}  // namespace gauge
