#pragma once

#include <string_view>

namespace sim {

/// Write a contact sheet of PNG frames from a scripted drive to ./out.
int runSnapshots();

/// Stream raw RGB frames to stdout and read ECU commands from stdin. Meant to be driven
/// by tools/preview.py. `paced` runs in real time (one frame per 33 ms of simulated time);
/// unpaced runs flat out, which is what tools/check_renders.py wants (the frames are the
/// same either way: simulated time does not depend on the wall clock).
int runLive(std::string_view initialLook, bool paced = true);

}  // namespace sim
