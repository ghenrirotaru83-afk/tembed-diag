#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static IRsend s_irsend(BOARD_IR_TX);
static IRrecv s_irrecv(BOARD_IR_RX);
static decode_results s_results;

// A short raw burst so the test can show the actual on/off timings it emitted.
static uint16_t s_raw[] = { 9000, 4500, 560, 1690, 560, 560, 560, 1690, 560 };
static const int s_raw_len = (int)(sizeof(s_raw) / sizeof(s_raw[0]));

static uint64_t s_rx_value = 0;
static uint32_t s_rx_count = 0;
static char s_rx_protocol[24] = "waiting";
static lv_obj_t* s_tx = nullptr;
static lv_obj_t* s_rx = nullptr;
static lv_obj_t* s_tx_count = nullptr;
static lv_obj_t* s_raw_lbl = nullptr;

static void transmit() {
  s_irsend.sendNEC(0x00FF00FF, 32);
  delay(20);
  s_irsend.sendRaw(s_raw, s_raw_len, 38);
}

static void build(lv_obj_t* c) {
  s_tx = kv_row(c, "Transmitted (NEC)");
  s_tx_count = kv_row(c, "TX bursts");
  s_rx = kv_row(c, "Received");
  s_raw_lbl = label(c, "", 12, C_MUTED);
  label(c, "Point a camera at the IR LED: it should flicker on TX. "
           "Loop-back RX needs a second remote/receiver.", 11, C_MUTED);
  app_set_status(ST_INFO, "press to transmit");
}

static void enter() {
  s_irsend.begin();
  s_irrecv.enableIRIn();
  s_rx_value = 0;
  s_rx_count = 0;
  snprintf(s_rx_protocol, sizeof(s_rx_protocol), "waiting");
}

static void tick() {
  if (s_irrecv.decode(&s_results)) {
    s_rx_value = s_results.value;
    s_rx_count++;
    String p = typeToString(s_results.decode_type, s_results.repeat);
    p.toUpperCase();
    snprintf(s_rx_protocol, sizeof(s_rx_protocol), "%s", p.c_str());
    s_irrecv.resume();
  }

  if (s_tx) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)0x00FF00FF);
    lv_label_set_text(s_tx, buf);
    value_label(s_tx, C_PRIMARY);
  }
  if (s_tx_count) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)s_rx_count);
    lv_label_set_text(s_tx_count, buf);
  }
  if (s_rx) {
    if (s_rx_count == 0) {
      lv_label_set_text(s_rx, "nothing");
      value_label(s_rx, C_MUTED);
    } else {
      char buf[40];
      snprintf(buf, sizeof(buf), "%s 0x%08lX", s_rx_protocol, (unsigned long)s_rx_value);
      lv_label_set_text(s_rx, buf);
      value_label(s_rx, C_PASS);
    }
  }
  if (s_raw_lbl) {
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "raw us:");
    for (int i = 0; i < s_raw_len && n < (int)sizeof(buf) - 8; i++) {
      n += snprintf(buf + n, sizeof(buf) - n, " %u", s_raw[i]);
    }
    lv_label_set_text(s_raw_lbl, buf);
  }
}

static void leave() {
  s_irsend.sendNEC(0x0, 32);
}

static void enc(int, bool pressed) {
  if (pressed) {
    transmit();
    app_set_status(ST_RUN, "IR burst sent");
  }
}

extern const TestSpec test_ir = {
  "ir", "Infrared", build, enter, tick, leave, enc
};
