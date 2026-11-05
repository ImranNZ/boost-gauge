#pragma once

namespace gauge {

/// One measurement of a channel: its value in the channel's canonical unit (psi for pressure,
/// degrees Celsius for a temperature, volts for a voltage), and whether there is one at all.
struct Reading {
  float value = 0.0f;
  bool valid = false;
};

}  // namespace gauge
