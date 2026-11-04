// AmbientPressure: cached barometric pressure with throttled retries.

#include "test_support.h"

using namespace support;

namespace {

/// A stub that also counts how often the barometric PID is asked for.
struct CountingClient : StubObdClient {
  int baroReads = 0;
  bool readPid(uint8_t mode, uint8_t pid, ObdData& out) override {
    if (pid == obd::kPidBaroKpa) ++baroReads;
    return StubObdClient::readPid(mode, pid, out);
  }
};

}  // namespace

TEST_CASE("ambient pressure: assumes standard atmosphere when the car does not answer") {
  ManualClock clock;
  CountingClient client;
  AmbientPressure ambient(client, clock);
  CHECK(ambient.kpa() == doctest::Approx(kStdAtmosphereKpa));
}

TEST_CASE("ambient pressure: reads the car and caches it until the refresh is due") {
  ManualClock clock;
  CountingClient client;
  client.baro = {true, 1, 95};
  AmbientPressure::Options options;
  options.refreshMs = 1000;
  AmbientPressure ambient(client, clock, options);

  CHECK(ambient.kpa() == doctest::Approx(95.0f));
  CHECK(client.baroReads == 1);

  client.baro = {true, 1, 90};
  clock.advance(999);
  CHECK(ambient.kpa() == doctest::Approx(95.0f));  // still cached
  CHECK(client.baroReads == 1);

  clock.advance(1);
  CHECK(ambient.kpa() == doctest::Approx(90.0f));  // refreshed
  CHECK(client.baroReads == 2);
}

TEST_CASE("ambient pressure: an unsupported PID is retried on the slow timer, not every call") {
  ManualClock clock;
  CountingClient client;  // baro never answers
  AmbientPressure::Options options;
  options.retryMs = 400;
  AmbientPressure ambient(client, clock, options);

  for (int i = 0; i < 50; ++i) {
    clock.advance(5);  // 250 ms in total: inside the retry window
    ambient.kpa();
  }
  CHECK(client.baroReads == 1);

  client.baro = {true, 1, 99};
  clock.advance(400);
  CHECK(ambient.kpa() == doctest::Approx(99.0f));  // picked up on retry
}

TEST_CASE("ambient pressure: an empty payload is treated as unavailable") {
  ManualClock clock;
  CountingClient client;
  client.baro = {true, 0, 0};  // success flag but no data bytes
  AmbientPressure ambient(client, clock);
  CHECK(ambient.kpa() == doctest::Approx(kStdAtmosphereKpa));
}

TEST_CASE("ambient pressure: a later failure keeps the last good value") {
  ManualClock clock;
  CountingClient client;
  client.baro = {true, 1, 92};
  AmbientPressure::Options options;
  options.refreshMs = 100;
  AmbientPressure ambient(client, clock, options);
  REQUIRE(ambient.kpa() == doctest::Approx(92.0f));

  client.baro = {false, 0, 0};
  clock.advance(100);
  CHECK(ambient.kpa() == doctest::Approx(92.0f));
}

TEST_CASE("ambient pressure: invalidate forces a fresh read at the next call") {
  ManualClock clock;
  CountingClient client;
  client.baro = {true, 1, 100};
  AmbientPressure ambient(client, clock);
  REQUIRE(ambient.kpa() == doctest::Approx(100.0f));

  client.baro = {true, 1, 80};
  CHECK(ambient.kpa() == doctest::Approx(100.0f));  // cached
  ambient.invalidate();
  CHECK(ambient.kpa() == doctest::Approx(80.0f));
}

TEST_CASE("ambient pressure: a car known to lack the PID is never asked") {
  ManualClock clock;
  StubObdClient client;
  client.baro = {true, 1, 90};
  AmbientPressure ambient(client, clock);

  ambient.setAvailable(false);
  CHECK(ambient.kpa() == doctest::Approx(kStdAtmosphereKpa));
  clock.advance(120000);
  CHECK(ambient.kpa() == doctest::Approx(kStdAtmosphereKpa));  // not read even though a read is due

  ambient.setAvailable(true);
  CHECK(ambient.kpa() == doctest::Approx(90.0f));  // available again: it reads
}

TEST_CASE("ambient pressure: becoming unavailable forgets what was read") {
  ManualClock clock;
  StubObdClient client;
  client.baro = {true, 1, 90};
  AmbientPressure ambient(client, clock);
  REQUIRE(ambient.kpa() == doctest::Approx(90.0f));

  ambient.setAvailable(false);
  CHECK(ambient.kpa() == doctest::Approx(kStdAtmosphereKpa));
}
