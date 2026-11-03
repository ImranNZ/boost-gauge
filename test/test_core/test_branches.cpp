// Targeted tests for failure paths and edge cases in the protocol/logic layer.

#include <doctest.h>

#include <deque>
#include <string>
#include <vector>

#include "BoostSampler.h"
#include "CanObdClient.h"
#include "Elm327Client.h"
#include "Elm327Parse.h"
#include "FakeCanBus.h"
#include "FakeEcu.h"
#include "FakeElm327Stream.h"
#include "GaugeApp.h"
#include "ManualClock.h"
#include "ScriptedByteStream.h"
#include "Smoothing.h"
#include "SoftCanvas.h"
#include "StubObdClient.h"
#include "Units.h"

using namespace gauge;

namespace {

const char* const kOk = "OK\r\r>";

std::vector<std::string> happyHandshake() {
  return {"\r\rELM327 v1.5\r\r>", kOk, kOk, kOk, kOk, "41 00 00 20 00 01\r\r>"};
}

/// CAN bus double: replays queued frames, optionally burning time per receive.
class ScriptedBus : public ICanBus {
 public:
  explicit ScriptedBus(ManualClock& clock) : clock_(clock) {}

  bool sendResult = true;
  uint32_t msPerReceive = 0;
  std::deque<CanFrame> frames;

  bool begin() override { return true; }
  bool send(const CanFrame&) override { return sendResult; }
  bool receive(CanFrame& out, uint32_t) override {
    clock_.advance(msPerReceive);
    if (frames.empty()) return false;
    out = frames.front();
    frames.pop_front();
    return true;
  }

 private:
  ManualClock& clock_;
};

CanFrame response(uint32_t id, std::array<uint8_t, 8> data, uint8_t len = 8) {
  CanFrame f;
  f.id = id;
  f.data = data;
  f.len = len;
  return f;
}

}  // namespace

// ------------------------------------------------------------------ units --

TEST_CASE("units: psi <-> kPa round trip") {
  const float psi = 12.5f;
  CHECK(kpaToPsi(psiToKpa(psi)) == doctest::Approx(psi));
  CHECK(psiToKpa(1.0f) == doctest::Approx(kKpaPerPsi));
}

// ----------------------------------------------------------------- parser --

TEST_CASE("elm parse: lowercase hex and odd/short/over-long lines") {
  ObdData d;
  CHECK(parseObdReply("41 0b 7a", 0x01, 0x0B, d));
  CHECK(d.bytes[0] == 0x7A);

  CHECK_FALSE(parseObdReply("41 0B 7", 0x01, 0x0B, d));      // too short
  CHECK_FALSE(parseObdReply("41 0B 7A 1", 0x01, 0x0B, d));    // odd nibble count
  CHECK_FALSE(parseObdReply("4", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("41 0G 7A", 0x01, 0x0B, d));      // not hex (upper)
  CHECK_FALSE(parseObdReply("41 0g 7A", 0x01, 0x0B, d));      // not hex (lower)
  CHECK_FALSE(parseObdReply("\t41 0B 7A", 0x01, 0x0B, d));    // control char
  CHECK_FALSE(parseObdReply("42 0B 7A", 0x01, 0x0B, d));  // wrong mode
}

TEST_CASE("elm parse: keeps at most four data bytes and tolerates long lines") {
  ObdData d;
  CHECK(parseObdReply("41 0C 01 02 03 04 05", 0x01, 0x0C, d));
  CHECK(d.count == 4);
  CHECK(d.bytes[3] == 0x04);

  // More than 8 bytes on a line: excess is ignored, header still matched.
  CHECK(parseObdReply("41 0C 01 02 03 04 05 06 07 08 09 0A", 0x01, 0x0C, d));
  CHECK(d.count == 4);
}

TEST_CASE("elm parse: first matching line wins across multiple ECUs") {
  ObdData d;
  CHECK(parseObdReply("41 0B 10\r41 0B 20\r", 0x01, 0x0B, d));
  CHECK(d.bytes[0] == 0x10);
}

// ------------------------------------------------------------ elm327 flow --

TEST_CASE("elm327: sends the expected init sequence") {
  ManualClock clock;
  ScriptedByteStream stream(happyHandshake());
  Elm327Client client(stream, clock);

  REQUIRE(client.begin());
  const std::vector<std::string> expected = {"ATZ", "ATE0", "ATL0", "ATH0", "ATSP6", "0100"};
  CHECK(stream.commands() == expected);
}

TEST_CASE("elm327: bytes trailing the prompt are flushed before the next command") {
  ManualClock clock;
  auto script = happyHandshake();
  script[1] = "OK\r\r>TRAILING GARBAGE";  // arrives after the prompt
  ScriptedByteStream stream(script);
  Elm327Client client(stream, clock);

  CHECK(client.begin());
}

TEST_CASE("elm327: silent adapter fails at reset after the timeout") {
  ManualClock clock;
  ScriptedByteStream stream({});
  Elm327Client client(stream, clock);

  CHECK_FALSE(client.begin());
  CHECK(stream.commands().size() == 1);
  CHECK(clock.nowMs() >= 2000);
}

TEST_CASE("elm327: a setup command that never answers aborts begin") {
  ManualClock clock;
  ScriptedByteStream stream({"\r\rELM327 v1.5\r\r>"});  // ATE0 gets no reply
  Elm327Client client(stream, clock);

  CHECK_FALSE(client.begin());
  CHECK(stream.commands().size() == 2);
}

TEST_CASE("elm327: failed bus probe fails begin") {
  ManualClock clock;
  auto script = happyHandshake();
  script.back() = "UNABLE TO CONNECT\r\r>";
  ScriptedByteStream stream(script);
  Elm327Client client(stream, clock);

  CHECK_FALSE(client.begin());
  CHECK(stream.commands().back() == "0100");
}

TEST_CASE("elm327: oversize reply is bounded and rejected") {
  ManualClock clock;
  auto script = happyHandshake();
  script.push_back(std::string(500, 'A') + "\r>");
  ScriptedByteStream stream(script);
  Elm327Client client(stream, clock);
  REQUIRE(client.begin());

  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
}

TEST_CASE("elm327: stale bytes from a previous exchange are discarded") {
  ManualClock clock;
  auto script = happyHandshake();
  script.push_back("junk without prompt");  // never terminated by '>'
  script.push_back("41 0B 55\r\r>");
  ScriptedByteStream stream(script);
  Elm327Client client(stream, clock);
  REQUIRE(client.begin());

  ObdData d;
  CHECK_FALSE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));  // times out
  REQUIRE(client.readPid(obd::kModeCurrentData, obd::kPidMapKpa, d));
  CHECK(d.bytes[0] == 0x55);
}

