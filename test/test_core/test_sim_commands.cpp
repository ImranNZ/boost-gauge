#include <doctest.h>

#include "FakeEcu.h"
#include "ManualClock.h"
#include "ReadingSource.h"
#include "SimCommands.h"
#include "ThrottledSource.h"
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
  CHECK_FALSE(applyEcuCommand(ecu, "boost"));        // missing argument
  CHECK_FALSE(applyEcuCommand(ecu, "boostabc"));     // no separator
  CHECK_FALSE(applyEcuCommand(ecu, "boost abc"));
  CHECK_FALSE(applyEcuCommand(ecu, "boost 5 psi"));  // trailing junk
  CHECK_FALSE(applyEcuCommand(ecu, "boost 1234567890123456789012345678901234567890"));  // too long
}

namespace {

/// Source whose reading and seq are set by the test.
class FixedSource : public IReadingSource {
 public:
  Sample latest() override { return sample; }
  Sample sample;
};

}  // namespace

TEST_CASE("throttled source: fresh readings at most once per interval") {
  ManualClock clock;
  FixedSource inner;
  ThrottledSource source(inner, clock, 100);

  inner.sample = {{1.0f, true}, 1};
  CHECK(source.latest().seq == 1);  // first call always fetches

  inner.sample = {{2.0f, true}, 2};
  clock.advance(99);
  CHECK(source.latest().seq == 1);  // not due yet: same sample, same seq
  CHECK(source.latest().reading.psi == doctest::Approx(1.0f));

  clock.advance(1);
  CHECK(source.latest().seq == 2);  // due
  CHECK(source.latest().reading.psi == doctest::Approx(2.0f));
}

TEST_CASE("rate command sets the interval and validates input") {
  ManualClock clock;
  FixedSource inner;
  ThrottledSource source(inner, clock, 40);

  CHECK(applyRateCommand(source, "rate 10"));
  CHECK(source.intervalMs() == 100);
  CHECK(applyRateCommand(source, " rate 25 \n"));
  CHECK(source.intervalMs() == 40);
  CHECK(applyRateCommand(source, "rate 100"));
  CHECK(source.intervalMs() == 10);

  CHECK_FALSE(applyRateCommand(source, "rate 0"));     // below range
  CHECK_FALSE(applyRateCommand(source, "rate 500"));   // above range
  CHECK_FALSE(applyRateCommand(source, "rate fast"));
  CHECK_FALSE(applyRateCommand(source, "rate"));
  CHECK_FALSE(applyRateCommand(source, "boost 5"));    // not a rate command
  CHECK(source.intervalMs() == 10);                    // unchanged by failures
}
