// LookId: naming the looks and which of them need the boost channel.

#include "test_support.h"

using namespace support;

TEST_CASE("look id: every look is recognised by its lower-case name") {
  LookId look = LookId::Retro;
  CHECK(parseLookId("retro", look));
  CHECK(look == LookId::Retro);
  CHECK(parseLookId("classic", look));
  CHECK(look == LookId::Classic);
  CHECK(parseLookId("minimal", look));
  CHECK(look == LookId::Minimal);
  CHECK(parseLookId("motorsport", look));
  CHECK(look == LookId::Motorsport);
  CHECK(parseLookId("modern", look));
  CHECK(look == LookId::Modern);
}

TEST_CASE("look id: an unknown name is refused and leaves the look alone") {
  LookId look = LookId::Minimal;
  CHECK_FALSE(parseLookId("Retro", look));  // case matters
  CHECK_FALSE(parseLookId("", look));
  CHECK_FALSE(parseLookId("sport", look));
  CHECK(look == LookId::Minimal);
}

TEST_CASE("look id: only the pictures of a psi boost dial need the boost channel") {
  CHECK(lookNeedsBoost(LookId::Retro));
  CHECK(lookNeedsBoost(LookId::Modern));
  CHECK_FALSE(lookNeedsBoost(LookId::Classic));
  CHECK_FALSE(lookNeedsBoost(LookId::Minimal));
  CHECK_FALSE(lookNeedsBoost(LookId::Motorsport));
}
