// The pretend engine computer and its scripted drive.

#include "test_support.h"

using namespace support;

TEST_CASE("fake ecu: PIDs, clamping, profile and offline") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ObdData d;

  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidSupported01To20, d));
  CHECK(d.count == 4);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidBaroKpa, d));
  CHECK(d.bytes[0] == 101);
  CHECK_FALSE(ecu.answer(0x09, obd::kPidMapKpa, d));         // wrong mode
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, 0xEE, d));  // unknown pid

  ecu.holdMapKpa(999.0f);  // clamps to a byte
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.bytes[0] == 255);
  ecu.holdMapKpa(-5.0f);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.bytes[0] == 0);

  ecu.followDriveProfile();
  clock.advance(6000);  // mid full-boost pull
  CHECK(ecu.mapKpa() > 150.0f);

  ecu.setOnline(false);
  CHECK_FALSE(ecu.online());
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, obd::kPidMapKpa, d));
}

TEST_CASE("drive profile: bounded, periodic, hits both extremes") {
  float lo = 1e9f, hi = -1e9f;
  for (uint32_t t = 0; t < 20000; t += 50) {
    const float psi = driveProfileBoostPsi(t);
    lo = std::min(lo, psi);
    hi = std::max(hi, psi);
  }
  CHECK(lo == doctest::Approx(-12.0f).epsilon(0.05));
  CHECK(hi > 17.0f);
  CHECK(driveProfileBoostPsi(1234) == doctest::Approx(driveProfileBoostPsi(21234)));
  CHECK(driveProfileBoostPsi(19999) == doctest::Approx(-11.0f).epsilon(0.01));
}
