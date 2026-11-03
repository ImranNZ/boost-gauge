#pragma once

namespace sim {

/// Write a contact sheet of PNG frames from a scripted drive to ./out.
int runSnapshots();

/// Stream raw RGB frames to stdout in real time and read ECU commands from
/// stdin. Meant to be driven by tools/preview.py.
int runLive();

}  // namespace sim
