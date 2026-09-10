// Shared visual language: palette + small LVGL widget helpers so every screen
// uses the same chrome/spacing/colour coding.
#pragma once

#include <lvgl.h>

namespace ui {

// Palette (dark by default; partial updates keep it cheap to restyle).
constexpr uint32_t C_BG        = 0x0E1116;
constexpr uint32_t C_SURFACE   = 0x1A1F29;
constexpr uint32_t C_SURFACE2  = 0x232A36;
constexpr uint32_t C_BORDER    = 0x2E3646;
constexpr uint32_t C_TEXT      = 0xE6EAF2;
constexpr uint32_t C_MUTED     = 0x8B93A7;
constexpr uint32_t C_PRIMARY   = 0x4CC2FF;
constexpr uint32_t C_PASS      = 0x38D39F;
constexpr uint32_t C_FAIL      = 0xFF5C6C;
constexpr uint32_t C_WARN      = 0xFFCF5C;

inline lv_color_t col(uint32_t hex) { return lv_color_hex(hex); }

void init_styles();

// Screens / chrome -----------------------------------------------------------
lv_obj_t* screen();                                    // dark full-screen container
lv_obj_t* header(lv_obj_t* scr, const char* title);    // accent bar + title, returns title label's parent
void      set_header_title(lv_obj_t* header, const char* title);

// Widgets --------------------------------------------------------------------
lv_obj_t* label(lv_obj_t* parent, const char* text, int size = 14,
                uint32_t color = C_TEXT, bool bold = false);
lv_obj_t* kv_row(lv_obj_t* parent, const char* key);   // returns value label to fill
void      style_row(lv_obj_t* row, bool selected);
void      value_label(lv_obj_t* lbl, uint32_t color);
lv_obj_t* bar(lv_obj_t* parent, int width, int height, uint32_t fill);

// Footer status helper colours.
uint32_t status_color(int app_status);

}  // namespace ui
