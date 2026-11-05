#pragma once

namespace gauge {

/// What the owner did, as far as the gauge cares. A board adapter turns its button, touch screen
/// or motion sensor into these, so nothing else knows which one it is.
enum class InputEvent {
  None,    ///< nothing happened
  Next,    ///< change the value (a short press, a swipe)
  Select,  ///< accept it and move on (a long press, a tap)
};

class IInput {
 public:
  virtual ~IInput() = default;

  /// The next event, or None. Never waits.
  virtual InputEvent poll() = 0;
};

}  // namespace gauge
