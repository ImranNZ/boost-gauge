#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "render/ICanvas.h"

namespace gauge {

/// Owned RGB888 bitmap.
class Image {
 public:
  Image(int width, int height)
      : width_(width), height_(height), pixels_(static_cast<size_t>(width) * height * 3) {}

  int width() const { return width_; }
  int height() const { return height_; }

  void set(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    uint8_t* p = &pixels_[(static_cast<size_t>(y) * width_ + x) * 3];
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }

  Color get(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return {};
    const uint8_t* p = &pixels_[(static_cast<size_t>(y) * width_ + x) * 3];
    return {p[0], p[1], p[2]};
  }

  void fill(Color c) {
    for (int y = 0; y < height_; ++y)
      for (int x = 0; x < width_; ++x) set(x, y, c);
  }

  /// Copy `src` into this image with its top-left at (dx, dy).
  void blit(const Image& src, int dx, int dy) {
    for (int y = 0; y < src.height(); ++y)
      for (int x = 0; x < src.width(); ++x) set(dx + x, dy + y, src.get(x, y));
  }

  const std::vector<uint8_t>& pixels() const { return pixels_; }

 private:
  int width_;
  int height_;
  std::vector<uint8_t> pixels_;
};

/// Write an image as an (uncompressed) PNG. Returns false on I/O failure.
bool writePng(const Image& image, const std::string& path);

}  // namespace gauge
