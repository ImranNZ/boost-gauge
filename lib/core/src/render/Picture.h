#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "render/IDrawSurface.h"

namespace gauge {

/// A picture rendered ahead of time at one or more resolutions. Like Font, it picks the smallest
/// face at least as large as the surface needs, so artwork is only shrunk when a larger face is
/// available. The arrays normally live in flash and are never copied.
class Picture {
 public:
  static constexpr int kMaxFaces = 6;

  Picture() = default;
  Picture(ImageView image) : images_{image}, count_(1) {}
  Picture(const ImageView* images, int count) : count_(std::min(count, kMaxFaces)) {
    for (int i = 0; i < count_; ++i) images_[static_cast<size_t>(i)] = images[i];
  }

  void draw(IDrawSurface& surface, int x, int y) const {
    if (count_ == 0) return;
    surface.drawImage(faceFor(surface.scale()), x, y);
  }

 private:
  const ImageView& faceFor(float scale) const {
    for (int i = 0; i < count_; ++i) {
      const ImageView& image = images_[static_cast<size_t>(i)];
      if (image.resolution >= scale - 0.001f) return image;
    }
    return images_[static_cast<size_t>(count_ - 1)];
  }

  std::array<ImageView, kMaxFaces> images_{};
  int count_ = 0;
};

}  // namespace gauge
