#include "looks/LookId.h"

namespace gauge {

bool parseLookId(std::string_view name, LookId& look) {
  if (name == "retro")
    look = LookId::Retro;
  else if (name == "classic")
    look = LookId::Classic;
  else if (name == "minimal")
    look = LookId::Minimal;
  else if (name == "motorsport")
    look = LookId::Motorsport;
  else if (name == "modern")
    look = LookId::Modern;
  else
    return false;
  return true;
}

}  // namespace gauge
