// SettingsMenu: stepping through look, gauge and unit, and drawing itself.

#include "test_support.h"

using namespace support;

namespace {

constexpr Color kPanel{16, 18, 22};
constexpr Color kDotOn{255, 176, 32};
constexpr Color kDotOff{70, 76, 84};

/// Press Select to open the menu, then Select `fields` more times to reach that choice.
void openOn(SettingsMenu& menu, int fields) {
  menu.handle(InputEvent::Select);
  for (int i = 0; i < fields; ++i) menu.handle(InputEvent::Select);
}

/// How many pixels in the value's row are the value's (white) colour.
int valuePixels(const SoftCanvas& canvas) {
  int n = 0;
  for (int y = 100; y < 140; ++y)
    for (int x = 10; x < 230; ++x)
      if (same(canvas.image().get(x, y), kWhite)) ++n;
  return n;
}

/// What the menu draws on a black 240x240 screen.
void drawOnto(const SettingsMenu& menu, SoftCanvas& canvas) { menu.draw(canvas); }

}  // namespace

TEST_CASE("menu: closed, it ignores everything but Select, and draws nothing") {
  SettingsMenu menu(Setup{}, menuFonts());
  CHECK_FALSE(menu.active());
  CHECK_FALSE(menu.handle(InputEvent::None));
  CHECK_FALSE(menu.handle(InputEvent::Next));
  CHECK_FALSE(menu.active());
  SoftCanvas canvas(240, 240);
  drawOnto(menu, canvas);
  CHECK(countNonBlack(canvas.image()) == 0);

  CHECK_FALSE(menu.handle(InputEvent::Select));
  CHECK(menu.active());
}

TEST_CASE("menu: a setup that cannot exist is repaired when the menu is given it") {
  SettingsMenu menu(Setup{LookId::Retro, ChannelKind::Coolant, Unit::Psi}, menuFonts());
  CHECK(menu.setup() == Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius});
}

TEST_CASE("menu: Next steps through the looks and wraps") {
  SettingsMenu menu(Setup{}, menuFonts());  // Retro on boost
  openOn(menu, 0);
  for (int i = 0; i < kLookIdCount; ++i) menu.handle(InputEvent::Next);  // all the way round
  menu.handle(InputEvent::Select);
  menu.handle(InputEvent::Select);
  CHECK_FALSE(menu.handle(InputEvent::Select));  // nothing changed, so nothing to apply
  CHECK(menu.setup().look == LookId::Retro);
  CHECK_FALSE(menu.active());
}

TEST_CASE("menu: on another channel the looks skip the pictures of a boost dial") {
  SettingsMenu menu(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius}, menuFonts());
  openOn(menu, 0);
  menu.handle(InputEvent::Next);  // Minimal
  menu.handle(InputEvent::Next);  // Motorsport
  menu.handle(InputEvent::Next);  // skips Modern and Retro: Classic again
  menu.handle(InputEvent::Select);
  menu.handle(InputEvent::Select);
  CHECK_FALSE(menu.handle(InputEvent::Select));  // back where it started
  CHECK(menu.setup().look == LookId::Classic);
}

TEST_CASE("menu: choosing another gauge repairs the look and the unit") {
  SettingsMenu menu(Setup{LookId::Retro, ChannelKind::Boost, Unit::Kpa}, menuFonts());
  openOn(menu, 1);                // the gauge
  menu.handle(InputEvent::Next);  // coolant
  menu.handle(InputEvent::Select);
  CHECK(menu.handle(InputEvent::Select));
  CHECK(menu.setup() == Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius});
}

TEST_CASE("menu: the gauges go round: boost, coolant, intake, volts, boost") {
  SettingsMenu menu(Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi}, menuFonts());
  openOn(menu, 1);
  menu.handle(InputEvent::Next);
  menu.handle(InputEvent::Next);
  menu.handle(InputEvent::Next);
  menu.handle(InputEvent::Select);
  CHECK(menu.handle(InputEvent::Select));
  CHECK(menu.setup() == Setup{LookId::Classic, ChannelKind::Voltage, Unit::Volt});

  openOn(menu, 1);
  menu.handle(InputEvent::Next);  // back to boost, in its own unit
  menu.handle(InputEvent::Select);
  CHECK(menu.handle(InputEvent::Select));
  CHECK(menu.setup() == Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi});
}

