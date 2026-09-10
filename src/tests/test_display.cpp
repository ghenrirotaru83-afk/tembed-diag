#include "app.h"
#include "display.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static const int MODE_COUNT = 4;
static const char* MODES[] = { "Colour bars", "Gradient", "Text / fonts", "Backlight PWM" };

static int s_mode = 0;
static lv_obj_t* s_value = nullptr;
static lv_obj_t* s_bar = nullptr;
static int s_bl = 15;
static int s_dir = 1;
static uint32_t s_next = 0;

static void rebuild();

static lv_obj_t* box(lv_obj_t* parent, int w, int h, uint32_t color) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, col(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_set_style_radius(o, 3, 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}

static void build_bars(lv_obj_t* c) {
  auto row = [&](uint32_t a, uint32_t b, uint32_t d) {
    lv_obj_t* r = lv_obj_create(c);
    lv_obj_set_size(r, lv_pct(100), 70);
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    box(r, 44, 64, a);
    box(r, 44, 64, b);
    box(r, 44, 64, d);
  };
  row(0xFF0000, 0x00FF00, 0x0000FF);
  row(0xFFFFFF, 0x000000, 0x808080);
}

static void build_gradient(lv_obj_t* c) {
  lv_obj_t* g = lv_obj_create(c);
  lv_obj_set_size(g, lv_pct(100), 90);
  lv_obj_set_style_radius(g, 8, 0);
  lv_obj_set_style_border_width(g, 0, 0);
  lv_obj_set_style_bg_opa(g, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(g, col(0xFF3B30), 0);
  lv_obj_set_style_bg_grad_color(g, col(0x4CC2FF), 0);
  lv_obj_set_style_bg_grad_dir(g, LV_GRAD_DIR_HOR, 0);
  lv_obj_clear_flag(g, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* v = lv_obj_create(c);
  lv_obj_set_size(v, lv_pct(100), 90);
  lv_obj_set_style_radius(v, 8, 0);
  lv_obj_set_style_border_width(v, 0, 0);
  lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(v, col(0x38D39F), 0);
  lv_obj_set_style_bg_grad_color(v, col(0x0E1116), 0);
  lv_obj_set_style_bg_grad_dir(v, LV_GRAD_DIR_VER, 0);
  lv_obj_clear_flag(v, LV_OBJ_FLAG_SCROLLABLE);
}

static void build_text(lv_obj_t* c) {
  label(c, "AaBbCc 0123456789", 12, C_TEXT);
  label(c, "Medium 16px", 16, C_PRIMARY);
  label(c, "Large 20px", 20, C_PASS);
  label(c, "Header 24px", 24, C_WARN);
  label(c, "!@#$%^&*()[]{}", 14, C_MUTED);
}

static void build_backlight(lv_obj_t* c) {
  label(c, "PWM sweep on GPIO21", 14, C_TEXT);
  s_value = label(c, "15%", 28, C_PRIMARY, true);
  s_bar = ui::bar(c, DISPLAY_WIDTH - 60, 16, C_PRIMARY);
  lv_bar_set_value(s_bar, s_bl, LV_ANIM_OFF);
  label(c, "Auto-sweeps 0-100%. Press to toggle direction.", 11, C_MUTED);
}

static void rebuild() {
  lv_obj_t* c = app_content();
  if (!c) return;
  lv_obj_clean(c);
  s_value = nullptr;
  s_bar = nullptr;
  label(c, MODES[s_mode], 16, C_PRIMARY, true);
  switch (s_mode) {
    case 0: build_bars(c); break;
    case 1: build_gradient(c); break;
    case 2: build_text(c); break;
    case 3: build_backlight(c); break;
  }
  char buf[48];
  snprintf(buf, sizeof(buf), "mode %d/%d", s_mode + 1, MODE_COUNT);
  app_set_status(ST_INFO, "%s", buf);
}

static void enter() { s_mode = 0; display::set_backlight(80); }
static void build(lv_obj_t*) { rebuild(); }
static void leave() { display::set_backlight(80); }

static void enc(int delta, bool pressed) {
  if (delta) { s_mode = (s_mode + (delta > 0 ? 1 : -1) + MODE_COUNT) % MODE_COUNT; rebuild(); }
  else if (pressed) { s_mode = (s_mode + 1) % MODE_COUNT; rebuild(); }
}

static void tick() {
  if (s_mode != 3) return;
  uint32_t now = millis();
  if (now < s_next) return;
  s_next = now + 40;
  s_bl += s_dir * 3;
  if (s_bl >= 100) { s_bl = 100; s_dir = -1; }
  if (s_bl <= 0) { s_bl = 0; s_dir = 1; }
  display::set_backlight(s_bl);
  if (s_bar) lv_bar_set_value(s_bar, s_bl, LV_ANIM_OFF);
  if (s_value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", s_bl);
    lv_label_set_text(s_value, buf);
  }
}

extern const TestSpec test_display = {
  "display", "Display test", build, enter, tick, leave, enc
};
