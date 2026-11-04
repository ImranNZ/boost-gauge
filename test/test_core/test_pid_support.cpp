// PidSupport: discovering which mode 01 PIDs a car implements.

#include <map>

#include "test_support.h"

using namespace support;

namespace {

/// A car that answers the support PIDs from a table, and records what it was asked.
class BitmapClient : public IObdClient {
 public:
  std::map<uint8_t, std::array<uint8_t, 4>> bitmaps;
  uint8_t replyCount = 4;
  std::vector<uint8_t> asked;

  bool begin() override { return true; }
  bool readPid(uint8_t, uint8_t pid, ObdData& out) override {
    asked.push_back(pid);
    const auto it = bitmaps.find(pid);
    if (it == bitmaps.end()) return false;
    out.bytes = it->second;
    out.count = replyCount;
    return true;
  }
};

// PIDs 04, 05, 0B and 0C: byte 0 holds PIDs 01-08 (top bit = 01), byte 1 holds 09-10.
constexpr std::array<uint8_t, 4> kFirstRangeOnly = {0x18, 0x30, 0x00, 0x00};
// The same, plus the last bit: "the next range (21-40) exists".
constexpr std::array<uint8_t, 4> kFirstRangeThenMore = {0x18, 0x30, 0x00, 0x01};
// PID 33 in the 21-40 range: bit 18 from the top, so byte 2, 0x20.
constexpr std::array<uint8_t, 4> kSecondRange = {0x00, 0x00, 0x20, 0x00};

}  // namespace

TEST_CASE("pid support: a single range is read, and the rest is known to be empty") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeOnly;
  PidSupport support;

  REQUIRE(support.discover(car));
  CHECK(support.supports(0x04));
  CHECK(support.supports(0x05));
  CHECK(support.supports(0x0B));
  CHECK(support.supports(0x0C));
  CHECK_FALSE(support.supports(0x0D));
  CHECK(support.count() == 4);

  CHECK(support.knows(0x0D));           // inside the range that was read
  CHECK(support.knows(0x33));           // the car said there is nothing further...
  CHECK_FALSE(support.supports(0x33));  // ...so it is known to be absent
  CHECK(car.asked.size() == 1);         // and the chain stopped there
}

TEST_CASE("pid support: the last bit chains to the next range") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeThenMore;
  car.bitmaps[0x20] = kSecondRange;
  PidSupport support;

  REQUIRE(support.discover(car));
  CHECK(support.supports(0x0B));
  CHECK(support.supports(0x33));
  CHECK_FALSE(support.supports(0x34));
  CHECK(support.knows(0x33));
  CHECK((car.asked == std::vector<uint8_t>{0x00, 0x20}));
}

TEST_CASE("pid support: a car that chains through every range is read to the end") {
  BitmapClient car;
  for (uint8_t base = 0x00; base <= 0xC0; base += 0x20)
    car.bitmaps[base] = {0x00, 0x00, 0x00, 0x01};
  PidSupport support;

  REQUIRE(support.discover(car));
  CHECK(car.asked.size() == 7);  // 00, 20, 40, 60, 80, A0, C0: never past C0
  CHECK(support.knows(0xE0));
  CHECK(support.count() == 7);  // each range's last bit: PIDs 20, 40, 60, 80, A0, C0 and E0
}

TEST_CASE("pid support: a car that does not answer PID 00 leaves everything unknown") {
  BitmapClient car;  // answers nothing
  PidSupport support;

  CHECK_FALSE(support.discover(car));
  CHECK_FALSE(support.knows(0x0B));
  CHECK_FALSE(support.supports(0x0B));
  CHECK_FALSE(support.knows(0x00));
  CHECK_FALSE(support.supports(0x00));
  CHECK(support.count() == 0);
}

TEST_CASE("pid support: a chain that breaks keeps what it read and leaves the rest unknown") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeThenMore;  // promises a second range, then never answers it
  PidSupport support;

  REQUIRE(support.discover(car));
  CHECK(support.supports(0x0B));
  CHECK(support.knows(0x0B));
  CHECK_FALSE(support.knows(0x33));  // not "unsupported": we just do not know
  CHECK_FALSE(support.supports(0x33));
}

TEST_CASE("pid support: a short reply is treated as no answer") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeOnly;
  car.replyCount = 2;
  PidSupport support;
  CHECK_FALSE(support.discover(car));
  CHECK_FALSE(support.knows(0x0B));
}

TEST_CASE("pid support: PID 00 is known to be supported once anything was read") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeOnly;
  PidSupport support;
  REQUIRE(support.discover(car));
  CHECK(support.knows(0x00));
  CHECK(support.supports(0x00));
}

TEST_CASE("pid support: discovering again replaces the old answer") {
  BitmapClient car;
  car.bitmaps[0x00] = kFirstRangeOnly;
  PidSupport support;
  REQUIRE(support.discover(car));
  REQUIRE(support.supports(0x0B));

  car.bitmaps[0x00] = {0x00, 0x00, 0x00, 0x00};  // a different car
  REQUIRE(support.discover(car));
  CHECK_FALSE(support.supports(0x0B));
  CHECK(support.count() == 0);
}
