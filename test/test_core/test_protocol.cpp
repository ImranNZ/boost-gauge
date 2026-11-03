#include <doctest.h>

#include "BoostSampler.h"
#include "CanObdClient.h"
#include "Elm327Client.h"
#include "Elm327Parse.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "FakeElm327Stream.h"
#include "ManualClock.h"
#include "SpscRing.h"
#include "Units.h"

using namespace gauge;

TEST_CASE("units: kPa <-> psi and boost") {
  CHECK(kpaToPsi(100.0f) == doctest::Approx(14.5038f).epsilon(0.001));
  CHECK(boostPsi(101.0f, 101.0f) == doctest::Approx(0.0f));
  CHECK(boostPsi(170.0f, 101.0f) == doctest::Approx(10.0f).epsilon(0.01));
  CHECK(boostPsi(30.0f, 101.0f) < 0.0f);  // vacuum
}

TEST_CASE("elm parse: plain, spaced, echo and noise") {
  ObdData d;
  CHECK(parseObdReply("41 0B 65\r\r", 0x01, 0x0B, d));
  CHECK(d.count == 1);
  CHECK(d.bytes[0] == 0x65);

  CHECK(parseObdReply("410B65", 0x01, 0x0B, d));
  CHECK(d.bytes[0] == 0x65);

  CHECK(parseObdReply("010B\rSEARCHING...\r41 0B 7A\r", 0x01, 0x0B, d));
  CHECK(d.bytes[0] == 0x7A);

  CHECK(parseObdReply("41 00 00 20 00 01", 0x01, 0x00, d));
  CHECK(d.count == 4);
  CHECK(d.bytes[1] == 0x20);
}

TEST_CASE("elm parse: rejects failures and other PIDs") {
  ObdData d;
  CHECK_FALSE(parseObdReply("NO DATA\r", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("UNABLE TO CONNECT\r", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("?\r", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("41 0C 1A F8", 0x01, 0x0B, d));  // RPM, not MAP
  CHECK_FALSE(parseObdReply("41 0B", 0x01, 0x0B, d));  // header, no data
}

TEST_CASE("elm327 client against fake adapter") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(150.0f);
  FakeElm327Stream stream(ecu);
  Elm327Client client(stream, clock);

  REQUIRE(client.begin());

  ObdData d;
  REQUIRE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.bytes[0] == 150);

  CHECK_FALSE(client.readPid(obd::kModeCurrentData, 0x0C, d));  // NO DATA
}

TEST_CASE("elm327 client: connection refused / silent adapter") {
  ManualClock clock;
  FakeEcu ecu(clock);
  FakeElm327Stream stream(ecu);

  stream.setRefuseConnection(true);
  Elm327Client client(stream, clock);
  CHECK_FALSE(client.begin());

  stream.setRefuseConnection(false);
  REQUIRE(client.begin());
  stream.dropLink();
  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
}

TEST_CASE("elm327 client: ECU offline times out cleanly") {
  ManualClock clock;
  FakeEcu ecu(clock);
  FakeElm327Stream stream(ecu);
  Elm327Client client(stream, clock);
  ecu.setOnline(false);
  CHECK_FALSE(client.begin());  // probe query gets NO DATA
}

TEST_CASE("can client reads MAP and ignores noise") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.holdMapKpa(120.0f);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  REQUIRE(client.begin());

  CanFrame noise;
  noise.id = 0x1A0;
  noise.len = 8;
  bus.injectNoise(noise);
  CanFrame wrongPid;
  wrongPid.id = 0x7E8;
  wrongPid.len = 8;
  wrongPid.data = {0x03, 0x41, 0x0C, 0x11, 0, 0, 0, 0};
  bus.injectNoise(wrongPid);

  ObdData d;
  REQUIRE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.bytes[0] == 120);

  REQUIRE(bus.sent().size() == 1);
  CHECK(bus.sent()[0].id == 0x7DF);
  CHECK(bus.sent()[0].data[0] == 0x02);
}

TEST_CASE("can client: no response -> false") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setOnline(false);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  REQUIRE(client.begin());
  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
}

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
  options.failuresBeforeReconnect = 2;
  options.reconnectIntervalMs = 1000;
  BoostSampler sampler(client, clock, options);

  REQUIRE(sampler.sample().valid);

  ecu.setOnline(false);
  CHECK_FALSE(sampler.sample().valid);
  CHECK_FALSE(sampler.sample().valid);  // now marked unlinked

  ecu.setOnline(true);
  clock.advance(1500);
  CHECK(sampler.sample().valid);  // reconnects and reads again
}

TEST_CASE("spsc ring is FIFO and bounded") {
  SpscRing<4> ring;  // holds 3
  CHECK(ring.pop() == -1);
  CHECK(ring.push(1));
  CHECK(ring.push(2));
  CHECK(ring.push(3));
  CHECK_FALSE(ring.push(4));
  CHECK(ring.pop() == 1);
  CHECK(ring.push(4));
  CHECK(ring.pop() == 2);
  CHECK(ring.pop() == 3);
  CHECK(ring.pop() == 4);
  CHECK(ring.pop() == -1);
}
