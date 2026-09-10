#include "app.h"
#include "board_pins.h"
#include "encoder.h"
#include "display.h"
#include "spi_bus.h"
#include "theme.h"
#include "tests.h"

#include <Arduino.h>
#include <cstdio>
#include <cstdarg>

using namespace ui;

// ── categories ───────────────────────────────────────────────────────────────

static const TestSpec* T_DISPLAY[] = { &test_display };
static const TestSpec* T_ENCODER[] = { &test_encoder };
static const TestSpec* T_LEDS[]    = { &test_leds };
static const TestSpec* T_IR[]      = { &test_ir };
static const TestSpec* T_SUBGHZ[]  = { &test_cc1101 };
static const TestSpec* T_NFC[]     = { &test_nfc };
static const TestSpec* T_SD[]      = { &test_sd };
static const TestSpec* T_AUDIO[]   = { &test_mic, &test_speaker };
static const TestSpec* T_BATTERY[] = { &test_battery };
static const TestSpec* T_GPIO[]    = { &test_gpio };
static const TestSpec* T_RADIO[]   = { &test_radio };
static const TestSpec* T_SYS[]     = { &test_sysinfo };

struct Category {
  const char* name;
  const char* hint;
  const TestSpec* const* tests;
  int count;
};

static const Category CATS[] = {
  { "Display",          "colour / gradient / backlight", T_DISPLAY, 1 },
  { "Rotary encoder",   "rotation + button",             T_ENCODER, 1 },
  { "RGB LEDs",         "8x WS2812",                     T_LEDS,    1 },
  { "Infrared",         "TX + loop-back RX",             T_IR,      1 },
  { "Sub-GHz CC1101",   "SPI + RSSI + carrier",          T_SUBGHZ,  1 },
  { "NFC PN532",        "I2C tag detect",                T_NFC,     1 },
  { "microSD",          "mount + read/write",            T_SD,      1 },
  { "Audio",            "microphone + speaker",          T_AUDIO,   2 },
  { "Battery",          "BQ27220 + BQ25896",             T_BATTERY, 1 },
  { "External GPIO",    "toggle / read pins",            T_GPIO,    1 },
  { "Wireless",         "WiFi + BLE scan",               T_RADIO,   1 },
  { "System info",      "chip / memory / reset",         T_SYS,     1 },
};
static const int CAT_COUNT = (int)(sizeof(CATS) / sizeof(CATS[0]));

// ── state ────────────────────────────────────────────────────────────────────

enum Level { LVL_HOME, LVL_CAT, LVL_TEST };

static Level g_level = LVL_HOME;
static lv_obj_t* g_home = nullptr;
static lv_obj_t* g_cat = nullptr;
static lv_obj_t* g_test = nullptr;

static lv_obj_t* g_content = nullptr;
static lv_obj_t* g_status_dot = nullptr;
static lv_obj_t* g_status_lbl = nullptr;
static lv_obj_t* g_title_lbl = nullptr;
static lv_obj_t* g_hint_lbl = nullptr;

static const TestSpec* g_active = nullptr;
static int g_home_sel = 0;
static int g_cat_sel = 0;
static int g_cat_index = 0;

static lv_obj_t* g_home_rows[CAT_COUNT];
static lv_obj_t* g_cat_rows[4];

static int clamp_index(int v, int n) {
  if (v < 0) return 0;
  if (v >= n) return n - 1;
  return v;
}

static void highlight(lv_obj_t** rows, int count, int sel) {
  for (int i = 0; i < count; i++) {
    if (!rows[i]) continue;
    ui::style_row(rows[i], i == sel);
  }
  if (sel >= 0 && sel < count && rows[sel]) {
    lv_obj_scroll_to_view(rows[sel], LV_ANIM_ON);
  }
}

static lv_obj_t* make_list(lv_obj_t* parent, int top, int bottom_reserve) {
  lv_obj_t* list = lv_obj_create(parent);
  lv_obj_set_size(list, DISPLAY_WIDTH, DISPLAY_HEIGHT - top - bottom_reserve);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, top);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 8, 0);
  lv_obj_set_style_pad_row(list, 5, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
  return list;
}

