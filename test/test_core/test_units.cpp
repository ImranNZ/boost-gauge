// Pressure units and colour maths.

#include "test_support.h"

using namespace support;

TEST_CASE("blend: all under, all over, and a rounded mix") {
  const Color under{0, 100, 200};
  const Color over{255, 50, 0};
  CHECK(same(blend(under, over, 0), under));
  CHECK(same(blend(under, over, 255), over));

  const Color half = blend(under, over, 128);
  CHECK(half.r == 128);  // (0*127 + 255*128 + 127) / 255
  CHECK(half.g == 75);
  CHECK(half.b == 100);
}

TEST_CASE("units: psi <-> kPa round trip") {
  const float psi = 12.5f;
  CHECK(kpaToPsi(psiToKpa(psi)) == doctest::Approx(psi));
  CHECK(psiToKpa(1.0f) == doctest::Approx(kKpaPerPsi));
}

TEST_CASE("units: kPa <-> psi and boost") {
  CHECK(kpaToPsi(100.0f) == doctest::Approx(14.5038f).epsilon(0.001));
  CHECK(boostPsi(101.0f, 101.0f) == doctest::Approx(0.0f));
  CHECK(boostPsi(170.0f, 101.0f) == doctest::Approx(10.0f).epsilon(0.01));
  CHECK(boostPsi(30.0f, 101.0f) < 0.0f);  // vacuum
}

TEST_CASE("rgb565ToColor expands 5/6/5 bits to full range") {
  CHECK(same(rgb565ToColor(0x0000), kBlack));
  CHECK(same(rgb565ToColor(0xFFFF), kWhite));
  CHECK(same(rgb565ToColor(0xF800), Color{255, 0, 0}));
  CHECK(same(rgb565ToColor(0x07E0), Color{0, 255, 0}));
  CHECK(same(rgb565ToColor(0x001F), Color{0, 0, 255}));
}
