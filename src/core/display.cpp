#include "display.h"
#include "board_pins.h"
#include "spi_bus.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace display {

static TFT_eSPI tft;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static const int BL_CHANNEL = 7;
static int bl_percent = 80;

static void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  spi_deselect_all();
  const uint32_t w = area->x2 - area->x1 + 1;
  const uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  // Third arg true = byte swap for the ST7789 (LV_COLOR_16_SWAP 0).
  tft.pushColors((uint16_t*)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(drv);
}

void set_backlight(int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  bl_percent = percent;
  // 8-bit resolution, 5 kHz PWM.
  ledcWrite(BL_CHANNEL, (uint32_t)(percent * 255 / 100));
}

int backlight() { return bl_percent; }

void init() {
  // Backlight PWM before anything draws so we can fade in later.
  ledcSetup(BL_CHANNEL, 5000, 8);
  ledcAttachPin(DISPLAY_BL, BL_CHANNEL);
  set_backlight(0);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  lv_init();

  // Partial draw buffer (~50 lines) in PSRAM: live labels repaint only what
  // changed instead of a full 170x320 frame.
  static lv_color_t* buf1 = (lv_color_t*)ps_malloc(DISPLAY_WIDTH * 50 * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, DISPLAY_WIDTH * 50);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = DISPLAY_WIDTH;
  disp_drv.ver_res = DISPLAY_HEIGHT;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.full_refresh = 0;
  lv_disp_drv_register(&disp_drv);

  // Draw the first frame, then fade the panel up.
  lv_timer_handler();
  set_backlight(80);
}

}  // namespace display
