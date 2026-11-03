#pragma once

#include <cstdint>

namespace gauge {

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

/// `over` laid on `under` at `alpha` (0 = all under, 255 = all over), rounded.
/// The one blend used by every canvas, so they agree to the last bit.
inline Color blend(Color under, Color over, int alpha) {
  const auto mix = [alpha](int u, int o) {
    return static_cast<uint8_t>((u * (255 - alpha) + o * alpha + 127) / 255);
  };
  return Color{mix(under.r, over.r), mix(under.g, over.g), mix(under.b, over.b)};
}

}  // namespace gauge
