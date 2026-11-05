#pragma once

#include <string>
#include <string_view>

#include "FakeEcu.h"
#include "ThrottledSource.h"
#include "app/IInput.h"

namespace gauge {

/// Apply one text command to a FakeEcu. Used by the live preview to let a person
/// drive the mock. Returns false if the command is unknown or malformed.
///
///   drive          every reading follows its scripted drive cycle
///   boost <psi>    hold a fixed gauge pressure (clamped to what a sensor can report)
///   coolant <C>    hold the coolant temperature, degrees Celsius
///   intake <C>     hold the intake air temperature, degrees Celsius
///   volts <V>      hold the battery voltage
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

/// Recognise "unit <name>" (psi, bar, kpa, c, f or v) and return the name, as parseLookCommand
/// does.
bool parseUnitCommand(std::string_view line, std::string& name);

/// Recognise "channel <name>" (boost, coolant, intake or voltage), likewise.
bool parseChannelCommand(std::string_view line, std::string& name);

/// Recognise "button <next|select>": a press of the gauge's button, as an input event. False if
/// the line is anything else.
bool parseButtonCommand(std::string_view line, InputEvent& event);

}  // namespace gauge
