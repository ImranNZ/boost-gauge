#include "app/SettingsMenu.h"

namespace gauge {
namespace {

constexpr Color kPanel{16, 18, 22};
constexpr Color kTitle{138, 147, 157};
constexpr Color kValue{255, 255, 255};
constexpr Color kDotOn{255, 176, 32};
constexpr Color kDotOff{70, 76, 84};

constexpr int kCentre = 120;
constexpr int kTitleBaseline = 84;
constexpr int kValueBaseline = 129;
constexpr int kDots = 3;

const char* lookLabel(LookId look) {
  switch (look) {
    case LookId::Classic:
      return "CLASSIC";
    case LookId::Minimal:
      return "MINIMAL";
    case LookId::Motorsport:
      return "MOTORSPORT";
    case LookId::Modern:
      return "MODERN";
    case LookId::Retro:
    default:
      return "RETRO";
  }
}

const char* channelLabel(ChannelKind channel) {
  switch (channel) {
    case ChannelKind::Coolant:
      return "COOLANT";
    case ChannelKind::IntakeAir:
      return "INTAKE";
    case ChannelKind::Voltage:
      return "VOLTS";
    case ChannelKind::Boost:
    default:
      return "BOOST";
  }
}

const char* unitLabel(Unit unit) {
  switch (unit) {
    case Unit::Bar:
      return "BAR";
    case Unit::Kpa:
      return "KPA";
    case Unit::Celsius:
      return "CELSIUS";
    case Unit::Fahrenheit:
      return "FAHRENHEIT";
    case Unit::Volt:
      return "VOLTS";
    case Unit::Psi:
    default:
      return "PSI";
  }
}

/// The next look after `from` that `channel` can be shown with.
LookId nextLook(LookId from, ChannelKind channel) {
  LookId look = from;
  do {
    look = static_cast<LookId>((static_cast<int>(look) + 1) % kLookIdCount);
  } while (lookNeedsBoost(look) && channel != ChannelKind::Boost);
  return look;
}

/// The next unit after `from` that `channel` can be shown in (itself, if there is only one).
Unit nextUnit(Unit from, ChannelKind channel) {
  constexpr int kUnits = static_cast<int>(Unit::Volt) + 1;
  GaugeProfile unused;
  Unit unit = from;
  do {
    unit = static_cast<Unit>((static_cast<int>(unit) + 1) % kUnits);
  } while (!profileFor(channel, unit, unused));
  return unit;
}

}  // namespace

bool SettingsMenu::handle(InputEvent event) {
  if (event == InputEvent::None) return false;

  if (!open_) {
    if (event == InputEvent::Select) {
      open_ = true;
      field_ = Field::Look;
      working_ = committed_;
    }
    return false;
  }

  if (event == InputEvent::Next) {
    changeValue();
    return false;
  }

  // Select: on to the next choice, or done after the last.
  if (field_ == Field::Look) {
    field_ = Field::Gauge;
  } else if (field_ == Field::Gauge) {
    field_ = Field::Unit;
  } else {
    open_ = false;
    const bool changed = working_ != committed_;
    committed_ = working_;
    return changed;
  }
  return false;
}

void SettingsMenu::changeValue() {
  if (field_ == Field::Look) {
    working_.look = nextLook(working_.look, working_.channel);
  } else if (field_ == Field::Gauge) {
    working_.channel = static_cast<ChannelKind>((static_cast<int>(working_.channel) + 1) %
                                                static_cast<int>(kChannelKindCount));
    working_ = normalised(working_);  // the look and unit may not suit the new channel
  } else {
    working_.unit = nextUnit(working_.unit, working_.channel);
  }
}

void SettingsMenu::draw(IDrawSurface& surface) const {
  if (!open_) return;

  surface.fillCircle(kCentre, kCentre, kCentre, kPanel);

  const char* title = "UNIT";
  const char* value = unitLabel(working_.unit);
  if (field_ == Field::Look) {
    title = "LOOK";
    value = lookLabel(working_.look);
  } else if (field_ == Field::Gauge) {
    title = "GAUGE";
    value = channelLabel(working_.channel);
  }
  fonts_.title.draw(surface, kCentre, kTitleBaseline, title, TextAlign::Center, kTitle);
  fonts_.value.draw(surface, kCentre, kValueBaseline, value, TextAlign::Center, kValue);

  // One dot per choice; the one being set is lit.
  const int lit = static_cast<int>(field_);
  for (int i = 0; i < kDots; ++i) {
    surface.fillCircle(kCentre + (i - 1) * 16, 168, 4, i == lit ? kDotOn : kDotOff);
  }
}

}  // namespace gauge
