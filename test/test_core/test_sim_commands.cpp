// The preview's mock controls: ECU, data-rate and look commands.

#include "test_support.h"

using namespace support;

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
  CHECK_FALSE(applyEcuCommand(ecu, "boost"));     // missing argument
  CHECK_FALSE(applyEcuCommand(ecu, "boostabc"));  // no separator
  CHECK_FALSE(applyEcuCommand(ecu, "boost abc"));
  CHECK_FALSE(applyEcuCommand(ecu, "boost 5 psi"));  // trailing junk
  CHECK_FALSE(applyEcuCommand(ecu, "boost 1234567890123456789012345678901234567890"));  // too long
}

TEST_CASE("throttled source: fresh readings at most once per interval") {
  ManualClock clock;
  FixedSource inner;
  ThrottledSource source(inner, clock, 100);

  inner.sample = {{1.0f, true}, 1};
  CHECK(source.latest().seq == 1);  // first call always fetches

  inner.sample = {{2.0f, true}, 2};
  clock.advance(99);
  CHECK(source.latest().seq == 1);  // not due yet: same sample, same seq
  CHECK(source.latest().reading.value == doctest::Approx(1.0f));

  clock.advance(1);
  CHECK(source.latest().seq == 2);  // due
  CHECK(source.latest().reading.value == doctest::Approx(2.0f));
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

  CHECK_FALSE(applyRateCommand(source, "rate 0"));    // below range
  CHECK_FALSE(applyRateCommand(source, "rate 500"));  // above range
  CHECK_FALSE(applyRateCommand(source, "rate fast"));
  CHECK_FALSE(applyRateCommand(source, "rate"));
  CHECK_FALSE(applyRateCommand(source, "boost 5"));  // not a rate command
  CHECK(source.intervalMs() == 10);                  // unchanged by failures
}

TEST_CASE("parseLookCommand recognises 'look <name>'") {
  std::string name;
  CHECK(parseLookCommand("look retro", name));
  CHECK(name == "retro");
  CHECK(parseLookCommand("  look   classic \r\n", name));
  CHECK(name == "classic");

  CHECK_FALSE(parseLookCommand("look", name));
  CHECK_FALSE(parseLookCommand("look   ", name));
  CHECK_FALSE(parseLookCommand("looks retro", name));
  CHECK_FALSE(parseLookCommand("look two words", name));
  CHECK_FALSE(parseLookCommand("boost 5", name));
  CHECK_FALSE(parseLookCommand("", name));
}

TEST_CASE("parseUnitCommand recognises 'unit <name>'") {
  std::string name;
  CHECK(parseUnitCommand("unit bar", name));
  CHECK(name == "bar");
  CHECK(parseUnitCommand("  unit   kpa \r\n", name));
  CHECK(name == "kpa");

  CHECK_FALSE(parseUnitCommand("unit", name));
  CHECK_FALSE(parseUnitCommand("unit   ", name));
  CHECK_FALSE(parseUnitCommand("units bar", name));
  CHECK_FALSE(parseUnitCommand("unit bar psi", name));
  CHECK_FALSE(parseUnitCommand("look bar", name));
}

TEST_CASE("throttled source: reset makes the next call fetch afresh") {
  ManualClock clock;
  FixedSource inner;
  ThrottledSource source(inner, clock, 100);

  inner.sample = {{1.0f, true}, 1};
  CHECK(source.latest().seq == 1);
  inner.sample = {{2.0f, true}, 2};
  CHECK(source.latest().seq == 1);  // not due: the cached one

  source.reset();
  CHECK(source.latest().seq == 2);  // fetched straight away
}

TEST_CASE("ecu commands pin coolant, intake air and voltage, and drive releases them") {
  ManualClock clock;
  FakeEcu ecu(clock);

  CHECK(applyEcuCommand(ecu, "coolant 95.5"));
  CHECK(ecu.coolantC() == doctest::Approx(95.5f));
  CHECK(applyEcuCommand(ecu, "intake 12"));
  CHECK(ecu.intakeAirC() == doctest::Approx(12.0f));
  CHECK(applyEcuCommand(ecu, "volts 12.6"));
  CHECK(ecu.voltage() == doctest::Approx(12.6f));

  // Clamped to what the PID can report.
  CHECK(applyEcuCommand(ecu, "coolant 999"));
  CHECK(ecu.coolantC() == doctest::Approx(215.0f));
  CHECK(applyEcuCommand(ecu, "intake -99"));
  CHECK(ecu.intakeAirC() == doctest::Approx(-40.0f));
  CHECK(applyEcuCommand(ecu, "volts 999"));
  CHECK(ecu.voltage() == doctest::Approx(65.0f));

  CHECK_FALSE(applyEcuCommand(ecu, "coolant"));
  CHECK_FALSE(applyEcuCommand(ecu, "coolant hot"));
  CHECK_FALSE(applyEcuCommand(ecu, "volts"));

  CHECK(applyEcuCommand(ecu, "drive"));
  clock.advance(5000);
  CHECK(ecu.coolantC() == doctest::Approx(100.0f).epsilon(0.01));  // following the cycle again
}

TEST_CASE("parseChannelCommand recognises 'channel <name>'") {
  std::string name;
  CHECK(parseChannelCommand("channel coolant", name));
  CHECK(name == "coolant");
  CHECK(parseChannelCommand("  channel   voltage \r\n", name));
  CHECK(name == "voltage");

  CHECK_FALSE(parseChannelCommand("channel", name));
  CHECK_FALSE(parseChannelCommand("channels coolant", name));
  CHECK_FALSE(parseChannelCommand("channel two words", name));
  CHECK_FALSE(parseChannelCommand("look coolant", name));
}

TEST_CASE("parseButtonCommand recognises 'button next' and 'button select'") {
  InputEvent event = InputEvent::None;
  CHECK(parseButtonCommand("button next", event));
  CHECK(event == InputEvent::Next);
  CHECK(parseButtonCommand("  button   select \r\n", event));
  CHECK(event == InputEvent::Select);

  event = InputEvent::None;
  CHECK_FALSE(parseButtonCommand("button", event));
  CHECK_FALSE(parseButtonCommand("button back", event));
  CHECK_FALSE(parseButtonCommand("buttons next", event));
  CHECK_FALSE(parseButtonCommand("look next", event));
  CHECK(event == InputEvent::None);
}
