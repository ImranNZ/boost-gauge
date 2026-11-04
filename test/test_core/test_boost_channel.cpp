// BoostRig end to end against fake and stub clients.

#include "test_support.h"

using namespace support;

TEST_CASE("sampler computes boost from MAP and baro") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setBaroKpa(100.0f);
  ecu.holdMapKpa(169.0f);  // +69 kPa ~ 10 psi
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  BoostRig sampler(client, clock);

  const Reading r = sampler.sample();
  REQUIRE(r.valid);
  CHECK(r.value == doctest::Approx(10.0f).epsilon(0.01));

  ecu.holdMapKpa(60.0f);
  const Reading vac = sampler.sample();
  REQUIRE(vac.valid);
  CHECK(vac.value < 0.0f);
}

TEST_CASE("sampler: link loss and recovery") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(130.0f);
  FakeElm327Stream stream(ecu);
  Elm327Client client(stream, clock);
  BoostRig::Options options;
  options.link.failuresBeforeReconnect = 2;
  options.link.reconnectIntervalMs = 1000;
  BoostRig sampler(client, clock, options);

  REQUIRE(sampler.sample().valid);

  ecu.setOnline(false);
  CHECK_FALSE(sampler.sample().valid);
  CHECK_FALSE(sampler.sample().valid);  // now marked unlinked

  ecu.setOnline(true);
  clock.advance(1500);
  CHECK(sampler.sample().valid);  // reconnects and reads again
}

TEST_CASE("sampler: connect failure is retried only after the interval") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  BoostRig sampler(client, clock);

  CHECK_FALSE(sampler.sample().valid);
  CHECK(client.beginCalls == 1);
  CHECK_FALSE(sampler.sample().valid);
  CHECK(client.beginCalls == 1);  // throttled

  clock.advance(2000);
  client.beginResult = true;
  client.map = {true, 1, 101};
  CHECK(sampler.sample().valid);
  CHECK(client.beginCalls == 2);
}

TEST_CASE("sampler: empty MAP payload counts as failure and triggers reconnect") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 0, 0};  // success flag but no data bytes
  BoostRig::Options options;
  options.link.failuresBeforeReconnect = 2;
  BoostRig sampler(client, clock, options);

  CHECK_FALSE(sampler.sample().valid);
  CHECK_FALSE(sampler.sample().valid);
  clock.advance(2000);
  CHECK_FALSE(sampler.sample().valid);
  CHECK(client.beginCalls == 2);
}

TEST_CASE("sampler: a success resets the failure streak") {
  ManualClock clock;
  StubObdClient client;
  BoostRig::Options options;
  options.link.failuresBeforeReconnect = 3;
  BoostRig sampler(client, clock, options);

  client.map = {false, 0, 0};
  sampler.sample();
  sampler.sample();
  client.map = {true, 1, 120};
  CHECK(sampler.sample().valid);
  client.map = {false, 0, 0};
  sampler.sample();
  sampler.sample();
  CHECK(client.beginCalls == 1);  // never relinked
}

TEST_CASE("sampler: baro fallback, update, refresh and failure retention") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 101};
  BoostRig::Options options;
  options.ambient.refreshMs = 1000;
  options.ambient.retryMs = 400;
  BoostRig sampler(client, clock, options);
  const float assumed = boostPsi(101.0f, kStdAtmosphereKpa);

  // No baro PID: standard atmosphere (101.325) is assumed.
  client.baro = {false, 0, 0};
  CHECK(sampler.sample().value == doctest::Approx(assumed));

  // Baro with an empty payload is treated as unavailable (retried after 400 ms).
  client.baro = {true, 0, 0};
  clock.advance(400);
  CHECK(sampler.sample().value == doctest::Approx(assumed));

  // While unavailable, retries are throttled: a now-working PID is not read yet...
  client.baro = {true, 1, 95};
  clock.advance(399);
  CHECK(sampler.sample().value == doctest::Approx(assumed));
  // ...but is picked up once the retry interval has elapsed.
  clock.advance(1);
  CHECK(sampler.sample().value == doctest::Approx(boostPsi(101.0f, 95.0f)));

  // Once known, it is only re-read every baroRefreshMs.
  client.baro = {true, 1, 90};
  clock.advance(999);
  CHECK(sampler.sample().value == doctest::Approx(boostPsi(101.0f, 95.0f)));
  clock.advance(1);
  CHECK(sampler.sample().value == doctest::Approx(boostPsi(101.0f, 90.0f)));

  // A later baro failure keeps the last good value.
  client.baro = {false, 0, 0};
  clock.advance(1000);
  CHECK(sampler.sample().value == doctest::Approx(boostPsi(101.0f, 90.0f)));
}

