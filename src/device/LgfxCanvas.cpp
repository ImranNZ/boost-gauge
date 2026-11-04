#include "device/LgfxCanvas.h"

#include "render/ArcCoverage.h"
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
      cfg.freq_write = config::kLcdSpiHz;
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

LgfxCanvas::LgfxCanvas() : panel_(new Panel()), frame_(panel_), dial_(panel_) {}

bool LgfxCanvas::begin() {
  panel_->init();
  panel_->setBrightness(200);

  frame_.setColorDepth(16);
  frame_.setPsram(false);  // internal RAM: faster to push than PSRAM
  if (frame_.createSprite(config::kLcdSize, config::kLcdSize) == nullptr) return false;

  // Optional: without it restoreBackground() reports false and the view simply
  // redraws the dial every frame (slower, but correct).
  dial_.setColorDepth(16);
  dial_.setPsram(true);
  hasDial_ = false;
  dialAllocated_ = dial_.createSprite(config::kLcdSize, config::kLcdSize) != nullptr;
  return true;
}

int LgfxCanvas::width() const { return config::kLcdSize; }
int LgfxCanvas::height() const { return config::kLcdSize; }

void LgfxCanvas::clear(gauge::Color color) { frame_.fillScreen(rgb(color)); }

void LgfxCanvas::fillCircle(int cx, int cy, int radius, gauge::Color color) {
  frame_.fillSmoothCircle(cx, cy, radius, rgb(color));
}

// LovyanGFX has no anti-aliased arc, so blend the ring edges ourselves using the
// shared coverage maths. Only used for the dial, which is drawn once and cached.
void LgfxCanvas::fillArc(int cx, int cy, int innerRadius, int outerRadius, float startDeg,
                         float endDeg, gauge::Color color) {
  const float start = gauge::wrapDegrees(startDeg);
  const float sweep = gauge::wrapDegrees(endDeg - startDeg);
  const int reach = outerRadius + 1;

  for (int y = cy - reach; y <= cy + reach; ++y) {
    for (int x = cx - reach; x <= cx + reach; ++x) {
      const float coverage = gauge::arcCoverage(static_cast<float>(x - cx),
                                                static_cast<float>(y - cy),
                                                static_cast<float>(innerRadius),
                                                static_cast<float>(outerRadius), start, sweep);
      if (coverage <= 0.0f) continue;
      if (coverage >= 1.0f) {
        frame_.drawPixel(x, y, rgb(color));
        continue;
      }
      const int alpha = static_cast<int>(coverage * 255.0f + 0.5f);
      frame_.drawPixel(x, y, rgb(gauge::blend(readColor(x, y), color, alpha)));
    }
  }
}

void LgfxCanvas::drawWedge(float x0, float y0, float x1, float y1, float r0, float r1,
                           gauge::Color color) {
  frame_.drawWedgeLine(x0, y0, x1, y1, r0, r1, rgb(color));
}

void LgfxCanvas::drawLine(int x0, int y0, int x1, int y1, int thickness,
                          gauge::Color color) {
  const float radius = static_cast<float>(thickness < 1 ? 1 : thickness) * 0.5f;
  frame_.drawWideLine(x0, y0, x1, y1, radius, rgb(color));
}

void LgfxCanvas::drawImage(const gauge::ImageView& image, int x, int y) {
  if (image.rgb565 == nullptr) return;

  if (image.alpha == nullptr) {
    // Opaque (the dial): one bulk copy straight from flash. rgb565_t is the plain
    // 16-bit RGB565 layout, which is what the generated arrays hold.
    frame_.pushImage(x, y, image.width, image.height,
                     reinterpret_cast<const lgfx::rgb565_t*>(image.rgb565));
    return;
  }

  // With alpha (a badge): blend pixel by pixel. It is small and drawn once.
  for (int iy = 0; iy < image.height; ++iy) {
    for (int ix = 0; ix < image.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= frame_.width() || py >= frame_.height()) continue;

      const size_t index = static_cast<size_t>(iy) * static_cast<size_t>(image.width) +
                           static_cast<size_t>(ix);
      const int alpha = image.alpha[index];
      if (alpha == 0) continue;
      const gauge::Color src = gauge::rgb565ToColor(image.rgb565[index]);
      if (alpha == 255) {
        frame_.drawPixel(px, py, rgb(src));
        continue;
      }
      frame_.drawPixel(px, py, rgb(gauge::blend(readColor(px, py), src, alpha)));
    }
  }
}

void LgfxCanvas::drawMask(const gauge::MaskView& mask, int x, int y, gauge::Color color) {
  if (mask.alpha == nullptr) return;

  for (int iy = 0; iy < mask.height; ++iy) {
    for (int ix = 0; ix < mask.width; ++ix) {
      const int px = x + ix;
      const int py = y + iy;
      if (px < 0 || py < 0 || px >= frame_.width() || py >= frame_.height()) continue;

      const int alpha = mask.alpha[static_cast<size_t>(iy) * static_cast<size_t>(mask.width) +
                                   static_cast<size_t>(ix)];
      if (alpha == 0) continue;
      frame_.drawPixel(px, py,
                       rgb(alpha == 255 ? color : gauge::blend(readColor(px, py), color, alpha)));
    }
  }
}

void LgfxCanvas::saveBackground() {
  if (!dialAllocated_) return;
  frame_.pushSprite(&dial_, 0, 0);
  hasDial_ = true;
}

bool LgfxCanvas::restoreBackground() {
  if (!hasDial_) return false;
  dial_.pushSprite(&frame_, 0, 0);
  return true;
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
