// BoostSampler end to end against fake and stub clients.

#include "test_support.h"

using namespace support;

TEST_CASE("sampler computes boost from MAP and baro") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setBaroKpa(100.0f);
  ecu.holdMapKpa(169.0f);  // +69 kPa ~ 10 psi
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  BoostSampler sampler(client, clock);

  const BoostReading r = sampler.sample();
  REQUIRE(r.valid);
  CHECK(r.psi == doctest::Approx(10.0f).epsilon(0.01));

  ecu.holdMapKpa(60.0f);
  const BoostReading vac = sampler.sample();
  REQUIRE(vac.valid);
  CHECK(vac.psi < 0.0f);
}

TEST_CASE("sampler: link loss and recovery") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(130.0f);
  FakeElm327Stream stream(ecu);
  Elm327Client client(stream, clock);
  BoostSampler::Options options;
  options.link.failuresBeforeReconnect = 2;
  options.link.reconnectIntervalMs = 1000;
  BoostSampler sampler(client, clock, options);

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
  BoostSampler sampler(client, clock);

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
  BoostSampler::Options options;
  options.link.failuresBeforeReconnect = 2;
  BoostSampler sampler(client, clock, options);

  CHECK_FALSE(sampler.sample().valid);
  CHECK_FALSE(sampler.sample().valid);
  clock.advance(2000);
  CHECK_FALSE(sampler.sample().valid);
  CHECK(client.beginCalls == 2);
}

TEST_CASE("sampler: a success resets the failure streak") {
  ManualClock clock;
  StubObdClient client;
  BoostSampler::Options options;
  options.link.failuresBeforeReconnect = 3;
  BoostSampler sampler(client, clock, options);

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
  BoostSampler::Options options;
  options.ambient.refreshMs = 1000;
  options.ambient.retryMs = 400;
  BoostSampler sampler(client, clock, options);
  const float assumed = boostPsi(101.0f, kStdAtmosphereKpa);

  // No baro PID: standard atmosphere (101.325) is assumed.
  client.baro = {false, 0, 0};
  CHECK(sampler.sample().psi == doctest::Approx(assumed));

  // Baro with an empty payload is treated as unavailable (retried after 400 ms).
  client.baro = {true, 0, 0};
  clock.advance(400);
  CHECK(sampler.sample().psi == doctest::Approx(assumed));

  // While unavailable, retries are throttled: a now-working PID is not read yet...
  client.baro = {true, 1, 95};
  clock.advance(399);
  CHECK(sampler.sample().psi == doctest::Approx(assumed));
  // ...but is picked up once the retry interval has elapsed.
  clock.advance(1);
  CHECK(sampler.sample().psi == doctest::Approx(boostPsi(101.0f, 95.0f)));

  // Once known, it is only re-read every baroRefreshMs.
  client.baro = {true, 1, 90};
  clock.advance(999);
  CHECK(sampler.sample().psi == doctest::Approx(boostPsi(101.0f, 95.0f)));
  clock.advance(1);
  CHECK(sampler.sample().psi == doctest::Approx(boostPsi(101.0f, 90.0f)));

  // A later baro failure keeps the last good value.
  client.baro = {false, 0, 0};
  clock.advance(1000);
  CHECK(sampler.sample().psi == doctest::Approx(boostPsi(101.0f, 90.0f)));
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
  BoostSampler sampler(client, clock);

  for (int i = 0; i < 50; ++i) {
    clock.advance(33);
    sampler.sample();
  }
  CHECK(client.baroReads == 1);
}
