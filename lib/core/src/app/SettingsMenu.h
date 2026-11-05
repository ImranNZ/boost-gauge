#pragma once

#include "app/IInput.h"
#include "app/Setup.h"
#include "render/Font.h"
#include "render/IOverlay.h"

namespace gauge {

/// The on-gauge menu for choosing the look, the reading and its unit.
///
/// Closed, it does nothing until Select opens it. Open, it shows one choice at a time: Next
/// changes the value, Select accepts it and moves on (look, then gauge, then unit). Accepting the
/// last one closes the menu. It holds the choice, draws itself as an overlay, and knows nothing
/// of buttons, flash or the sampler.
class SettingsMenu : public IOverlay {
 public:
  /// The typefaces it draws with: a small one for the title, a larger one for the value (capitals,
  /// digits and spaces).
  struct Fonts {
    Font title;
    Font value;
  };

  SettingsMenu(const Setup& current, const Fonts& fonts)
      : fonts_(fonts), committed_(normalised(current)) {}

  /// Feed one event. True when the menu has just closed on a different setup than it opened
  /// with, which is now setup().
  bool handle(InputEvent event);

  /// Something else changed the setup (a typed command in the simulator): show that from now on.
  void setCurrent(const Setup& current) { committed_ = normalised(current); }

  /// The setup in force: the one given at construction until a change is accepted.
  const Setup& setup() const { return committed_; }

  bool active() const override { return open_; }
  void draw(IDrawSurface& surface) const override;

 private:
  enum class Field { Look, Gauge, Unit };

  void changeValue();

  Fonts fonts_;
  bool open_ = false;
  Field field_ = Field::Look;
  Setup committed_;
  Setup working_;  // what the menu shows while open
};

}  // namespace gauge
