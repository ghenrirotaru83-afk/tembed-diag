#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

// 0 = drive LOW, 1 = drive HIGH, 2 = high-Z input
static int s_state[BOARD_EXT_GPIO_COUNT] = { 2, 2, 2, 2 };
static int s_sel = 0;
static lv_obj_t* s_rows[BOARD_EXT_GPIO_COUNT] = { nullptr };
static lv_obj_t* s_vals[BOARD_EXT_GPIO_COUNT] = { nullptr };
static lv_obj_t* s_hint = nullptr;

static void apply_pin(int i) {
  int pin = BOARD_EXT_GPIOS[i];
  if (s_state[i] == 2) {
    pinMode(pin, INPUT_PULLUP);
  } else {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, s_state[i] == 1 ? HIGH : LOW);
  }
}

static void refresh_style() {
  for (int i = 0; i < BOARD_EXT_GPIO_COUNT; i++) {
    if (s_rows[i]) style_row(s_rows[i], i == s_sel);
  }
  if (s_hint) {
    char buf[64];
    snprintf(buf, sizeof(buf), "pin GPIO%d  ·  press: LOW > HIGH > input", BOARD_EXT_GPIOS[s_sel]);
    lv_label_set_text(s_hint, buf);
  }
}

static void make_pin_row(lv_obj_t* list, int pin, lv_obj_t** row_out, lv_obj_t** val_out) {
  lv_obj_t* row = lv_obj_create(list);
  lv_obj_set_size(row, lv_pct(100), 38);
  lv_obj_set_style_radius(row, 9, 0);
  lv_obj_set_style_bg_color(row, col(C_SURFACE), 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(row, col(C_BORDER), 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 8, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* name = lv_label_create(row);
  char buf[16];
  snprintf(buf, sizeof(buf), "GPIO%d", pin);
  lv_label_set_text(name, buf);
  lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(name, col(C_TEXT), 0);
  lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t* val = lv_label_create(row);
  lv_label_set_text(val, "--");
  lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
  lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, 0);

  *row_out = row;
  *val_out = val;
}

static void build(lv_obj_t* c) {
  lv_obj_t* list = lv_obj_create(c);
  lv_obj_set_size(list, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 0, 0);
  lv_obj_set_style_pad_row(list, 4, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

  for (int i = 0; i < BOARD_EXT_GPIO_COUNT; i++) {
    make_pin_row(list, BOARD_EXT_GPIOS[i], &s_rows[i], &s_vals[i]);
  }
  s_hint = label(c, "", 11, C_MUTED);
  label(c, "GPIO8/18 are the shared I2C bus - driving them disrupts the PN532 "
           "and battery gauge. Use a multimeter or LED for continuity.", 11, C_MUTED);
  refresh_style();
}

static void enter() {
  s_sel = 0;
  for (int i = 0; i < BOARD_EXT_GPIO_COUNT; i++) {
    s_state[i] = 2;
    apply_pin(i);
  }
  app_set_status(ST_INFO, "select a pin");
}

static void tick() {
  static uint32_t next = 0;
  uint32_t now = millis();
  if (now < next) return;
  next = now + 200;
  char buf[32];
  for (int i = 0; i < BOARD_EXT_GPIO_COUNT; i++) {
    if (!s_vals[i]) continue;
    int pin = BOARD_EXT_GPIOS[i];
    if (s_state[i] == 2) {
      int lvl = digitalRead(pin);
      snprintf(buf, sizeof(buf), "in %s", lvl ? "HIGH" : "LOW");
      lv_label_set_text(s_vals[i], buf);
      value_label(s_vals[i], lvl ? C_PASS : C_MUTED);
    } else {
      snprintf(buf, sizeof(buf), "out %s", s_state[i] == 1 ? "HIGH" : "LOW");
      lv_label_set_text(s_vals[i], buf);
      value_label(s_vals[i], C_PRIMARY);
    }
  }
}

static void leave() {
  for (int i = 0; i < BOARD_EXT_GPIO_COUNT; i++) {
    pinMode(BOARD_EXT_GPIOS[i], INPUT);
  }
}

static void enc(int delta, bool pressed) {
  if (delta) {
    s_sel = (s_sel + (delta > 0 ? 1 : -1) + BOARD_EXT_GPIO_COUNT) % BOARD_EXT_GPIO_COUNT;
    refresh_style();
  }
  if (pressed) {
    s_state[s_sel] = (s_state[s_sel] + 1) % 3;
    apply_pin(s_sel);
    refresh_style();
    const char* mode = s_state[s_sel] == 0 ? "LOW" : (s_state[s_sel] == 1 ? "HIGH" : "input");
    app_set_status(s_state[s_sel] == 2 ? ST_INFO : ST_PASS,
                   "GPIO%d %s", BOARD_EXT_GPIOS[s_sel], mode);
  }
}

extern const TestSpec test_gpio = {
  "gpio", "External GPIO", build, enter, tick, leave, enc
};
