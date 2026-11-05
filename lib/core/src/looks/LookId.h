#pragma once

#include <string_view>

namespace gauge {

/// The looks the gauge can wear. Which one is drawn is a choice; how it is drawn is an ILook.
enum class LookId { Retro, Classic, Minimal, Motorsport, Modern };

constexpr int kLookIdCount = 5;

/// Retro and Modern are pictures of a psi boost dial: they cannot show another channel.
constexpr bool lookNeedsBoost(LookId look) {
  return look == LookId::Retro || look == LookId::Modern;
}

/// Recognise a look by its lower-case name: retro, classic, minimal, motorsport or modern.
/// False if it is none.
bool parseLookId(std::string_view name, LookId& look);

}  // namespace gauge