static lv_obj_t* make_menu_row(lv_obj_t* list, const char* title, const char* hint) {
  lv_obj_t* row = lv_obj_create(list);
  lv_obj_set_size(row, lv_pct(100), hint ? 46 : 38);
  lv_obj_set_style_radius(row, 9, 0);
  lv_obj_set_style_bg_color(row, col(C_SURFACE), 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(row, col(C_BORDER), 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 8, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = lv_label_create(row);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(t, col(C_TEXT), 0);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);

  if (hint) {
    lv_obj_t* s = lv_label_create(row);
    lv_label_set_text(s, hint);
    lv_obj_set_style_text_font(s, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s, col(C_MUTED), 0);
    lv_obj_align(s, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  }
  return row;
}

static lv_obj_t* footer_hint(lv_obj_t* scr, const char* text) {
  lv_obj_t* lbl = ui::label(scr, text, 11, C_MUTED);
  lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -5);
  return lbl;
}

// ── screens ──────────────────────────────────────────────────────────────────

static void build_home() {
  g_home = ui::screen();
  ui::header(g_home, BOARD_NAME);
  lv_obj_t* sub = ui::label(g_home, "Hardware diagnostics", 12, C_MUTED);
  lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 12, 40);

  lv_obj_t* list = make_list(g_home, 58, 24);
  for (int i = 0; i < CAT_COUNT; i++) {
    g_home_rows[i] = make_menu_row(list, CATS[i].name, CATS[i].hint);
  }
  footer_hint(g_home, "turn: move   press: open   hold: back");
  highlight(g_home_rows, CAT_COUNT, g_home_sel);
}

static void open_category(int index) {
  if (g_cat) { lv_obj_del(g_cat); g_cat = nullptr; }
  g_cat_index = index;
  g_cat_sel = 0;

  g_cat = ui::screen();
  ui::header(g_cat, CATS[index].name);
  lv_obj_t* list = make_list(g_cat, 38, 24);
  for (int i = 0; i < CATS[index].count; i++) {
    g_cat_rows[i] = make_menu_row(list, CATS[index].tests[i]->name, nullptr);
  }
  footer_hint(g_cat, "turn: move   press: run   hold: back");
  highlight(g_cat_rows, CATS[index].count, g_cat_sel);

  g_level = LVL_CAT;
  lv_scr_load_anim(g_cat, LV_SCR_LOAD_ANIM_MOVE_LEFT, 160, 0, false);
}

