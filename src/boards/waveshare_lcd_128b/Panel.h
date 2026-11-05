#pragma once

#include <LovyanGFX.hpp>

#include "boards/Board.h"

namespace board {

/// LovyanGFX board description: GC9A01 on SPI2 with PWM backlight.
class Panel : public lgfx::LGFX_Device {
 public:
  Panel() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = kLcdSpiHz;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = kLcdSclk;
      cfg.pin_mosi = kLcdMosi;
      cfg.pin_miso = -1;
      cfg.pin_dc = kLcdDc;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = kLcdCs;
      cfg.pin_rst = kLcdRst;
      cfg.pin_busy = -1;
      cfg.panel_width = kWidth;
      cfg.panel_height = kHeight;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.readable = false;
      cfg.invert = true;  // GC9A01 modules are normally wired inverted
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = light_.config();
      cfg.pin_bl = kLcdBacklight;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_.config(cfg);
      panel_.setLight(&light_);
    }
    setPanel(&panel_);
  }

 private:
  lgfx::Panel_GC9A01 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
};

}  // namespace board
