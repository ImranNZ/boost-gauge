#pragma once

namespace gauge {

constexpr float kKpaPerPsi = 6.894757f;
constexpr float kStdAtmosphereKpa = 101.325f;

constexpr float kpaToPsi(float kpa) { return kpa / kKpaPerPsi; }
constexpr float psiToKpa(float psi) { return psi * kKpaPerPsi; }

/// Gauge boost in psi from absolute manifold pressure and ambient pressure
/// (both kPa). Negative result means manifold vacuum.
constexpr float boostPsi(float mapKpa, float baroKpa) { return kpaToPsi(mapKpa - baroKpa); }

}  // namespace gauge
