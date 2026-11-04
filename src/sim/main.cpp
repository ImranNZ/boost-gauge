// PC simulator: runs the real gauge stack against mocks, with no hardware.
//
//   pio run -e sim -t exec             stills: out/sheet.png, out/no_link.png
//   python tools/preview.py            live animation in your browser
//
// Options for the live stream:  --live [--look retro|classic|minimal|motorsport|modern] [--no-pace]

#include <cstring>
#include <string_view>

#include "Modes.h"

int main(int argc, char** argv) {
  bool live = false;
  bool paced = true;
  std::string_view look = "retro";
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--live") == 0) live = true;
    if (std::strcmp(argv[i], "--no-pace") == 0) paced = false;
    if (std::strcmp(argv[i], "--look") == 0 && i + 1 < argc) look = argv[++i];
  }
  return live ? sim::runLive(look, paced) : sim::runSnapshots();
}
