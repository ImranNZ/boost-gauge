// SettingsController: input -> menu -> store and target. Also the fakes it is tested with.

#include <initializer_list>

#include "test_support.h"

using namespace support;

namespace {

struct RecordingTarget : ISetupTarget {
  void apply(const Setup& setup) override {
    ++applied;
    last = setup;
  }
  int applied = 0;
  Setup last;
};

struct Fixture {
  ScriptedInput input;
  MemorySettingsStore store;
  SettingsMenu menu{Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi}, menuFonts()};
  RecordingTarget target;
  SettingsController controller{input, menu, store, target};

  void press(std::initializer_list<InputEvent> events) {
    for (InputEvent event : events) {
      input.push(event);
      controller.tick();
    }
  }
};

}  // namespace

TEST_CASE("controller: nothing happens without input") {
  Fixture f;
  f.controller.tick();
  CHECK(f.target.applied == 0);
  CHECK(f.store.saves() == 0);
  CHECK_FALSE(f.menu.active());
}

TEST_CASE("controller: a finished change is saved and applied once") {
  Fixture f;
  // open, look -> Minimal, on to gauge, on to unit, unit -> bar.
  f.press({InputEvent::Select, InputEvent::Next, InputEvent::Select, InputEvent::Select,
           InputEvent::Next});
  CHECK(f.target.applied == 0);  // still choosing
  f.press({InputEvent::Select});

  CHECK(f.target.applied == 1);
  CHECK(f.target.last == Setup{LookId::Minimal, ChannelKind::Boost, Unit::Bar});
  CHECK(f.store.saves() == 1);
  Setup saved;
  CHECK(f.store.load(saved));
  CHECK(saved == f.target.last);
}

TEST_CASE("controller: leaving the menu without a change saves and applies nothing") {
  Fixture f;
  f.press({InputEvent::Select, InputEvent::Select, InputEvent::Select, InputEvent::Select});
  CHECK_FALSE(f.menu.active());
  CHECK(f.target.applied == 0);
  CHECK(f.store.saves() == 0);
}

TEST_CASE("memory store: load says no until something is saved") {
  MemorySettingsStore store;
  Setup out{LookId::Motorsport, ChannelKind::Voltage, Unit::Volt};
  CHECK_FALSE(store.load(out));
  CHECK(out == Setup{LookId::Motorsport, ChannelKind::Voltage, Unit::Volt});
}

TEST_CASE("scripted input: events come out in order, then None") {
  ScriptedInput input;
  CHECK(input.poll() == InputEvent::None);
  input.push(InputEvent::Next);
  input.push(InputEvent::Select);
  CHECK(input.poll() == InputEvent::Next);
  CHECK(input.poll() == InputEvent::Select);
  CHECK(input.poll() == InputEvent::None);
}
