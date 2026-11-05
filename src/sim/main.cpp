// PC simulator: runs the real gauge stack against mocks, with no hardware.
//
//   pio run -e sim -t exec             stills: out/sheet.png, out/no_link.png
//   python tools/preview.py            live animation in your browser
//
// Options for the live stream:  --live [--look retro|classic|minimal|motorsport|modern] [--no-pace]
//                               [--channel boost|coolant|intake|voltage] [--unit psi|bar|kpa|c|f|v]
//                               [--size 240x240]   (frames of another screen size, e.g. 466x466)

#include <cstdio>
#include <cstring>
#include <string_view>

#include "Modes.h"

int main(int argc, char** argv) {
  bool live = false;
  bool paced = true;
  std::string_view channel = "boost";
  std::string_view unit;
  std::string_view look = "retro";
  int width = 240;
  int height = 240;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--live") == 0) live = true;
    if (std::strcmp(argv[i], "--no-pace") == 0) paced = false;
    if (std::strcmp(argv[i], "--look") == 0 && i + 1 < argc) look = argv[++i];
    if (std::strcmp(argv[i], "--channel") == 0 && i + 1 < argc) channel = argv[++i];
    if (std::strcmp(argv[i], "--unit") == 0 && i + 1 < argc) unit = argv[++i];
    if (std::strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
      if (std::sscanf(argv[++i], "%dx%d", &width, &height) != 2 || width < 1 || height < 1) {
        std::fprintf(stderr, "--size wants WIDTHxHEIGHT, e.g. 466x466\n");
        return 2;
      }
    }
  }
  return live ? sim::runLive(look, paced, channel, unit, width, height) : sim::runSnapshots();
}
