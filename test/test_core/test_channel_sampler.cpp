// ChannelSampler: several channels over one link, and which is read when.

#include "test_support.h"

using namespace support;

namespace {

/// A fake car, a boost channel, a coolant channel and a sampler over them.
struct Rig {
  ManualClock clock;
  FakeEcu ecu{clock};
  FakeCanBus bus{ecu};
  CanObdClient inner{bus, clock};
  CountingObdClient client{inner};
  BoostChannel boost{client, clock};
  PidChannel coolant = PidChannel::coolant();
  ChannelSampler sampler;

  explicit Rig(const ChannelSampler::Options& options = {}) : sampler(client, clock, options) {
    ecu.setSupported(obd::kPidCoolantTemp, true);
    ecu.holdMapKpa(150.0f);
    ecu.setCoolantC(90.0f);
    sampler.add(boost);
    sampler.add(coolant);
  }
};

}  // namespace

TEST_CASE("channel sampler: holds a limited number of channels") {
  ManualClock clock;
  StubObdClient client;
  ChannelSampler sampler(client, clock);
  std::vector<PidChannel> channels(ChannelSampler::kMaxChannels + 1, PidChannel::coolant());

  for (size_t i = 0; i < ChannelSampler::kMaxChannels; ++i) CHECK(sampler.add(channels[i]));
  CHECK(sampler.channelCount() == ChannelSampler::kMaxChannels);
  CHECK_FALSE(sampler.add(channels.back()));
  CHECK(sampler.channelCount() == ChannelSampler::kMaxChannels);
}

TEST_CASE("channel sampler: every channel is read once straight after the link comes up") {
  Rig rig;
  ChannelSampler::Step step;

  REQUIRE(rig.sampler.step(step));
  CHECK(step.channel == 0);  // boost: they are all due, and boost was added first
  CHECK(step.reading.valid);

  REQUIRE(rig.sampler.step(step));
  CHECK(step.channel == 1);  // coolant has waited longest now
  CHECK(step.reading.valid);
  CHECK(step.reading.value == doctest::Approx(90.0f));
}

TEST_CASE("channel sampler: the fast channel runs flat out and the slow one keeps to its period") {
  Rig rig;
  ChannelSampler::Step step;
  std::vector<uint32_t> coolantAt;
  int boostReads = 0;

  for (int i = 0; i < 500; ++i) {  // five seconds, 10 ms per step
    if (rig.sampler.step(step)) {
      if (step.channel == 0) ++boostReads;
      if (step.channel == 1) coolantAt.push_back(rig.clock.nowMs());
    }
    rig.clock.advance(10);
  }

  CHECK(boostReads > 480);  // nearly every step
  REQUIRE(coolantAt.size() >= 4);
  CHECK(coolantAt.size() <= 6);
  for (size_t i = 1; i < coolantAt.size(); ++i) {
    const uint32_t gap = coolantAt[i] - coolantAt[i - 1];
    CHECK(gap >= 1000);  // never early...
    CHECK(gap <= 1030);  // ...and not starved by the fast channel
  }
}

TEST_CASE("channel sampler: nothing is attempted while no channel is due") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setSupported(obd::kPidCoolantTemp, true);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  PidChannel coolant = PidChannel::coolant();
  ChannelSampler sampler(client, clock);
  sampler.add(coolant);
  ChannelSampler::Step step;

  CHECK(sampler.step(step));        // the first read
  CHECK_FALSE(sampler.step(step));  // not due again yet
  clock.advance(999);
  CHECK_FALSE(sampler.step(step));
  clock.advance(1);
  CHECK(sampler.step(step));
}

TEST_CASE("channel sampler: with the link down nothing is attempted") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  PidChannel coolant = PidChannel::coolant();
  ChannelSampler sampler(client, clock);
  sampler.add(coolant);
  ChannelSampler::Step step;

  CHECK_FALSE(sampler.step(step));
}

TEST_CASE("channel sampler: a channel the car lacks reads as nothing and is left alone") {
  Rig rig;
  rig.ecu.setSupported(obd::kPidCoolantTemp, false);
  ChannelSampler::Step step;

  bool sawCoolant = false;
  for (int i = 0; i < 100; ++i) {
    if (rig.sampler.step(step) && step.channel == 1) {
      sawCoolant = true;
      CHECK_FALSE(step.reading.valid);
    }
    rig.clock.advance(50);
  }
  CHECK(sawCoolant);
  CHECK(rig.client.asked[obd::kPidCoolantTemp] == 0);  // never asked
  CHECK(rig.client.begins == 1);                       // and it did not look like a dead link
}

TEST_CASE("channel sampler: failed reads count against the link and trigger a reconnect") {
  ChannelSampler::Options options;
  options.link.failuresBeforeReconnect = 2;
  options.link.reconnectIntervalMs = 1000;
  Rig rig(options);
  ChannelSampler::Step step;

  REQUIRE(rig.sampler.step(step));
  CHECK(step.reading.valid);

  rig.ecu.setOnline(false);
  REQUIRE(rig.sampler.step(step));
  CHECK_FALSE(step.reading.valid);  // a failed read is reported, as an invalid reading
  REQUIRE(rig.sampler.step(step));
  CHECK_FALSE(step.reading.valid);
  CHECK_FALSE(rig.sampler.step(step));  // two failures: the link is declared down

  rig.ecu.setOnline(true);
  rig.clock.advance(1500);
  CHECK(rig.sampler.step(step));  // reconnects and reads again
  CHECK(rig.client.begins == 2);
  CHECK(step.reading.valid);
}

