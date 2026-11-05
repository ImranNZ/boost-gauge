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
  CHECK_FALSE(ecu.answer(0x09, obd::kPidMapKpa, d));        // wrong mode
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

TEST_CASE("fake ecu: the support bitmaps follow the PIDs it implements") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ObdData d;

  // By default 0B and 33: PID 00 has 0B and says a second range exists; PID 20 has 33 and no more.
  REQUIRE(ecu.answer(obd::kModeCurrentData, 0x00, d));
  CHECK((d.bytes == std::array<uint8_t, 4>{0x00, 0x20, 0x00, 0x01}));
  REQUIRE(ecu.answer(obd::kModeCurrentData, 0x20, d));
  CHECK((d.bytes == std::array<uint8_t, 4>{0x00, 0x00, 0x20, 0x00}));
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, 0x40, d));  // no PIDs there: no such support PID

  ecu.setSupported(obd::kPidBaroKpa, false);
  CHECK_FALSE(ecu.supported(obd::kPidBaroKpa));
  REQUIRE(ecu.answer(obd::kModeCurrentData, 0x00, d));
  CHECK(d.bytes[3] == 0x00);  // nothing beyond the first range any more
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, 0x20, d));
  CHECK_FALSE(
      ecu.answer(obd::kModeCurrentData, obd::kPidBaroKpa, d));  // and it no longer answers 33

  ecu.setSupported(0x0C, true);  // engine speed: a PID this fake has no data for
  CHECK(ecu.supported(0x0C));
  REQUIRE(ecu.answer(obd::kModeCurrentData, 0x00, d));
  CHECK((d.bytes[1] & 0x10) != 0);  // bit for PID 0C: byte 1 (PIDs 09-10), fourth from the top
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, 0x0C, d));  // listed, but no answer to give
}

TEST_CASE("fake ecu: coolant, intake air and voltage, once they are listed") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ObdData d;

  // Not implemented until a test says so.
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, obd::kPidCoolantTemp, d));
  ecu.setSupported(obd::kPidCoolantTemp, true);
  ecu.setSupported(obd::kPidIntakeAirTemp, true);
  ecu.setSupported(obd::kPidControlModuleVolts, true);

  ecu.setCoolantC(88.0f);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidCoolantTemp, d));
  CHECK(d.count == 1);
  CHECK(obd::coolantTempC(d) == doctest::Approx(88.0f));

  ecu.setIntakeAirC(-5.0f);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidIntakeAirTemp, d));
  CHECK(obd::intakeAirTempC(d) == doctest::Approx(-5.0f));

  ecu.setVoltage(12.634f);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidControlModuleVolts, d));
  CHECK(d.count == 2);
  CHECK(obd::controlModuleVolts(d) == doctest::Approx(12.634f).epsilon(0.0001));

  ecu.setCoolantC(500.0f);  // clamps to a byte
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidCoolantTemp, d));
  CHECK(d.bytes[0] == 255);
}

TEST_CASE(
    "drive profiles: coolant, intake air and voltage swing within their ranges, once a loop") {
  float coolantLow = 1000.0f, coolantHigh = -1000.0f;
  float intakeLow = 1000.0f, intakeHigh = -1000.0f;
  float voltsLow = 1000.0f, voltsHigh = -1000.0f;
  for (uint32_t t = 0; t < 20000; t += 100) {
    coolantLow = std::min(coolantLow, driveProfileCoolantC(t));
    coolantHigh = std::max(coolantHigh, driveProfileCoolantC(t));
    intakeLow = std::min(intakeLow, driveProfileIntakeAirC(t));
    intakeHigh = std::max(intakeHigh, driveProfileIntakeAirC(t));
    voltsLow = std::min(voltsLow, driveProfileVolts(t));
    voltsHigh = std::max(voltsHigh, driveProfileVolts(t));
    CHECK(driveProfileCoolantC(t) == doctest::Approx(driveProfileCoolantC(t + 20000)));  // periodic
  }
  CHECK(coolantLow == doctest::Approx(76.0f).epsilon(0.01));
  CHECK(coolantHigh == doctest::Approx(100.0f).epsilon(0.01));
  CHECK(intakeLow == doctest::Approx(22.0f).epsilon(0.01));
  CHECK(intakeHigh == doctest::Approx(58.0f).epsilon(0.01));
  CHECK(voltsLow == doctest::Approx(13.4f).epsilon(0.01));
  CHECK(voltsHigh == doctest::Approx(14.4f).epsilon(0.01));
}

TEST_CASE(
    "fake ecu: the other readings follow the drive cycle until pinned, and again after drive") {
  ManualClock clock;
  FakeEcu ecu(clock);

  clock.advance(5000);  // the top of the swing
  CHECK(ecu.coolantC() == doctest::Approx(100.0f).epsilon(0.01));
  CHECK(ecu.intakeAirC() == doctest::Approx(58.0f).epsilon(0.01));
  CHECK(ecu.voltage() == doctest::Approx(14.4f).epsilon(0.01));

  ecu.setCoolantC(50.0f);
  ecu.setIntakeAirC(10.0f);
  ecu.setVoltage(12.0f);
  clock.advance(5000);  // time moves on; pinned readings do not
  CHECK(ecu.coolantC() == doctest::Approx(50.0f));
  CHECK(ecu.intakeAirC() == doctest::Approx(10.0f));
  CHECK(ecu.voltage() == doctest::Approx(12.0f));

  ecu.followDriveProfile();
  CHECK(ecu.coolantC() == doctest::Approx(driveProfileCoolantC(10000)));
  CHECK(ecu.intakeAirC() == doctest::Approx(driveProfileIntakeAirC(10000)));
  CHECK(ecu.voltage() == doctest::Approx(driveProfileVolts(10000)));
}
