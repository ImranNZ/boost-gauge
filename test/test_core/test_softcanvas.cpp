#include <doctest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "Image.h"
#include "SoftCanvas.h"

using namespace gauge;

namespace {

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

int countNonBlack(const Image& img) {
  int n = 0;
  for (int y = 0; y < img.height(); ++y)
    for (int x = 0; x < img.width(); ++x)
      if (!same(img.get(x, y), kBlack)) ++n;
  return n;
}

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

// ------------------------------------------------------------------ Image --

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
    for (int x = 0; x < 240; ++x)
      img.set(x, y, Color{uint8_t(x), uint8_t(y), uint8_t(x ^ y)});

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

// ------------------------------------------------------------- SoftCanvas --

TEST_CASE("softcanvas: size, clear, present counter") {
  SoftCanvas c(32, 16);
  CHECK(c.width() == 32);
  CHECK(c.height() == 16);
  c.clear(Color{9, 8, 7});
  CHECK(c.image().get(31, 15).r == 9);
  CHECK(c.framesPresented() == 0);
  c.present();
  CHECK(c.framesPresented() == 1);
}

TEST_CASE("softcanvas: circle") {
  SoftCanvas c(40, 40);
  c.fillCircle(20, 20, 5, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));
  CHECK(same(c.image().get(25, 20), kWhite));
  CHECK_FALSE(same(c.image().get(26, 20), kWhite));
  CHECK_FALSE(same(c.image().get(25, 25), kWhite));
}

TEST_CASE("softcanvas: arc quadrants, negative start and wrap-around") {
  SoftCanvas c(100, 100);

  c.fillArc(50, 50, 10, 20, -90.0f, 0.0f, kWhite);  // top-right quadrant
  CHECK(same(c.image().get(60, 40), kWhite));
  CHECK_FALSE(same(c.image().get(40, 60), kWhite));
  CHECK_FALSE(same(c.image().get(50, 50), kWhite));  // inside the hole

  c.clear(kBlack);
  c.fillArc(50, 50, 10, 20, 350.0f, 10.0f, kWhite);  // wraps through 0 degrees
  CHECK(same(c.image().get(65, 50), kWhite));
  CHECK_FALSE(same(c.image().get(50, 65), kWhite));

  c.clear(kBlack);
  c.fillArc(50, 50, 10, 20, 90.0f, 180.0f, kWhite);  // bottom-left (y grows down)
  CHECK(same(c.image().get(40, 60), kWhite));
  CHECK_FALSE(same(c.image().get(60, 40), kWhite));
}

TEST_CASE("softcanvas: triangle fills regardless of winding") {
  SoftCanvas a(40, 40);
  a.fillTriangle(10, 10, 30, 10, 10, 30, kWhite);
  CHECK(same(a.image().get(14, 14), kWhite));
  CHECK_FALSE(same(a.image().get(28, 28), kWhite));

  SoftCanvas b(40, 40);
  b.fillTriangle(10, 10, 10, 30, 30, 10, kWhite);  // opposite winding
  CHECK(same(b.image().get(14, 14), kWhite));
  CHECK(countNonBlack(a.image()) == countNonBlack(b.image()));
}

TEST_CASE("softcanvas: lines, thickness clamp and zero length") {
  SoftCanvas c(40, 40);
  c.drawLine(5, 20, 35, 20, 1, kWhite);
  CHECK(same(c.image().get(20, 20), kWhite));
  CHECK_FALSE(same(c.image().get(20, 18), kWhite));

  c.clear(kBlack);
  c.drawLine(5, 20, 35, 20, 0, kWhite);  // clamped up to 1px
  CHECK(same(c.image().get(20, 20), kWhite));

  c.clear(kBlack);
  c.drawLine(5, 20, 35, 20, 7, kWhite);
  CHECK(same(c.image().get(20, 17), kWhite));

  c.clear(kBlack);
  c.drawLine(10, 10, 10, 10, 3, kWhite);  // degenerate segment
  CHECK(same(c.image().get(10, 10), kWhite));
}

TEST_CASE("softcanvas: text alignment, scale and unknown glyphs") {
  // '-' draws a 5px bar on glyph row 3.
  SoftCanvas left(60, 20);
  left.drawText(20, 0, "-", 1, TextAlign::Left, kWhite);
  CHECK(same(left.image().get(20, 3), kWhite));
  CHECK(same(left.image().get(24, 3), kWhite));
  CHECK_FALSE(same(left.image().get(25, 3), kWhite));

  SoftCanvas right(60, 20);
  right.drawText(20, 0, "-", 1, TextAlign::Right, kWhite);  // cell is 6 wide
  CHECK(same(right.image().get(14, 3), kWhite));
  CHECK_FALSE(same(right.image().get(20, 3), kWhite));

  SoftCanvas centre(60, 20);
  centre.drawText(20, 0, "-", 1, TextAlign::Center, kWhite);
  CHECK(same(centre.image().get(17, 3), kWhite));
  CHECK_FALSE(same(centre.image().get(16, 3), kWhite));

  SoftCanvas scaled(60, 20);
  scaled.drawText(0, 0, "-", 3, TextAlign::Left, kWhite);
  CHECK(same(scaled.image().get(14, 9), kWhite));  // 5*3 wide, row 3 * 3 tall
  CHECK(same(scaled.image().get(0, 11), kWhite));

  SoftCanvas unknown(60, 20);
  unknown.drawText(0, 0, "Z~", 2, TextAlign::Left, kWhite);
  CHECK(countNonBlack(unknown.image()) == 0);
}

TEST_CASE("softcanvas: every glyph the gauge uses renders something") {
  const std::string used = "0123456789-+.ABEIKLNOPST";
  for (char ch : used) {
    SoftCanvas c(10, 10);
    const char text[2] = {ch, 0};
    c.drawText(0, 0, text, 1, TextAlign::Left, kWhite);
    CAPTURE(ch);
    CHECK(countNonBlack(c.image()) > 0);
  }
}
