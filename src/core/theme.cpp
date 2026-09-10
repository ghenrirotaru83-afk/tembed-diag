#include "theme.h"
#include "board_pins.h"
#include "app.h"

namespace ui {

static const lv_font_t* font_for(int size) {
  if (size <= 12) return &lv_font_montserrat_12;
  if (size <= 14) return &lv_font_montserrat_14;
  if (size <= 16) return &lv_font_montserrat_16;
  if (size <= 20) return &lv_font_montserrat_20;
  if (size <= 24) return &lv_font_montserrat_24;
  return &lv_font_montserrat_28;
}

void init_styles() {}

lv_obj_t* screen() {
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scr, col(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 0, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  return scr;
}

lv_obj_t* header(lv_obj_t* scr, const char* title) {
  lv_obj_t* h = lv_obj_create(scr);
  lv_obj_set_size(h, DISPLAY_WIDTH, 34);
  lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(h, col(C_SURFACE), 0);
  lv_obj_set_style_radius(h, 0, 0);
  lv_obj_set_style_border_width(h, 0, 0);
  lv_obj_set_style_pad_all(h, 0, 0);
  lv_obj_clear_flag(h, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* accent = lv_obj_create(h);
  lv_obj_set_size(accent, 4, 18);
  lv_obj_align(accent, LV_ALIGN_LEFT_MID, 8, 0);
  lv_obj_set_style_bg_color(accent, col(C_PRIMARY), 0);
  lv_obj_set_style_border_width(accent, 0, 0);
  lv_obj_set_style_radius(accent, 2, 0);
  lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* lbl = lv_label_create(h);
  lv_label_set_text(lbl, title);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl, col(C_TEXT), 0);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 18, 0);
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lbl, DISPLAY_WIDTH - 26);
  return lbl;
}

lv_obj_t* label(lv_obj_t* parent, const char* text, int size, uint32_t color, bool bold) {
  lv_obj_t* lbl = lv_label_create(parent);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_font(lbl, font_for(bold ? size + 1 : size), 0);
  lv_obj_set_style_text_color(lbl, col(color), 0);
  return lbl;
}

lv_obj_t* kv_row(lv_obj_t* parent, const char* key) {
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_set_size(row, lv_pct(100), 24);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* k = lv_label_create(row);
  lv_label_set_text(k, key);
  lv_obj_set_style_text_font(k, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(k, col(C_MUTED), 0);
  lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t* v = lv_label_create(row);
  lv_label_set_text(v, "--");
  lv_obj_set_style_text_font(v, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(v, col(C_TEXT), 0);
  lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
  return v;
}

void style_row(lv_obj_t* row, bool selected) {
  lv_obj_set_style_bg_color(row, col(selected ? C_PRIMARY : C_SURFACE), 0);
  lv_obj_set_style_bg_opa(row, selected ? LV_OPA_30 : LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(row, col(selected ? C_PRIMARY : C_BORDER), 0);
  lv_obj_set_style_border_width(row, selected ? 2 : 0, 0);
}

void value_label(lv_obj_t* lbl, uint32_t color) {
  lv_obj_set_style_text_color(lbl, col(color), 0);
}

lv_obj_t* bar(lv_obj_t* parent, int width, int height, uint32_t fill) {
  lv_obj_t* b = lv_bar_create(parent);
  lv_obj_set_size(b, width, height);
  lv_bar_set_range(b, 0, 100);
  lv_obj_set_style_bg_color(b, col(C_SURFACE2), LV_PART_MAIN);
  lv_obj_set_style_bg_color(b, col(fill), LV_PART_INDICATOR);
  lv_obj_set_style_radius(b, height / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(b, height / 2, LV_PART_INDICATOR);
  return b;
}

uint32_t status_color(int app_status) {
  switch (app_status) {
    case ST_PASS: return C_PASS;
    case ST_FAIL: return C_FAIL;
    case ST_RUN:  return C_WARN;
    case ST_INFO: return C_PRIMARY;
    default:      return C_MUTED;
  }
}

}  // namespace ui