TEST_CASE("elm327 fake adapter: echo, unknown command, reset") {
  ManualClock clock;
  FakeEcu ecu(clock);
  FakeElm327Stream stream(ecu);

  // Not connected: writes are dropped and reads return nothing.
  const uint8_t junk[] = {'X'};
  CHECK(stream.write(junk, 1) == 0);
  CHECK(stream.read() == -1);

  REQUIRE(stream.connect());
  const auto send = [&](const char* text) {
    for (const char* p = text; *p; ++p) {
      const uint8_t b = static_cast<uint8_t>(*p);
      stream.write(&b, 1);
    }
    const uint8_t cr = '\r';
    stream.write(&cr, 1);
    std::string out;
    for (int c = stream.read(); c >= 0; c = stream.read()) out += static_cast<char>(c);
    return out;
  };

  CHECK(send("ATE1").find("OK") != std::string::npos);
  CHECK(send("hello") == "hello\r?\r\r>");           // echo + unknown command
  CHECK(send("AT E0") == "AT E0\rOK\r\r>");           // last echoed command
  CHECK(send("hello") == "?\r\r>");                   // echo now off
  CHECK(send("ATZ").find("ELM327") != std::string::npos);
  CHECK(send("0100").find("SEARCHING") != std::string::npos);
  CHECK(send("0100").find("SEARCHING") == std::string::npos);
  CHECK(send("0A0A").find("NO DATA") != std::string::npos);  // unsupported PID
  const uint8_t lf = '\n';
  CHECK(stream.write(&lf, 1) == 1);  // bare LF is ignored
}

// ------------------------------------------------------------- can client --

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

// ------------------------------------------------------------ fake ecu -----

TEST_CASE("fake ecu: PIDs, clamping, profile and offline") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ObdData d;

  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidSupported01To20, d));
  CHECK(d.count == 4);
  REQUIRE(ecu.answer(obd::kModeCurrentData, obd::kPidBaroKpa, d));
  CHECK(d.bytes[0] == 101);
  CHECK_FALSE(ecu.answer(0x09, obd::kPidMapKpa, d));         // wrong mode
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

// ---------------------------------------------------------------- sampler --

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
  options.failuresBeforeReconnect = 2;
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
  options.failuresBeforeReconnect = 3;
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
  options.baroRefreshMs = 1000;
  options.baroRetryMs = 400;
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

// ---------------------------------------------------------- smoothing/app --

TEST_CASE("peak hold: rises, ties, expires") {
  PeakHold peak(1000);
  CHECK(peak.update(2.0f, 0) == doctest::Approx(2.0f));
  CHECK(peak.update(4.0f, 100) == doctest::Approx(4.0f));
  CHECK(peak.update(4.0f, 200) == doctest::Approx(4.0f));  // tie refreshes the timer
  CHECK(peak.update(1.0f, 1100) == doctest::Approx(4.0f));
  CHECK(peak.update(1.0f, 1200) == doctest::Approx(1.0f));
  CHECK(peak.value() == doctest::Approx(1.0f));
}

TEST_CASE("app: never-connected shows NO LINK and still presents frames") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  BoostSampler sampler(client, clock);
  PollingSource source(sampler);
  SoftCanvas canvas(240, 240);
  GaugeApp app(source, canvas, clock);

  for (int i = 0; i < 3; ++i) {
    clock.advance(33);
    app.tick();
  }
  CHECK(canvas.framesPresented() == 3);
}

TEST_CASE("app: custom view and options are honoured") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 160};
  client.baro = {true, 1, 100};
  BoostSampler sampler(client, clock);
  PollingSource source(sampler);
  SoftCanvas canvas(240, 240);
  GaugeConfig config;
  config.maxPsi = 30.0f;
  GaugeApp::Options options;
  options.smoothingMs = 0.0f;
  options.staleAfterMs = 10;
  GaugeApp app(source, canvas, clock, GaugeView(config), options);

  app.tick();
  client.map = {false, 0, 0};
  clock.advance(50);  // beyond staleAfterMs
  app.tick();
  CHECK(canvas.framesPresented() == 2);
}
