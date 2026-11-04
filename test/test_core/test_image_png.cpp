// The Image bitmap and the PNG writer.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "test_support.h"

using namespace support;

namespace {

// ---- minimal PNG reader used to verify the writer ------------------------

uint32_t be32(const std::vector<uint8_t>& v, size_t i) {
  return (uint32_t(v[i]) << 24) | (uint32_t(v[i + 1]) << 16) | (uint32_t(v[i + 2]) << 8) | v[i + 3];
}

uint32_t crc32(const uint8_t* d, size_t n) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) {
    crc ^= d[i];
    for (int k = 0; k < 8; ++k) crc = (crc & 1) ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
  }
  return crc ^ 0xFFFFFFFFu;
}

struct DecodedPng {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> raw;  // unfiltered scanlines incl. filter byte
  bool crcOk = true;
  bool adlerOk = false;
  bool endSeen = false;
};

DecodedPng decodePng(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<uint8_t> f((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  DecodedPng out;
  REQUIRE(f.size() > 8);
  const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  REQUIRE(std::equal(sig, sig + 8, f.begin()));

  std::vector<uint8_t> idat;
  size_t pos = 8;
  while (pos + 12 <= f.size()) {
    const uint32_t len = be32(f, pos);
    const std::string type(f.begin() + pos + 4, f.begin() + pos + 8);
    if (crc32(&f[pos + 4], len + 4) != be32(f, pos + 8 + len)) out.crcOk = false;
    if (type == "IHDR") {
      out.width = be32(f, pos + 8);
      out.height = be32(f, pos + 12);
    } else if (type == "IDAT") {
      idat.insert(idat.end(), f.begin() + pos + 8, f.begin() + pos + 8 + len);
    } else if (type == "IEND") {
      out.endSeen = true;
    }
    pos += 12 + len;
  }

  // zlib: 2-byte header, stored deflate blocks, 4-byte Adler-32.
  REQUIRE(idat.size() > 6);
  size_t p = 2;
  for (;;) {
    const bool last = idat[p] & 1;
    REQUIRE((idat[p] & 0x06) == 0);  // stored block
    const size_t n = idat[p + 1] | (idat[p + 2] << 8);
    const size_t nn = idat[p + 3] | (idat[p + 4] << 8);
    REQUIRE((n ^ nn) == 0xFFFF);
    out.raw.insert(out.raw.end(), idat.begin() + p + 5, idat.begin() + p + 5 + n);
    p += 5 + n;
    if (last) break;
  }
  uint32_t a = 1, b = 0;
  for (uint8_t byte : out.raw) {
    a = (a + byte) % 65521u;
    b = (b + a) % 65521u;
  }
  out.adlerOk = be32(idat, p) == ((b << 16) | a);
  return out;
}

std::string tempPng(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace

TEST_CASE("image: bounds-checked set/get, fill and blit") {
  Image img(4, 3);
  CHECK(img.width() == 4);
  CHECK(img.height() == 3);

  img.set(-1, 0, kWhite);
  img.set(0, -1, kWhite);
  img.set(4, 0, kWhite);
  img.set(0, 3, kWhite);
  CHECK(countNonBlack(img) == 0);  // all out of range, ignored

  CHECK(same(img.get(-1, 0), kBlack));
  CHECK(same(img.get(0, -1), kBlack));
  CHECK(same(img.get(4, 0), kBlack));
  CHECK(same(img.get(0, 3), kBlack));

  img.fill(Color{1, 2, 3});
  CHECK(img.get(3, 2).b == 3);

  Image big(6, 6);
  big.blit(img, 4, 4);  // partly off the edge
  CHECK(same(big.get(4, 4), Color{1, 2, 3}));
  CHECK(same(big.get(3, 3), kBlack));
}

TEST_CASE("png writer: valid structure, CRCs, Adler and pixel data") {
  Image img(240, 240);  // 172,800 bytes raw => several stored blocks
  for (int y = 0; y < 240; ++y)
    for (int x = 0; x < 240; ++x) img.set(x, y, Color{uint8_t(x), uint8_t(y), uint8_t(x ^ y)});

  const std::string path = tempPng("boost_gauge_test_big.png");
  REQUIRE(writePng(img, path));
  const DecodedPng png = decodePng(path);

  CHECK(png.width == 240);
  CHECK(png.height == 240);
  CHECK(png.crcOk);
  CHECK(png.adlerOk);
  CHECK(png.endSeen);
  REQUIRE(png.raw.size() == 240u * (240u * 3u + 1u));
  CHECK(png.raw[0] == 0);  // filter type
  const size_t row = 17, col = 33;
  const size_t at = row * (240 * 3 + 1) + 1 + col * 3;
  CHECK(png.raw[at] == col);
  CHECK(png.raw[at + 1] == row);
  CHECK(png.raw[at + 2] == (col ^ row));
  std::remove(path.c_str());
}

TEST_CASE("png writer: tiny single-block image") {
  Image img(2, 2);
  img.fill(kWhite);
  const std::string path = tempPng("boost_gauge_test_tiny.png");
  REQUIRE(writePng(img, path));
  const DecodedPng png = decodePng(path);
  CHECK(png.crcOk);
  CHECK(png.adlerOk);
  CHECK(png.raw.size() == 2u * 7u);
  std::remove(path.c_str());
}

TEST_CASE("png writer: reports I/O failure") {
  Image img(2, 2);
  CHECK_FALSE(writePng(img, "Z:/definitely/not/a/real/dir/x.png"));
}
