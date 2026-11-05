#pragma once

// The LovyanGFX device for the selected board (see Board.h): `board::Panel`, a
// default-constructible lgfx::LGFX_Device already configured with the board's bus, panel and
// backlight.

#if defined(BOARD_WAVESHARE_LCD_128B)
#include "boards/waveshare_lcd_128b/Panel.h"
#else
#error "No board selected: add -DBOARD_<NAME> to the environment (see src/boards/Board.h)"
#endif
