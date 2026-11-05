// Setup: repairing impossible combinations, its profile, and its one-word encoding.

#include "test_support.h"

using namespace support;

TEST_CASE("setup: a possible combination is left alone") {
  const Setup setup{LookId::Minimal, ChannelKind::Coolant, Unit::Fahrenheit};
  CHECK(normalised(setup) == setup);
  CHECK_FALSE(normalised(setup) != setup);
}

TEST_CASE("setup: a boost-dial look with another channel becomes Classic") {
  CHECK(normalised(Setup{LookId::Retro, ChannelKind::Voltage, Unit::Volt}).look == LookId::Classic);
  CHECK(normalised(Setup{LookId::Modern, ChannelKind::IntakeAir, Unit::Celsius}).look ==
        LookId::Classic);
  CHECK(normalised(Setup{LookId::Retro, ChannelKind::Boost, Unit::Bar}).look == LookId::Retro);
}

TEST_CASE("setup: a unit the channel cannot be shown in becomes its default") {
  CHECK(normalised(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Psi}).unit == Unit::Celsius);
  CHECK(normalised(Setup{LookId::Classic, ChannelKind::Boost, Unit::Volt}).unit == Unit::Psi);
  CHECK(normalised(Setup{LookId::Classic, ChannelKind::Voltage, Unit::Kpa}).unit == Unit::Volt);
}

TEST_CASE("setup: its profile is the channel shown in that unit") {
  CHECK(profileOf(Setup{LookId::Classic, ChannelKind::Boost, Unit::Bar}).unit == Unit::Bar);
  CHECK(profileOf(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Fahrenheit}).unit ==
        Unit::Fahrenheit);
  // An impossible unit falls back to the channel's default rather than to nonsense.
  CHECK(profileOf(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Psi}).unit == Unit::Celsius);
}

TEST_CASE("setup: encoding and decoding round-trip every valid setup") {
  const LookId looks[] = {LookId::Retro, LookId::Classic, LookId::Minimal, LookId::Motorsport,
                          LookId::Modern};
  const ChannelKind channels[] = {ChannelKind::Boost, ChannelKind::Coolant, ChannelKind::IntakeAir,
                                  ChannelKind::Voltage};
  for (LookId look : looks) {
    for (ChannelKind channel : channels) {
      const Setup setup = normalised(Setup{look, channel, defaultUnit(channel)});
      Setup back;
      CHECK(decodeSetup(encodeSetup(setup), back));
      CHECK(back == setup);
    }
  }
}

TEST_CASE("setup: a word that is not a setup is refused and leaves the result alone") {
  const Setup valid{LookId::Minimal, ChannelKind::Boost, Unit::Bar};
  const uint32_t good = encodeSetup(valid);
  Setup out{LookId::Motorsport, ChannelKind::Voltage, Unit::Volt};
  const Setup before = out;

  CHECK_FALSE(decodeSetup(0xFFFFFFFFu, out));  // erased flash
  CHECK_FALSE(decodeSetup(0u, out));
  CHECK_FALSE(decodeSetup(good ^ 0x01000000u, out));  // wrong marker
  CHECK_FALSE(decodeSetup(good | (9u << 16), out));   // no such look
  CHECK_FALSE(decodeSetup(good | (9u << 8), out));    // no such channel
  CHECK_FALSE(decodeSetup(good | 9u, out));           // no such unit
  CHECK(out == before);
}

TEST_CASE("setup: decoding repairs a combination that cannot exist") {
  // Retro showing coolant in psi: as if written by a build that allowed it.
  const uint32_t word = (0x47u << 24) | (static_cast<uint32_t>(LookId::Retro) << 16) |
                        (static_cast<uint32_t>(ChannelKind::Coolant) << 8) |
                        static_cast<uint32_t>(Unit::Psi);
  Setup out;
  CHECK(decodeSetup(word, out));
  CHECK(out == Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius});
}
