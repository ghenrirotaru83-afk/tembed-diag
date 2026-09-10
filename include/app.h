// Application framework: menu stack + shared test interface.
//
// Navigation is rotary-encoder first. A test is a small struct of callbacks;
// the framework owns the screen chrome (header/footer) and the back stack.
#pragma once

#include <lvgl.h>

enum AppStatus {
  ST_IDLE,   // grey  - not started
  ST_RUN,    // yellow- running / streaming
  ST_PASS,   // green - passed
  ST_FAIL,   // red   - failed
  ST_INFO,   // blue  - informational / manual check
};

struct TestSpec {
  const char* id;
  const char* name;
  // Called once when the test screen opens. Add widgets to `content`.
  void (*build)(lv_obj_t* content);
  // Optional lifecycle hooks.
  void (*enter)();
  void (*tick)();                       // called every app_loop iteration
  void (*leave)();
  // Optional encoder handler: rotation delta and short-press. Long-press is
  // always "back" and never reaches the test.
  void (*enc)(int delta, bool pressed);
};

// Setup: display + LVGL + encoder + home screen. Call from setup().
void app_init();
// Drive the UI + input. Call from loop().
void app_loop();

// Content area of the active test screen (valid during build/tick).
lv_obj_t* app_content();
// Active test spec (null on menu screens).
const TestSpec* app_active();

// Header/footer updates for test screens.
void app_set_status(AppStatus st, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void app_set_title(const char* title);
void app_set_hint(const char* text);

// Pop one level (test -> category -> home). Also bound to encoder long-press.
void app_back();
