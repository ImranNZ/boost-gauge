#include <doctest.h>

#include "EcuCommands.h"
#include "FakeEcu.h"
#include "ManualClock.h"
#include "Units.h"

using namespace gauge;

TEST_CASE("ecu commands: drive, boost, offline, online") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ObdData d;

  CHECK(applyEcuCommand(ecu, "boost 10"));
  CHECK(ecu.mapKpa() == doctest::Approx(101.0f + psiToKpa(10.0f)));

  CHECK(applyEcuCommand(ecu, "  boost -5.5 \r\n"));  // whitespace and negatives
  CHECK(ecu.mapKpa() == doctest::Approx(101.0f + psiToKpa(-5.5f)));

  CHECK(applyEcuCommand(ecu, "offline"));
  CHECK_FALSE(ecu.answer(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(applyEcuCommand(ecu, "online"));
  CHECK(ecu.answer(obd::kModeCurrentData, obd::kPidMapKpa, d));

  clock.advance(6000);  // mid full-boost pull of the scripted drive
  CHECK(applyEcuCommand(ecu, "drive"));
  CHECK(ecu.mapKpa() > 150.0f);
}

TEST_CASE("ecu commands: values are clamped to what the sensor can report") {
  ManualClock clock;
  FakeEcu ecu(clock);

  CHECK(applyEcuCommand(ecu, "boost 500"));
  CHECK(ecu.mapKpa() == doctest::Approx(255.0f));
  CHECK(applyEcuCommand(ecu, "boost -500"));
  CHECK(ecu.mapKpa() == doctest::Approx(0.0f));
}

TEST_CASE("ecu commands: rejects unknown and malformed input") {
  ManualClock clock;
  FakeEcu ecu(clock);

  CHECK_FALSE(applyEcuCommand(ecu, ""));
  CHECK_FALSE(applyEcuCommand(ecu, "   "));
  CHECK_FALSE(applyEcuCommand(ecu, "launch"));
  CHECK_FALSE(applyEcuCommand(ecu, "boost"));       // missing argument
  CHECK_FALSE(applyEcuCommand(ecu, "boost abc"));
  CHECK_FALSE(applyEcuCommand(ecu, "boost 5 psi"));  // trailing junk
}
