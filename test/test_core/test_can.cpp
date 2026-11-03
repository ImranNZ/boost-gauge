// The CAN OBD client and the fake CAN bus.

#include "test_support.h"

using namespace support;

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

TEST_CASE("can client: send failure") {
  ManualClock clock;
  ScriptedBus bus(clock);
  bus.sendResult = false;
  CanObdClient client(bus, clock);
  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
}

TEST_CASE("can client: reply must arrive within the timeout despite chatter") {
  ManualClock clock;
  ScriptedBus bus(clock);
  bus.msPerReceive = 60;
  for (int i = 0; i < 5; ++i) bus.frames.push_back(response(0x100, {}, 8));
  CanObdClient client(bus, clock);
  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(bus.frames.size() >= 2);  // gave up before draining the bus
}

TEST_CASE("can client: skips malformed and mismatched frames") {
  ManualClock clock;
  ScriptedBus bus(clock);
  bus.frames = {
      response(0x7E7, {0x03, 0x41, 0x0B, 0x01}),          // id below range
      response(0x7F0, {0x03, 0x41, 0x0B, 0x02}),          // id above range
      response(0x7E8, {0x03, 0x41, 0x0B, 0x03}, 2),       // too short
      response(0x7E8, {0x02, 0x41, 0x0B, 0x04}),          // PCI too small
      response(0x7E8, {0x08, 0x41, 0x0B, 0x05}),          // PCI too large
      response(0x7E8, {0x03, 0x42, 0x0B, 0x06}),          // wrong mode
      response(0x7E8, {0x03, 0x41, 0x0C, 0x07}),          // wrong PID
      response(0x7EF, {0x03, 0x41, 0x0B, 0x99}),          // the one
  };
  CanObdClient client(bus, clock);
  ObdData d;
  REQUIRE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.count == 1);
  CHECK(d.bytes[0] == 0x99);
}

TEST_CASE("can client: data length limited by PCI, frame length and buffer") {
  ManualClock clock;
  ScriptedBus bus(clock);
  CanObdClient client(bus, clock);
  ObdData d;

  bus.frames = {response(0x7E8, {0x07, 0x41, 0x00, 1, 2, 3, 4, 5})};  // 5 data bytes
  REQUIRE(client.readPid(obd::kModeCurrentData, 0x00, d));
  CHECK(d.count == 4);

  bus.frames = {response(0x7E8, {0x05, 0x41, 0x00, 9, 0, 0, 0, 0}, 4)};  // frame cut short
  REQUIRE(client.readPid(obd::kModeCurrentData, 0x00, d));
  CHECK(d.count == 1);
  CHECK(d.bytes[0] == 9);
}

TEST_CASE("fake CAN bus: not started, malformed and unsupported requests") {
  ManualClock clock;
  FakeEcu ecu(clock);
  FakeCanBus bus(ecu);

  CanFrame req;
  req.id = 0x7DF;
  req.len = 8;
  req.data = {0x02, 0x01, 0x0B, 0, 0, 0, 0, 0};
  CHECK_FALSE(bus.send(req));  // begin() not called

  REQUIRE(bus.begin());
  CanFrame out;

  CanFrame wrongId = req;
  wrongId.id = 0x123;
  CHECK(bus.send(wrongId));
  CHECK_FALSE(bus.receive(out, 10));

  CanFrame tooShort = req;
  tooShort.len = 2;
  CHECK(bus.send(tooShort));
  CHECK_FALSE(bus.receive(out, 10));

  CanFrame multiFrame = req;
  multiFrame.data[0] = 0x10;
  CHECK(bus.send(multiFrame));
  CHECK_FALSE(bus.receive(out, 10));

  CanFrame unsupported = req;
  unsupported.data[2] = 0xEE;
  CHECK(bus.send(unsupported));
  CHECK_FALSE(bus.receive(out, 10));

  CHECK(bus.send(req));
  CHECK(bus.receive(out, 10));
  CHECK(out.id == 0x7E8);
}
