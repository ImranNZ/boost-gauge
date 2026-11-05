#include "sampling/ChannelKind.h"

namespace gauge {
namespace {

constexpr ChannelKind kAll[] = {ChannelKind::Boost, ChannelKind::Coolant, ChannelKind::IntakeAir,
                                ChannelKind::Voltage};

}  // namespace

const char* channelName(ChannelKind kind) {
  if (kind == ChannelKind::Coolant) return "coolant";
  if (kind == ChannelKind::IntakeAir) return "intake";
  if (kind == ChannelKind::Voltage) return "voltage";
  return "boost";
}

bool parseChannelKind(std::string_view name, ChannelKind& kind) {
  for (ChannelKind candidate : kAll) {
    if (name == channelName(candidate)) {
      kind = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace gauge
