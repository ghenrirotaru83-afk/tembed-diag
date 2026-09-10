#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <cstdio>

using namespace ui;

static Adafruit_NeoPixel s_strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

static const int MODE_COUNT = 4;
static const char* MODES[] = { "Walk", "Solid cycle", "Rainbow", "Brightness" };

static int s_mode = 0;
static int s_step = 0;
static int s_brightness = 60;
static uint32_t s_next = 0;
static lv_obj_t* s_info = nullptr;

static uint32_t wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85)  return s_strip.Color(255 - pos * 3, 0, pos * 3);
  if (pos < 170) { pos -= 85; return s_strip.Color(0, pos * 3, 255 - pos * 3); }
  pos -= 170;
  return s_strip.Color(pos * 3, 255 - pos * 3, 0);
}

static void apply_mode() {
  s_strip.setBrightness(s_brightness);
  s_strip.clear();
  switch (s_mode) {
    case 0:  // single lit LED walking
      s_strip.setPixelColor(s_step % WS2812_NUM_LEDS, s_strip.Color(0, 160, 255));
      break;
    case 1: {  // all LEDs, hue cycling
      uint32_t c = wheel((s_step * 4) & 0xFF);
      for (int i = 0; i < WS2812_NUM_LEDS; i++) s_strip.setPixelColor(i, c);
      break;
    }
    case 2:  // rainbow chase
      for (int i = 0; i < WS2812_NUM_LEDS; i++)
        s_strip.setPixelColor(i, wheel((i * 32 + s_step * 8) & 0xFF));
      break;
    case 3:  // brightness sweep on a fixed colour
      for (int i = 0; i < WS2812_NUM_LEDS; i++) s_strip.setPixelColor(i, s_strip.Color(255, 120, 0));
      break;
  }
  s_strip.show();
}

static void build(lv_obj_t* c) {
  label(c, MODES[s_mode], 16, C_PRIMARY, true);
  s_info = label(c, "", 12, C_TEXT);
  label(c, "8x WS2812 on GPIO14. Press to change mode.", 11, C_MUTED);
}

static void enter() {
  s_strip.begin();
  s_mode = 0;
  s_step = 0;
  s_brightness = 60;
  apply_mode();
}

static void tick() {
  uint32_t now = millis();
  if (now < s_next) return;
  s_next = now + (s_mode == 3 ? 30 : 90);
  s_step++;
  if (s_mode == 3) {
    static int dir = 1;
    s_brightness += dir * 5;
    if (s_brightness >= 255) { s_brightness = 255; dir = -1; }
    if (s_brightness <= 10)  { s_brightness = 10;  dir = 1; }
  }
  apply_mode();
  if (s_info) {
    char buf[48];
    snprintf(buf, sizeof(buf), "mode: %s   step %d   bri %d",
             MODES[s_mode], s_step, s_brightness);
    lv_label_set_text(s_info, buf);
  }
  app_set_status(ST_RUN, "%s", MODES[s_mode]);
}

static void leave() {
  s_strip.clear();
  s_strip.show();
}

static void enc(int delta, bool pressed) {
  if (delta) {
    s_mode = (s_mode + (delta > 0 ? 1 : -1) + MODE_COUNT) % MODE_COUNT;
    if (app_content()) {
      lv_obj_clean(app_content());
      build(app_content());
    }
    apply_mode();
  } else if (pressed) {
    s_mode = (s_mode + 1) % MODE_COUNT;
    if (app_content()) {
      lv_obj_clean(app_content());
      build(app_content());
    }
    apply_mode();
  }
}

extern const TestSpec test_leds = {
  "leds", "RGB LEDs (8x WS2812)", build, enter, tick, leave, enc
};