TEST_CASE("channel sampler: the schedule survives the millisecond clock wrapping") {
  Rig rig;
  rig.clock.advance(0xFFFFFF00u);  // 256 ms before the 32-bit clock wraps
  ChannelSampler::Step step;
  int coolantReads = 0;

  for (int i = 0; i < 300; ++i) {  // three seconds, across the wrap
    if (rig.sampler.step(step) && step.channel == 1) ++coolantReads;
    rig.clock.advance(10);
  }
  CHECK(coolantReads >= 3);
  CHECK(coolantReads <= 4);
}

TEST_CASE("polling source: reports one channel and repeats its last reading otherwise") {
  Rig rig;
  PollingSource coolant(rig.sampler, 1);

  const Sample first = coolant.latest();  // that step read boost, not coolant
  CHECK(first.seq == 0);
  CHECK_FALSE(first.reading.valid);

  const Sample second = coolant.latest();  // now coolant's turn
  CHECK(second.seq == 1);
  CHECK(second.reading.valid);
  CHECK(second.reading.value == doctest::Approx(90.0f));

  const Sample third = coolant.latest();  // boost again: coolant's reading is repeated
  CHECK(third.seq == 1);
  CHECK(third.reading.value == doctest::Approx(90.0f));
}

TEST_CASE("channel mailboxes: one slot per channel") {
  ChannelMailboxes mailboxes;
  mailboxes.publish(0, {7.5f, true});
  mailboxes.publish(3, {88.0f, true});

  CHECK(mailboxes.source(0).latest().reading.value == doctest::Approx(7.5f));
  CHECK(mailboxes.source(3).latest().reading.value == doctest::Approx(88.0f));
  CHECK(mailboxes.source(1).latest().seq == 0);  // untouched

  // Beyond the last slot nothing is published, and reading falls back to the last slot.
  mailboxes.publish(ChannelSampler::kMaxChannels, {1.0f, true});
  CHECK(mailboxes.source(ChannelSampler::kMaxChannels - 1).latest().seq == 0);
  mailboxes.publish(ChannelSampler::kMaxChannels - 1, {5.0f, true});
  CHECK(mailboxes.source(100).latest().reading.value == doctest::Approx(5.0f));
}

TEST_CASE("sampler loop: reports whether the car was asked anything") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  PidChannel coolant = PidChannel::coolant();
  ChannelSampler sampler(client, clock);
  sampler.add(coolant);
  ChannelMailboxes mailboxes;
  SamplerLoop loop(sampler, mailboxes);

  CHECK_FALSE(loop.step());  // the link is down: nothing asked, nothing published
  CHECK(mailboxes.source(0).latest().seq == 0);
}

TEST_CASE(
    "channel sampler: channels that are always due take turns, even if the clock stands still") {
  ManualClock clock;
  FakeEcu ecu(clock);
  ecu.setSupported(obd::kPidCoolantTemp, true);
  ecu.setSupported(obd::kPidIntakeAirTemp, true);
  FakeCanBus bus(ecu);
  CanObdClient client(bus, clock);
  PidChannel first(obd::kPidCoolantTemp, 1, &obd::coolantTempC, 0);
  PidChannel second(obd::kPidIntakeAirTemp, 1, &obd::intakeAirTempC, 0);
  ChannelSampler sampler(client, clock);
  sampler.add(first);
  sampler.add(second);
  ChannelSampler::Step step;

  std::vector<size_t> order;
  for (int i = 0; i < 6; ++i) {
    REQUIRE(sampler.step(step));  // the clock never moves, and both are always due
    order.push_back(step.channel);
  }
  CHECK((order == std::vector<size_t>{0, 1, 0, 1, 0, 1}));
}

TEST_CASE("channel sampler: a channel added after the link is up is told what the car supports") {
  Rig rig;
  rig.ecu.setSupported(obd::kPidCoolantTemp, false);
  ChannelSampler::Step step;
  REQUIRE(rig.sampler.step(step));  // the link comes up and the car is discovered

  rig.sampler.clear();
  CHECK(rig.sampler.channelCount() == 0);
  CHECK_FALSE(rig.sampler.step(step));  // nothing to read

  REQUIRE(rig.sampler.add(rig.coolant));  // the car has just said it has no coolant PID
  CHECK(rig.sampler.channelCount() == 1);
  REQUIRE(rig.sampler.step(step));
  CHECK(step.channel == 0);
  CHECK_FALSE(step.reading.valid);
  CHECK(rig.client.asked[obd::kPidCoolantTemp] == 0);  // so it is not asked
}

TEST_CASE("channel sampler: a channel added after the link is up is read straight away") {
  Rig rig;
  ChannelSampler::Step step;
  REQUIRE(rig.sampler.step(step));
  REQUIRE(rig.sampler.step(step));  // both channels read once

  rig.sampler.clear();
  rig.sampler.add(rig.coolant);  // read a moment ago, but the sampler starts it afresh
  REQUIRE(rig.sampler.step(step));
  CHECK(step.reading.valid);
}

TEST_CASE("polling source: switching channel forgets the old reading") {
  Rig rig;
  PollingSource source(rig.sampler, 0);
  const Sample before = source.latest();
  REQUIRE(before.reading.valid);

  source.setChannel(1);
  const Sample after =
      source.latest();  // that step read coolant... or boost: either way not channel 0
  CHECK(after.seq != before.seq);
  CHECK(after.reading.value != doctest::Approx(before.reading.value));
}
