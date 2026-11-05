// Handing readings from the sampler to the screen.

#include "test_support.h"

using namespace support;

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

TEST_CASE("mailbox: empty until published, then carries value and a new seq") {
  ReadingMailbox mailbox;
  Sample empty = mailbox.latest();
  CHECK(empty.seq == 0);
  CHECK_FALSE(empty.reading.valid);

  mailbox.publish({12.34f, true});
  Sample a = mailbox.latest();
  CHECK(a.seq == 1);
  CHECK(a.reading.valid);
  CHECK(a.reading.value == doctest::Approx(12.34f).epsilon(0.001));

  mailbox.publish({-9.87f, true});
  Sample b = mailbox.latest();
  CHECK(b.seq == 2);
  CHECK(b.reading.value == doctest::Approx(-9.87f).epsilon(0.001));

  CHECK(mailbox.latest().seq == 2);  // reading it does not consume it
}

TEST_CASE("mailbox: invalid flag, clamping and sequence wrap") {
  ReadingMailbox mailbox;
  mailbox.publish({0.0f, false});
  CHECK_FALSE(mailbox.latest().reading.valid);

  mailbox.publish({1000.0f, true});  // beyond +-327.67: clamps rather than wrapping
  CHECK(mailbox.latest().reading.value == doctest::Approx(327.67f).epsilon(0.001));
  mailbox.publish({-1000.0f, true});
  CHECK(mailbox.latest().reading.value == doctest::Approx(-327.68f).epsilon(0.001));

  // 15-bit sequence wraps back through 0 without breaking the payload.
  for (int i = 0; i < 32768; ++i) mailbox.publish({5.0f, true});
  const Sample wrapped = mailbox.latest();
  CHECK(wrapped.seq == (3 + 32768) % 32768);
  CHECK(wrapped.reading.value == doctest::Approx(5.0f));
}

TEST_CASE("polling source: every call is a fresh sample") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 150};
  client.baro = {true, 1, 100};
  BoostRig rig(client, clock);
  PollingSource source(rig.channels(), 0);

  const Sample a = source.latest();
  const Sample b = source.latest();
  CHECK(a.reading.valid);
  CHECK(b.seq == a.seq + 1);
}

TEST_CASE("sampler loop publishes each step into the mailbox") {
  ManualClock clock;
  StubObdClient client;
  client.map = {true, 1, 150};
  client.baro = {true, 1, 100};
  BoostRig rig(client, clock);
  ChannelMailboxes mailboxes;
  SamplerLoop loop(rig.channels(), mailboxes);
  IReadingSource& boost = mailboxes.source(0);

  CHECK(boost.latest().seq == 0);
  CHECK(loop.step());
  CHECK(boost.latest().seq == 1);
  CHECK(boost.latest().reading.valid);
  CHECK(boost.latest().reading.value == doctest::Approx(7.25f).epsilon(0.01));

  client.map = {false, 0, 0};
  CHECK(loop.step());
  CHECK_FALSE(boost.latest().reading.valid);
}
