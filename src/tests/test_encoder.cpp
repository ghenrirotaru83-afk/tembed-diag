#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static int s_position = 0;
static int s_direction = 0;   // +1 CW, -1 CCW
static int s_presses = 0;
static int s_bounces = 0;     // raw edges seen while pressed (debounce sanity)
static lv_obj_t* s_pos = nullptr;
static lv_obj_t* s_dir = nullptr;
static lv_obj_t* s_btn = nullptr;
static lv_obj_t* s_press = nullptr;
static lv_obj_t* s_bounce = nullptr;
static bool s_was_pressed = false;
static int s_raw_edges = 0;
static uint32_t s_last_edge = 0;

static void build(lv_obj_t* c) {
  s_pos    = kv_row(c, "Rotation count");
  s_dir    = kv_row(c, "Last direction");
  s_btn    = kv_row(c, "Button level (raw)");
  s_press  = kv_row(c, "Short presses");
  s_bounce = kv_row(c, "Raw edges (bounce)");
  label(c, "Rotate and press the encoder. Each detent should add exactly +1 or -1.",
        11, C_MUTED);
  app_set_status(ST_INFO, "live");
}

static void enter() {
  s_position = 0;
  s_direction = 0;
  s_presses = 0;
  s_bounces = 0;
}

static void tick() {
  bool pressed = (digitalRead(ENCODER_KEY) == LOW);
  if (pressed != s_was_pressed) {
    uint32_t now = millis();
    if (now - s_last_edge > 2) {
      s_raw_edges++;
      s_last_edge = now;
    }
    s_was_pressed = pressed;
  }
  bool raw = (digitalRead(ENCODER_KEY) == LOW);

  char buf[32];
  if (s_pos) { snprintf(buf, sizeof(buf), "%d", s_position); lv_label_set_text(s_pos, buf); }
  if (s_dir) {
    const char* d = s_direction > 0 ? "CW (+)" : (s_direction < 0 ? "CCW (-)" : "--");
    lv_label_set_text(s_dir, d);
    value_label(s_dir, s_direction == 0 ? C_TEXT : C_PRIMARY);
  }
  if (s_btn) {
    lv_label_set_text(s_btn, raw ? "LOW (pressed)" : "HIGH (released)");
    value_label(s_btn, raw ? C_PASS : C_MUTED);
  }
  if (s_press) { snprintf(buf, sizeof(buf), "%d", s_presses); lv_label_set_text(s_press, buf); }
  if (s_bounce) {
    snprintf(buf, sizeof(buf), "%d", s_raw_edges);
    lv_label_set_text(s_bounce, buf);
    value_label(s_bounce, s_raw_edges <= s_presses * 2 + 2 ? C_PASS : C_WARN);
  }
}

static void enc(int delta, bool pressed) {
  if (delta) {
    s_position += delta;
    s_direction = delta > 0 ? 1 : -1;
    app_set_status(ST_RUN, "rotation %+d", delta);
  }
  if (pressed) {
    s_presses++;
    app_set_status(ST_PASS, "press #%d", s_presses);
  }
}

extern const TestSpec test_encoder = {
  "encoder", "Rotary encoder", build, enter, tick, nullptr, enc
};