TEST_CASE("sampler: unsupported baro PID is not queried on every sample") {
  ManualClock clock;
  struct CountingClient : StubObdClient {
    int baroReads = 0;
    bool readPid(uint8_t mode, uint8_t pid, ObdData& out) override {
      if (pid == obd::kPidBaroKpa) ++baroReads;
      return StubObdClient::readPid(mode, pid, out);
    }
  } client;
  client.map = {true, 1, 120};
  BoostRig sampler(client, clock);

  for (int i = 0; i < 50; ++i) {
    clock.advance(33);
    sampler.sample();
  }
  CHECK(client.baroReads == 1);
}

namespace {

/// Wraps a client and counts how often each PID was asked for.
class CountingClient : public IObdClient {
 public:
  explicit CountingClient(IObdClient& inner) : inner_(inner) {}
  bool begin() override {
    ++begins;
    return inner_.begin();
  }
  bool readPid(uint8_t mode, uint8_t pid, ObdData& out) override {
    ++asked[pid];
    return inner_.readPid(mode, pid, out);
  }

  int begins = 0;
  std::array<int, 256> asked{};

 private:
  IObdClient& inner_;
};

int discoveries = 0;
bool sawBaro = false;

}  // namespace

TEST_CASE("sampler: a car that does not report barometric pressure is never asked for it") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setSupported(obd::kPidBaroKpa, false);
  ecu.holdMapKpa(150.0f);
  FakeCanBus bus(ecu);
  CanObdClient inner(bus, clock);
  CountingClient client(inner);
  BoostRig sampler(client, clock);

  for (int i = 0; i < 20; ++i) {
    clock.advance(20000);  // long enough that a baro read would be due every time
    const Reading r = sampler.sample();
    REQUIRE(r.valid);
    CHECK(r.value == doctest::Approx(boostPsi(150.0f, kStdAtmosphereKpa)));
  }
  CHECK(client.asked[obd::kPidBaroKpa] == 0);
  CHECK(sampler.support().knows(obd::kPidBaroKpa));
  CHECK_FALSE(sampler.support().supports(obd::kPidBaroKpa));
}

TEST_CASE("sampler: a car with no manifold pressure shows no boost and is left alone") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setSupported(obd::kPidMapKpa, false);
  FakeCanBus bus(ecu);
  CanObdClient inner(bus, clock);
  CountingClient client(inner);
  BoostRig sampler(client, clock);

  for (int i = 0; i < 30; ++i) {
    clock.advance(33);
    CHECK_FALSE(sampler.sample().valid);
  }
  CHECK(client.asked[obd::kPidMapKpa] == 0);  // not asked, so no failures and no reconnect storm
  CHECK(client.begins == 1);
}

TEST_CASE("sampler: when discovery fails it assumes nothing and carries on as before") {
  ManualClock clock;
  StubObdClient client;  // answers PID 00 with one byte: not a bitmap
  client.map = {true, 1, 150};
  client.baro = {true, 1, 100};
  BoostRig sampler(client, clock);

  const Reading r = sampler.sample();
  REQUIRE(r.valid);
  CHECK(r.value == doctest::Approx(boostPsi(150.0f, 100.0f)));
  CHECK_FALSE(sampler.support().knows(obd::kPidMapKpa));
}

TEST_CASE("sampler: the car is rediscovered after the link comes back") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(130.0f);
  FakeCanBus bus(ecu);
  CanObdClient inner(bus, clock);
  CountingClient client(inner);
  BoostRig::Options options;
  options.link.failuresBeforeReconnect = 2;
  options.link.reconnectIntervalMs = 1000;
  BoostRig sampler(client, clock, options);

  REQUIRE(sampler.sample().valid);
  CHECK(client.asked[obd::kPidSupported01To20] == 1);

  ecu.setOnline(false);
  CHECK_FALSE(sampler.sample().valid);
  CHECK_FALSE(sampler.sample().valid);
  ecu.setOnline(true);
  clock.advance(1500);
  REQUIRE(sampler.sample().valid);
  CHECK(client.asked[obd::kPidSupported01To20] == 2);  // asked again on the new connection
}

TEST_CASE("sampler: reports what the car supports to a listener") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setSupported(obd::kPidBaroKpa, false);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  BoostRig::Options options;
  options.onDiscovered = [](const PidSupport& support) {
    ++discoveries;
    sawBaro = support.supports(obd::kPidBaroKpa);
  };
  BoostRig sampler(client, clock, options);
  discoveries = 0;
  sawBaro = true;

  sampler.sample();
  sampler.sample();
  CHECK(discoveries == 1);  // once per connection, not per sample
  CHECK_FALSE(sawBaro);
}
