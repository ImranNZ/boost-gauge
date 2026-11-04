// ELM327 reply parsing, the ELM327 client and the fake adapter.

#include "test_support.h"

using namespace support;

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
  CHECK_FALSE(parseObdReply("41 0B", 0x01, 0x0B, d));        // header, no data
}

TEST_CASE("elm parse: lowercase hex and odd/short/over-long lines") {
  ObdData d;
  CHECK(parseObdReply("41 0b 7a", 0x01, 0x0B, d));
  CHECK(d.bytes[0] == 0x7A);

  CHECK_FALSE(parseObdReply("41 0B 7", 0x01, 0x0B, d));     // too short
  CHECK_FALSE(parseObdReply("41 0B 7A 1", 0x01, 0x0B, d));  // odd nibble count
  CHECK_FALSE(parseObdReply("4", 0x01, 0x0B, d));
  CHECK_FALSE(parseObdReply("41 0G 7A", 0x01, 0x0B, d));    // not hex (upper)
  CHECK_FALSE(parseObdReply("41 0g 7A", 0x01, 0x0B, d));    // not hex (lower)
  CHECK_FALSE(parseObdReply("\t41 0B 7A", 0x01, 0x0B, d));  // control char
  CHECK_FALSE(parseObdReply("42 0B 7A", 0x01, 0x0B, d));    // wrong mode
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
  CHECK(send("hello") == "hello\r?\r\r>");   // echo + unknown command
  CHECK(send("AT E0") == "AT E0\rOK\r\r>");  // last echoed command
  CHECK(send("hello") == "?\r\r>");          // echo now off
  CHECK(send("ATZ").find("ELM327") != std::string::npos);
  CHECK(send("0100").find("SEARCHING") != std::string::npos);
  CHECK(send("0100").find("SEARCHING") == std::string::npos);
  CHECK(send("0A0A").find("NO DATA") != std::string::npos);  // unsupported PID
  const uint8_t lf = '\n';
  CHECK(stream.write(&lf, 1) == 1);  // bare LF is ignored
}
