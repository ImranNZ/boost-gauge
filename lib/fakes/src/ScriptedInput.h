#pragma once

#include <deque>

#include "app/IInput.h"

namespace gauge {

/// An IInput that returns the events it was given, in order, then None. For tests and the
/// simulator; a board has its own adapter.
class ScriptedInput : public IInput {
 public:
  void push(InputEvent event) { queue_.push_back(event); }

  InputEvent poll() override {
    if (queue_.empty()) return InputEvent::None;
    const InputEvent event = queue_.front();
    queue_.pop_front();
    return event;
  }

 private:
  std::deque<InputEvent> queue_;
};

}  // namespace gauge
