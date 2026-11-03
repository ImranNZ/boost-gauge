#include "Image.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace gauge {
namespace {

uint32_t crc32(const uint8_t* data, size_t len, uint32_t crc = 0xFFFFFFFFu) {
  static const auto table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t n = 0; n < 256; ++n) {
      uint32_t c = n;
      for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      t[n] = c;
    }
    return t;
  }();
  for (size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  return crc;
}

void putBe32(std::vector<uint8_t>& v, uint32_t x) {
  v.push_back(static_cast<uint8_t>(x >> 24));
  v.push_back(static_cast<uint8_t>(x >> 16));
  v.push_back(static_cast<uint8_t>(x >> 8));
  v.push_back(static_cast<uint8_t>(x));
}

void putChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& body) {
  putBe32(out, static_cast<uint32_t>(body.size()));
  std::vector<uint8_t> typed(type, type + 4);
  typed.insert(typed.end(), body.begin(), body.end());
  out.insert(out.end(), typed.begin(), typed.end());
  putBe32(out, crc32(typed.data(), typed.size()) ^ 0xFFFFFFFFu);
}

/// zlib stream using "stored" (uncompressed) deflate blocks.
std::vector<uint8_t> zlibStore(const std::vector<uint8_t>& raw) {
  std::vector<uint8_t> z = {0x78, 0x01};
  size_t pos = 0;
  do {
    const size_t n = std::min<size_t>(65535, raw.size() - pos);
    const bool last = pos + n >= raw.size();
    z.push_back(last ? 1 : 0);
    z.push_back(static_cast<uint8_t>(n & 0xFF));
    z.push_back(static_cast<uint8_t>(n >> 8));
    z.push_back(static_cast<uint8_t>(~n & 0xFF));
    z.push_back(static_cast<uint8_t>((~n >> 8) & 0xFF));
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
  } while (pos < raw.size());

  uint32_t a = 1, b = 0;
  for (uint8_t byte : raw) {
    a = (a + byte) % 65521u;
    b = (b + a) % 65521u;
  }
  putBe32(z, (b << 16) | a);
  return z;
}

}  // namespace

bool writePng(const Image& image, const std::string& path) {
  std::vector<uint8_t> raw;
  const size_t stride = static_cast<size_t>(image.width()) * 3;
  raw.reserve((stride + 1) * image.height());
  for (int y = 0; y < image.height(); ++y) {
    raw.push_back(0);  // filter: none
    const uint8_t* row = &image.pixels()[y * stride];
    raw.insert(raw.end(), row, row + stride);
  }

  std::vector<uint8_t> ihdr;
  putBe32(ihdr, static_cast<uint32_t>(image.width()));
  putBe32(ihdr, static_cast<uint32_t>(image.height()));
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});  // 8-bit RGB

  std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  putChunk(png, "IHDR", ihdr);
  putChunk(png, "IDAT", zlibStore(raw));
  putChunk(png, "IEND", {});

  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  const bool ok = std::fwrite(png.data(), 1, png.size(), f) == png.size();
  std::fclose(f);
  return ok;
}

}  // namespace gauge
