#include <chrono>
#include <cstdio>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "Modes.h"
#include "Rig.h"
#include "SimCommands.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace sim {
namespace {

/// Lines typed/sent by the viewer, read on a helper thread.
class CommandQueue {
 public:
  void push(std::string line) {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back(std::move(line));
  }
  bool pop(std::string& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lines_.empty()) return false;
    out = std::move(lines_.front());
    lines_.pop_front();
    return true;
  }

 private:
  std::mutex mutex_;
  std::deque<std::string> lines_;
};

void useBinaryStdout() {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);  // no \n -> \r\n corruption of pixel data
#endif
}

}  // namespace

namespace {

/// One line from the viewer: an ECU command, a data-rate command, or a look, unit or channel
/// switch.
bool handleCommand(Rig& rig, const std::string& line) {
  std::string word;
  if (gauge::applyEcuCommand(rig.ecu, line)) return true;
  if (gauge::applyRateCommand(rig.link, line)) return true;
  if (gauge::parseLookCommand(line, word)) return rig.selectLook(word);
  if (gauge::parseUnitCommand(line, word)) return rig.selectUnit(word);
  if (gauge::parseChannelCommand(line, word)) return rig.selectChannel(word);
  return false;
}

}  // namespace

int runLive(std::string_view initialLook, bool paced, std::string_view channel,
            std::string_view unit, int width, int height) {
  useBinaryStdout();

  static CommandQueue commands;  // outlives the detached reader thread
  std::thread([] {
    std::string line;
    while (std::getline(std::cin, line)) commands.push(line);
  }).detach();

  Rig rig(width, height);
  if (!rig.selectChannel(channel)) {
    std::fprintf(stderr, "unknown channel '%.*s' (use boost, coolant, intake or voltage)\n",
                 static_cast<int>(channel.size()), channel.data());
  }
  if (!unit.empty() && !rig.selectUnit(unit)) {
    std::fprintf(stderr, "cannot show %s in '%.*s'\n", gauge::channelName(rig.channel),
                 static_cast<int>(unit.size()), unit.data());
  }
  if (!rig.selectLook(initialLook)) {
    std::fprintf(stderr,
                 "cannot use look '%.*s' (use retro, classic, minimal, motorsport or modern; "
                 "retro and modern need the boost channel)\n",
                 static_cast<int>(initialLook.size()), initialLook.data());
  }
  using Clock = std::chrono::steady_clock;
  auto nextFrame = Clock::now();

  for (;;) {
    std::string line;
    while (commands.pop(line)) {
      if (!handleCommand(rig, line)) std::fprintf(stderr, "ignored command: %s\n", line.c_str());
    }

    rig.step();

    const auto& pixels = rig.canvas.image().pixels();
    const bool ok = std::fwrite(pixels.data(), 1, pixels.size(), stdout) == pixels.size() &&
                    std::fflush(stdout) == 0;
    if (!ok) return 0;  // the viewer went away

    if (!paced) continue;
    nextFrame += std::chrono::milliseconds(Rig::kFrameMs);
    std::this_thread::sleep_until(nextFrame);
  }
}

}  // namespace sim
