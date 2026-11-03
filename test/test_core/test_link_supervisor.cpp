// LinkSupervisor: connecting, retrying, and noticing a dead link.

#include "test_support.h"

using namespace support;

TEST_CASE("link supervisor: connects on first use and reports the link up") {
  ManualClock clock;
  StubObdClient client;
  LinkSupervisor link(client, clock);

  CHECK(link.connectionCount() == 0);
  CHECK(link.ensureLinked());
  CHECK(client.beginCalls == 1);
  CHECK(link.connectionCount() == 1);

  CHECK(link.ensureLinked());  // already up: no new connection
  CHECK(client.beginCalls == 1);
}

TEST_CASE("link supervisor: failed connection is retried only after the interval") {
  ManualClock clock;
  StubObdClient client;
  client.beginResult = false;
  LinkSupervisor::Options options;
  options.reconnectIntervalMs = 1000;
  LinkSupervisor link(client, clock, options);

  CHECK_FALSE(link.ensureLinked());
  CHECK(client.beginCalls == 1);
  CHECK(link.connectionCount() == 0);  // nothing was established

  clock.advance(999);
  CHECK_FALSE(link.ensureLinked());
  CHECK(client.beginCalls == 1);  // throttled

  clock.advance(1);
  client.beginResult = true;
  CHECK(link.ensureLinked());
  CHECK(client.beginCalls == 2);
  CHECK(link.connectionCount() == 1);
}

TEST_CASE("link supervisor: a run of failures takes the link down, then it reconnects") {
  ManualClock clock;
  StubObdClient client;
  LinkSupervisor::Options options;
  options.failuresBeforeReconnect = 3;
  options.reconnectIntervalMs = 500;
  LinkSupervisor link(client, clock, options);
  REQUIRE(link.ensureLinked());

  link.noteFailure();
  link.noteFailure();
  CHECK(link.ensureLinked());  // two failures: still considered up

  link.noteFailure();          // the third
  clock.advance(500);
  CHECK(link.ensureLinked());  // it was down, so this reconnects
  CHECK(client.beginCalls == 2);
  CHECK(link.connectionCount() == 2);
}

TEST_CASE("link supervisor: a success resets the failure streak") {
  ManualClock clock;
  StubObdClient client;
  LinkSupervisor::Options options;
  options.failuresBeforeReconnect = 3;
  LinkSupervisor link(client, clock, options);
  REQUIRE(link.ensureLinked());

  link.noteFailure();
  link.noteFailure();
  link.noteSuccess();
  link.noteFailure();
  link.noteFailure();
  CHECK(link.ensureLinked());
  CHECK(client.beginCalls == 1);  // never dropped
}
