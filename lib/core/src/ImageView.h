#pragma once

#include <cstdint>

#include "Color.h"

namespace gauge {

/// A read-only picture held elsewhere (typically a generated array in flash).
/// Pixels are row-major RGB565; `alpha`, when present, is one byte per pixel
/// (255 = opaque) so artwork such as a badge can have soft edges.
struct ImageView {
  int width = 0;
  int height = 0;
  const uint16_t* rgb565 = nullptr;
  const uint8_t* alpha = nullptr;  // optional
};

/// A read-only 8-bit coverage bitmap (255 = fully covered), one byte per pixel,
/// row-major. Drawn tinted with a colour: this is how anti-aliased text is drawn.
struct MaskView {
  int width = 0;
  int height = 0;
  const uint8_t* alpha = nullptr;
};

/// Expand RGB565 to 8 bits per channel, replicating high bits so white stays white.
inline Color rgb565ToColor(uint16_t value) {
  const uint8_t r5 = (value >> 11) & 0x1F;
  const uint8_t g6 = (value >> 5) & 0x3F;
  const uint8_t b5 = value & 0x1F;
  return Color{static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
               static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
               static_cast<uint8_t>((b5 << 3) | (b5 >> 2))};
}

}  // namespace gauge
