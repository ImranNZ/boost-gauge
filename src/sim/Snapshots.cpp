#include <cstdio>
#include <filesystem>

#include "Image.h"
#include "Modes.h"
#include "Rig.h"

namespace sim {

int runSnapshots() {
  namespace fs = std::filesystem;
  fs::create_directories("out");

  Rig rig;

  // Seconds into the 20 s drive profile at which to capture a frame.
  constexpr float kCaptures[] = {1.0f, 4.0f, 6.0f, 9.0f, 10.5f, 13.0f, 16.0f, 18.0f};
  constexpr int kCount = sizeof(kCaptures) / sizeof(kCaptures[0]);
  constexpr int kColumns = 4;
  constexpr int kCell = Rig::kScreen;

  gauge::Image sheet(kCell * kColumns, kCell * ((kCount + kColumns - 1) / kColumns));

  int next = 0;
  while (next < kCount) {
    rig.step();
    if (static_cast<float>(rig.clock.nowMs()) / 1000.0f < kCaptures[next]) continue;

    char name[64];
    std::snprintf(name, sizeof(name), "out/frame_%04.1fs.png", kCaptures[next]);
    gauge::writePng(rig.canvas.image(), name);
    sheet.blit(rig.canvas.image(), (next % kColumns) * kCell, (next / kColumns) * kCell);
    ++next;
  }

  // And what a dead link looks like.
  rig.ecu.setOnline(false);
  for (int i = 0; i < 100; ++i) rig.step();
  gauge::writePng(rig.canvas.image(), "out/no_link.png");

  gauge::writePng(sheet, "out/sheet.png");
  std::printf("wrote %d frames + out/sheet.png + out/no_link.png\n", kCount);
  return 0;
}

}  // namespace sim
