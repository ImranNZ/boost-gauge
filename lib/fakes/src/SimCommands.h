#pragma once

#include <string>
#include <string_view>

#include "FakeEcu.h"
#include "ThrottledSource.h"

namespace gauge {

/// Apply one text command to a FakeEcu. Used by the live preview to let a person
/// drive the mock. Returns false if the command is unknown or malformed.
///
///   drive          follow the scripted drive cycle
///   boost <psi>    hold a fixed gauge pressure (clamped to what a sensor can report)
///   offline        ECU stops answering
///   online         ECU answers again
bool applyEcuCommand(FakeEcu& ecu, std::string_view line);

/// Apply a data-rate command to a throttled source:
///
///   rate <hz>      deliver fresh readings this often (1..100)
bool applyRateCommand(ThrottledSource& source, std::string_view line);

/// Recognise "look <name>" and return the name (no spaces). The caller decides
/// whether that look exists. False if the line is anything else.
bool parseLookCommand(std::string_view line, std::string& name);

/// Recognise "unit <name>" (psi, bar or kpa) and return the name, as parseLookCommand does.
bool parseUnitCommand(std::string_view line, std::string& name);

}  // namespace gauge
