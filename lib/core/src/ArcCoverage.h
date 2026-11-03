#pragma once

namespace gauge {

/// Wrap an angle in degrees into [0, 360).
float wrapDegrees(float degrees);

/// Fraction (0..1) of a pixel covered by a ring sector, for anti-aliased arcs.
///
/// (dx, dy) is the pixel centre relative to the arc centre. The ring runs from
/// innerRadius to outerRadius and sweeps `sweepDeg` clockwise from `startDeg`
/// (0 = 3 o'clock, screen space; both already wrapped, sweepDeg in [0, 360)).
/// The inner/outer edges ramp over one pixel; the two radial ends are hard.
float arcCoverage(float dx, float dy, float innerRadius, float outerRadius, float startDeg,
                  float sweepDeg);

}  // namespace gauge