TEST_CASE("menu: the units go round those the channel can be shown in") {
  SettingsMenu boost(Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi}, menuFonts());
  openOn(boost, 2);
  boost.handle(InputEvent::Next);  // bar
  boost.handle(InputEvent::Next);  // kpa
  CHECK(boost.handle(InputEvent::Select));
  CHECK(boost.setup().unit == Unit::Kpa);

  SettingsMenu coolant(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius}, menuFonts());
  openOn(coolant, 2);
  coolant.handle(InputEvent::Next);
  CHECK(coolant.handle(InputEvent::Select));
  CHECK(coolant.setup().unit == Unit::Fahrenheit);

  SettingsMenu volts(Setup{LookId::Classic, ChannelKind::Voltage, Unit::Volt}, menuFonts());
  openOn(volts, 2);
  volts.handle(InputEvent::Next);  // the only one: stays
  CHECK_FALSE(volts.handle(InputEvent::Select));
  CHECK(volts.setup().unit == Unit::Volt);
}

TEST_CASE("menu: None while open changes nothing") {
  SettingsMenu menu(Setup{}, menuFonts());
  openOn(menu, 0);
  CHECK_FALSE(menu.handle(InputEvent::None));
  CHECK(menu.active());
}

TEST_CASE("menu: setCurrent shows a setup chosen elsewhere the next time it opens") {
  SettingsMenu menu(Setup{}, menuFonts());
  menu.setCurrent(Setup{LookId::Motorsport, ChannelKind::IntakeAir, Unit::Fahrenheit});
  CHECK(menu.setup() == Setup{LookId::Motorsport, ChannelKind::IntakeAir, Unit::Fahrenheit});
  menu.setCurrent(Setup{LookId::Modern, ChannelKind::Voltage, Unit::Volt});  // impossible
  CHECK(menu.setup().look == LookId::Classic);
}

TEST_CASE("menu: it covers the gauge with a panel and lights the dot of the choice being set") {
  for (int field = 0; field < 3; ++field) {
    SettingsMenu menu(Setup{}, menuFonts());
    openOn(menu, field);
    SoftCanvas canvas(240, 240);
    drawOnto(menu, canvas);
    CAPTURE(field);
    CHECK(same(canvas.image().get(120, 20), kPanel));
    for (int dot = 0; dot < 3; ++dot) {
      CHECK(same(canvas.image().get(120 + (dot - 1) * 16, 168), dot == field ? kDotOn : kDotOff));
    }
    CHECK(valuePixels(canvas) > 0);
  }
}

TEST_CASE("menu: every look, gauge and unit has a label") {
  const auto shows = [](const SettingsMenu& menu) {
    SoftCanvas canvas(240, 240);
    drawOnto(menu, canvas);
    return valuePixels(canvas) > 0;
  };

  SettingsMenu looks(Setup{}, menuFonts());
  openOn(looks, 0);
  for (int i = 0; i < kLookIdCount; ++i) {
    CHECK(shows(looks));
    looks.handle(InputEvent::Next);
  }

  SettingsMenu gauges(Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi}, menuFonts());
  openOn(gauges, 1);
  for (size_t i = 0; i < kChannelKindCount; ++i) {
    CHECK(shows(gauges));
    gauges.handle(InputEvent::Next);
  }

  SettingsMenu boostUnits(Setup{LookId::Classic, ChannelKind::Boost, Unit::Psi}, menuFonts());
  openOn(boostUnits, 2);
  for (int i = 0; i < 3; ++i) {  // psi, bar, kpa
    CHECK(shows(boostUnits));
    boostUnits.handle(InputEvent::Next);
  }
  SettingsMenu tempUnits(Setup{LookId::Classic, ChannelKind::Coolant, Unit::Celsius}, menuFonts());
  openOn(tempUnits, 2);
  for (int i = 0; i < 2; ++i) {  // celsius, fahrenheit
    CHECK(shows(tempUnits));
    tempUnits.handle(InputEvent::Next);
  }
  SettingsMenu voltUnits(Setup{LookId::Classic, ChannelKind::Voltage, Unit::Volt}, menuFonts());
  openOn(voltUnits, 2);
  CHECK(shows(voltUnits));
}
