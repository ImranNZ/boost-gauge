#pragma once

#include "render/Picture.h"

namespace gauge {

/// An optional picture laid over a dial, e.g. a model badge. Top-left in design pixels.
struct Badge {
  Picture picture;
  int x = 0;
  int y = 0;
};

}  // namespace gauge
