// Display bring-up: TFT_eSPI + an LVGL 8 display driver that uses a partial
// draw buffer so live values only redraw the affected pixels.
#pragma once

#include <lvgl.h>

namespace display {
void init();          // call after spi_bus_init()
void set_backlight(int percent);  // 0..100 (PWM)
int  backlight();
}  // namespace display
