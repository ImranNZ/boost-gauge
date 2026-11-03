// PC simulator: runs the real gauge stack against mocks, with no hardware.
//
//   pio run -e sim -t exec             stills: out/sheet.png, out/no_link.png
//   python tools/preview.py            live animation in your browser

#include <cstring>

#include "Modes.h"

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "--live") == 0) return sim::runLive();
  return sim::runSnapshots();
}
