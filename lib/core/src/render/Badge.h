#pragma once

#include "render/ImageView.h"

namespace gauge {

/// An optional picture laid over a dial, e.g. a model badge. Top-left in pixels.
struct Badge {
  ImageView image;
  int x = 0;
  int y = 0;
};

}  // namespace gauge
