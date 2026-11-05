#pragma once

// The board the firmware is built for: its screen, pins and memory layout. One header per board
// under src/boards/<name>/, picked by a -DBOARD_<NAME> build flag in platformio.ini. A new board is
// a new folder (Board.h with the constants below, Panel.h with its display driver) and a flag; no
// other file changes. Deployment choices (what to show, the car link) stay in config.h.
//
// Every Board.h defines, in namespace `board`:
//   kName            a human-readable name
//   kWidth, kHeight  the screen in pixels (the looks are scaled to it; see ScaledSurface)
//   kFrameInPsram    true if the frame buffer is too big for internal RAM
//   kCanTx, kCanRx   free header pins for the CAN transceiver (TRANSPORT_CAN)
//   plus whatever pins and clocks its Panel.h needs.

#if defined(BOARD_WAVESHARE_LCD_128B)
#include "boards/waveshare_lcd_128b/Board.h"
#else
#error "No board selected: add -DBOARD_<NAME> to the environment (see src/boards/Board.h)"
#endif
