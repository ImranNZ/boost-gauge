#include "ArcCoverage.h"

#include <algorithm>
#include <cmath>

namespace gauge {
namespace {
constexpr float kRadToDeg = 180.0f / 3.14159265f;
}

float wrapDegrees(float degrees) {
  float wrapped = std::fmod(degrees, 360.0f);
  if (wrapped < 0.0f) wrapped += 360.0f;
  return wrapped;
}

float arcCoverage(float dx, float dy, float innerRadius, float outerRadius, float startDeg,
                  float sweepDeg) {
  // Cheap rejection before any sqrt/atan2: most pixels are nowhere near the ring.
  const float d2 = dx * dx + dy * dy;
  const float lo = std::max(innerRadius - 0.5f, 0.0f);
  const float hi = outerRadius + 0.5f;
  if (d2 < lo * lo || d2 > hi * hi) return 0.0f;

  const float r = std::sqrt(d2);
  // A zero inner radius is a solid disc: there is no inner edge to soften.
  const float inside = innerRadius <= 0.0f ? 1.0f : std::clamp(r - innerRadius + 0.5f, 0.0f, 1.0f);
  const float outside = std::clamp(outerRadius - r + 0.5f, 0.0f, 1.0f);
  const float radial = inside * outside;
  if (radial <= 0.0f) return 0.0f;

  const float angle = std::atan2(dy, dx) * kRadToDeg;
  return wrapDegrees(angle - startDeg) <= sweepDeg ? radial : 0.0f;
}

}  // namespace gauge
