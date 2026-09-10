// Test registry. Each peripheral module defines one TestSpec in its own .cpp.
#pragma once

#include "app.h"

// Display / UI
extern const TestSpec test_display;
extern const TestSpec test_encoder;

// Light + radio
extern const TestSpec test_leds;
extern const TestSpec test_ir;
extern const TestSpec test_cc1101;
extern const TestSpec test_nfc;

// Storage + audio
extern const TestSpec test_sd;
extern const TestSpec test_mic;
extern const TestSpec test_speaker;

// Power + IO
extern const TestSpec test_battery;
extern const TestSpec test_gpio;

// Radios + system
extern const TestSpec test_radio;
extern const TestSpec test_sysinfo;
