// PidChannel and the PID decoders: coolant temperature, intake air temperature, battery voltage.

#include "test_support.h"

using namespace support;

namespace {

/// A car on a fake bus, with the PIDs a test lists.
struct Car {
  ManualClock clock;
  FakeEcu ecu{clock};
  FakeCanBus bus{ecu};
  CanObdClient client{bus, clock};

  Car() {
    client.begin();
    ecu.setSupported(obd::kPidCoolantTemp, true);
    ecu.setSupported(obd::kPidIntakeAirTemp, true);
    ecu.setSupported(obd::kPidControlModuleVolts, true);
  }
};

/// A client that returns the same bytes for every PID.
class FixedClient : public IObdClient {
 public:
  ObdData reply;
  bool ok = true;

  bool begin() override { return true; }
  bool readPid(uint8_t, uint8_t, ObdData& out) override {
    out = reply;
    return ok;
  }
};

ObdData bytes(uint8_t a, uint8_t b = 0, uint8_t count = 2) {
  ObdData data;
  data.bytes = {a, b, 0, 0};
  data.count = count;
  return data;
}

}  // namespace

TEST_CASE("pid decode: temperatures are the byte minus 40 degrees") {
  CHECK(obd::coolantTempC(bytes(130)) == doctest::Approx(90.0f));
  CHECK(obd::coolantTempC(bytes(40)) == doctest::Approx(0.0f));
  CHECK(obd::coolantTempC(bytes(0)) == doctest::Approx(-40.0f));
  CHECK(obd::coolantTempC(bytes(255)) == doctest::Approx(215.0f));
  CHECK(obd::intakeAirTempC(bytes(75)) == doctest::Approx(35.0f));
  CHECK(obd::intakeAirTempC(bytes(0)) == doctest::Approx(-40.0f));
}

TEST_CASE("pid decode: control module voltage is 256 A + B millivolts") {
  CHECK(obd::controlModuleVolts(bytes(0x31, 0x58)) == doctest::Approx(12.632f));  // 12632 mV
  CHECK(obd::controlModuleVolts(bytes(0, 0)) == doctest::Approx(0.0f));
  CHECK(obd::controlModuleVolts(bytes(0xFF, 0xFF)) == doctest::Approx(65.535f));
}

TEST_CASE("pid channel: the standard channels read their PID and decode it") {
  Car car;
  float value = 0.0f;

  PidChannel coolant = PidChannel::coolant();
  car.ecu.setCoolantC(88.0f);
  CHECK(coolant.read(car.client, value) == ReadStatus::Ok);
  CHECK(value == doctest::Approx(88.0f));

  PidChannel intake = PidChannel::intakeAir();
  car.ecu.setIntakeAirC(-5.0f);
  CHECK(intake.read(car.client, value) == ReadStatus::Ok);
  CHECK(value == doctest::Approx(-5.0f));

  PidChannel voltage = PidChannel::voltage();
  car.ecu.setVoltage(13.9f);
  CHECK(voltage.read(car.client, value) == ReadStatus::Ok);
  CHECK(value == doctest::Approx(13.9f).epsilon(0.001));
}

TEST_CASE("pid channel: the slow channels are read about once a second") {
  CHECK(PidChannel::coolant().periodMs() == 1000);
  CHECK(PidChannel::intakeAir().periodMs() == 1000);
  CHECK(PidChannel::voltage().periodMs() == 1000);
  CHECK(PidChannel(0x0C, 2, &obd::controlModuleVolts, 250).periodMs() == 250);
}

TEST_CASE("pid channel: a car that does not report the PID is not asked for it") {
  Car car;
  car.ecu.setSupported(obd::kPidCoolantTemp, false);
  CountingObdClient counting(car.client);
  PidSupport support;
  REQUIRE(support.discover(counting));

  PidChannel coolant = PidChannel::coolant();
  coolant.onLinked(support);
  float value = 0.0f;
  CHECK(coolant.read(counting, value) == ReadStatus::Unavailable);
  CHECK(counting.asked[obd::kPidCoolantTemp] == 0);

  PidChannel intake = PidChannel::intakeAir();  // this one the car does report
  intake.onLinked(support);
  CHECK(intake.read(counting, value) == ReadStatus::Ok);
}

TEST_CASE("pid channel: when the car did not answer the discovery it is asked anyway") {
  Car car;
  car.ecu.setSupported(obd::kPidCoolantTemp, false);  // really absent, but we do not know that
  CountingObdClient counting(car.client);

  PidChannel coolant = PidChannel::coolant();
  coolant.onLinked(PidSupport{});  // nothing known
  float value = 0.0f;
  CHECK(coolant.read(counting, value) == ReadStatus::Failed);  // it tried, and the car said no
  CHECK(counting.asked[obd::kPidCoolantTemp] == 1);
}

TEST_CASE("pid channel: no answer, or too few bytes, is a failure") {
  FixedClient client;
  float value = 0.0f;
  PidChannel voltage = PidChannel::voltage();  // needs two bytes

  client.reply = bytes(0x31, 0x58, 1);  // only one
  CHECK(voltage.read(client, value) == ReadStatus::Failed);

  client.reply = bytes(0x31, 0x58, 2);
  CHECK(voltage.read(client, value) == ReadStatus::Ok);

  client.ok = false;
  CHECK(voltage.read(client, value) == ReadStatus::Failed);
}