static void open_test(const TestSpec* spec) {
  if (!spec) return;
  g_active = spec;

  g_test = ui::screen();
  g_title_lbl = ui::header(g_test, spec->name);

  g_hint_lbl = ui::label(g_test, "", 11, C_MUTED);
  lv_obj_align(g_hint_lbl, LV_ALIGN_TOP_LEFT, 12, 38);

  g_content = lv_obj_create(g_test);
  lv_obj_set_size(g_content, DISPLAY_WIDTH, DISPLAY_HEIGHT - 34 - 28 - 20);
  lv_obj_align(g_content, LV_ALIGN_TOP_MID, 0, 54);
  lv_obj_set_style_bg_opa(g_content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_content, 0, 0);
  lv_obj_set_style_pad_all(g_content, 10, 0);
  lv_obj_set_style_pad_row(g_content, 6, 0);
  lv_obj_set_flex_flow(g_content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_scrollbar_mode(g_content, LV_SCROLLBAR_MODE_AUTO);

  // Status footer.
  lv_obj_t* foot = lv_obj_create(g_test);
  lv_obj_set_size(foot, DISPLAY_WIDTH, 24);
  lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(foot, col(C_SURFACE), 0);
  lv_obj_set_style_radius(foot, 0, 0);
  lv_obj_set_style_border_width(foot, 0, 0);
  lv_obj_set_style_pad_all(foot, 0, 0);
  lv_obj_clear_flag(foot, LV_OBJ_FLAG_SCROLLABLE);

  g_status_dot = lv_obj_create(foot);
  lv_obj_set_size(g_status_dot, 8, 8);
  lv_obj_set_style_radius(g_status_dot, 4, 0);
  lv_obj_set_style_border_width(g_status_dot, 0, 0);
  lv_obj_set_style_bg_color(g_status_dot, col(C_MUTED), 0);
  lv_obj_align(g_status_dot, LV_ALIGN_LEFT_MID, 10, 0);

  g_status_lbl = ui::label(foot, "ready", 12, C_TEXT);
  lv_obj_align(g_status_lbl, LV_ALIGN_LEFT_MID, 24, 0);
  lv_obj_set_width(g_status_lbl, DISPLAY_WIDTH - 34);
  lv_label_set_long_mode(g_status_lbl, LV_LABEL_LONG_DOT);

  g_level = LVL_TEST;
  lv_scr_load_anim(g_test, LV_SCR_LOAD_ANIM_MOVE_LEFT, 160, 0, false);

  if (spec->enter) spec->enter();
  if (spec->build) spec->build(g_content);
  app_set_status(ST_IDLE, "ready");
}

void app_back() {
  if (g_level == LVL_TEST) {
    if (g_active && g_active->leave) g_active->leave();
    g_active = nullptr;
    g_content = nullptr;
    if (g_test) { lv_obj_del(g_test); g_test = nullptr; }
    if (g_cat) {
      g_level = LVL_CAT;
      lv_scr_load_anim(g_cat, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 160, 0, false);
    } else {
      g_level = LVL_HOME;
      lv_scr_load_anim(g_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 160, 0, false);
    }
  } else if (g_level == LVL_CAT) {
    if (g_cat) { lv_obj_del(g_cat); g_cat = nullptr; }
    g_level = LVL_HOME;
    lv_scr_load_anim(g_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 160, 0, false);
  }
}

// ── public API ───────────────────────────────────────────────────────────────

void app_init() {
  // Power the CC1101 + WS2812 rails before touching the bus.
  pinMode(BOARD_PWR_EN, OUTPUT);
  digitalWrite(BOARD_PWR_EN, HIGH);

  spi_bus_init();
  display::init();
  encoder_init();
  ui::init_styles();

  build_home();
  lv_scr_load(g_home);
  g_level = LVL_HOME;
}

void app_loop() {
  EncoderInput in = encoder_poll();

  if (in.longPress) {
    app_back();
  } else if (g_level == LVL_HOME) {
    if (in.delta) {
      g_home_sel = clamp_index(g_home_sel + in.delta, CAT_COUNT);
      highlight(g_home_rows, CAT_COUNT, g_home_sel);
    }
    if (in.shortPress) open_category(g_home_sel);
  } else if (g_level == LVL_CAT) {
    if (in.delta) {
      g_cat_sel = clamp_index(g_cat_sel + in.delta, CATS[g_cat_index].count);
      highlight(g_cat_rows, CATS[g_cat_index].count, g_cat_sel);
    }
    if (in.shortPress) open_test(CATS[g_cat_index].tests[g_cat_sel]);
  } else {
    if (g_active && g_active->enc) g_active->enc(in.delta, in.shortPress);
    if (g_active && g_active->tick) g_active->tick();
  }

  lv_timer_handler();
  delay(2);
}

lv_obj_t* app_content() { return g_content; }
const TestSpec* app_active() { return g_active; }

void app_set_status(AppStatus st, const char* fmt, ...) {
  if (!g_status_lbl) return;
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  lv_label_set_text(g_status_lbl, buf);
  if (g_status_dot) {
    lv_obj_set_style_bg_color(g_status_dot, col(ui::status_color(st)), 0);
  }
}

void app_set_title(const char* title) {
  if (g_title_lbl) lv_label_set_text(g_title_lbl, title);
}

void app_set_hint(const char* text) {
  if (g_hint_lbl) lv_label_set_text(g_hint_lbl, text);
}
