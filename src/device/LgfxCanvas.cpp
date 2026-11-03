#include "device/LgfxCanvas.h"

#include "config.h"

namespace device {

/// LovyanGFX board description: GC9A01 on SPI2 with PWM backlight.
class LgfxCanvas::Panel : public lgfx::LGFX_Device {
 public:
  Panel() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = config::kLcdSclk;
      cfg.pin_mosi = config::kLcdMosi;
      cfg.pin_miso = -1;
      cfg.pin_dc = config::kLcdDc;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = config::kLcdCs;
      cfg.pin_rst = config::kLcdRst;
      cfg.pin_busy = -1;
      cfg.panel_width = config::kLcdSize;
      cfg.panel_height = config::kLcdSize;
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
      cfg.pin_bl = config::kLcdBacklight;
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

LgfxCanvas::LgfxCanvas() : panel_(new Panel()), frame_(panel_) {}

bool LgfxCanvas::begin() {
  panel_->init();
  panel_->setBrightness(200);
  frame_.setColorDepth(16);
  frame_.setPsram(false);  // internal RAM: faster to push than PSRAM
  return frame_.createSprite(config::kLcdSize, config::kLcdSize) != nullptr;
}

int LgfxCanvas::width() const { return config::kLcdSize; }
int LgfxCanvas::height() const { return config::kLcdSize; }

void LgfxCanvas::clear(gauge::Color color) { frame_.fillScreen(rgb(color)); }

void LgfxCanvas::fillCircle(int cx, int cy, int radius, gauge::Color color) {
  frame_.fillCircle(cx, cy, radius, rgb(color));
}

void LgfxCanvas::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                         float endDeg, gauge::Color color) {
  // LovyanGFX arcs: 0 deg = 3 o'clock, clockwise; same convention as ICanvas.
  frame_.fillArc(cx, cy, innerRadius, outerRadius, startDeg, endDeg, rgb(color));
}

void LgfxCanvas::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
                              gauge::Color color) {
  frame_.fillTriangle(x0, y0, x1, y1, x2, y2, rgb(color));
}

void LgfxCanvas::drawLine(int x0, int y0, int x1, int y1, int thickness,
                          gauge::Color color) {
  if (thickness <= 1) {
    frame_.drawLine(x0, y0, x1, y1, rgb(color));
  } else {
    frame_.drawWideLine(x0, y0, x1, y1, static_cast<float>(thickness) * 0.5f, rgb(color));
  }
}

void LgfxCanvas::drawText(int x, int y, const char* text, int scale, gauge::TextAlign align,
                          gauge::Color color) {
  // Font0 is the 6x8 cell (5x7 glyph) font that SoftCanvas mimics.
  frame_.setFont(&fonts::Font0);
  frame_.setTextSize(scale);
  frame_.setTextColor(rgb(color));
  switch (align) {
    case gauge::TextAlign::Left:
      frame_.setTextDatum(lgfx::top_left);
      break;
    case gauge::TextAlign::Center:
      frame_.setTextDatum(lgfx::top_center);
      break;
    case gauge::TextAlign::Right:
      frame_.setTextDatum(lgfx::top_right);
      break;
  }
  frame_.drawString(text, x, y);
}

void LgfxCanvas::present() { frame_.pushSprite(panel_, 0, 0); }

}  // namespace device
