#pragma once

#include <string_view>

#include "FakeEcu.h"

namespace gauge {

/// Apply one text command to a FakeEcu. Used by the live preview to let a person
/// drive the mock. Returns false if the command is unknown or malformed.
///
///   drive          follow the scripted drive cycle
///   boost <psi>    hold a fixed gauge pressure (clamped to what a sensor can report)
///   offline        ECU stops answering
///   online         ECU answers again
bool applyEcuCommand(FakeEcu& ecu, std::string_view line);

}  // namespace gauge
