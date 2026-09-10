#include "app.h"
#include "spi_bus.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <RadioLib.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static Module* s_module = nullptr;
static CC1101* s_radio = nullptr;
static bool s_ok = false;
static int s_chip_version = -1;
static int s_tx_count = 0;
static float s_freq = 433.92f;
static float s_rssi = -127.0f;

static const float FREQS[] = { 315.0f, 433.92f, 868.0f, 915.0f };
static const char* BANDS[] = { "315 MHz", "433.92 MHz", "868 MHz", "915 MHz" };
static const int FREQ_COUNT = 4;
static int s_freq_idx = 1;

static lv_obj_t* s_version = nullptr;
static lv_obj_t* s_freq_lbl = nullptr;
static lv_obj_t* s_rssi_lbl = nullptr;
static lv_obj_t* s_tx_lbl = nullptr;
static lv_obj_t* s_band_lbl = nullptr;

static void set_band() {
  pinMode(BOARD_LORA_SW1, OUTPUT);
  pinMode(BOARD_LORA_SW0, OUTPUT);
  if (s_freq_idx == 0) {            // 315
    digitalWrite(BOARD_LORA_SW1, HIGH);
    digitalWrite(BOARD_LORA_SW0, LOW);
  } else if (s_freq_idx == 1) {     // 434
    digitalWrite(BOARD_LORA_SW1, HIGH);
    digitalWrite(BOARD_LORA_SW0, HIGH);
  } else {                          // 868 / 915
    digitalWrite(BOARD_LORA_SW1, LOW);
    digitalWrite(BOARD_LORA_SW0, HIGH);
  }
}

static void tune() {
  if (!s_ok) return;
  set_band();
  delay(5);
  s_radio->setFrequency(s_freq);
  s_radio->setOutputPower(10);
  s_radio->startReceive();
}

static void build(lv_obj_t* c) {
  s_version   = kv_row(c, "Chip version");
  s_band_lbl  = kv_row(c, "Band");
  s_freq_lbl  = kv_row(c, "Frequency");
  s_rssi_lbl  = kv_row(c, "Live RSSI");
  s_tx_lbl    = kv_row(c, "TX test");
  label(c, "Version 0x14 is current, 0x04 legacy, 0x17 clone. "
           "Rotate to pick a band, press to send a test packet.", 11, C_MUTED);
}

static void info_update() {
  char buf[40];
  if (!s_ok) {
    app_set_status(ST_FAIL, "radio not responding");
    if (s_version) { lv_label_set_text(s_version, "no response"); value_label(s_version, C_FAIL); }
    return;
  }
  if (s_version) {
    snprintf(buf, sizeof(buf), "0x%02X", s_chip_version & 0xFF);
    lv_label_set_text(s_version, buf);
    value_label(s_version, s_chip_version == 0x14 ? C_PASS : C_WARN);
  }
  if (s_band_lbl) lv_label_set_text(s_band_lbl, BANDS[s_freq_idx]);
  if (s_freq_lbl) { snprintf(buf, sizeof(buf), "%.2f MHz", s_freq); lv_label_set_text(s_freq_lbl, buf); }
}

static void enter() {
  s_ok = false;
  s_chip_version = -1;
  s_tx_count = 0;
  s_freq_idx = 1;
  s_freq = FREQS[s_freq_idx];

  spi_deselect_all();
  s_module = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, RADIOLIB_NC, BOARD_LORA_IO2);
  s_radio = new CC1101(s_module);
  set_band();
  int st = s_radio->begin(s_freq);
  if (st == RADIOLIB_ERR_NONE) {
    s_ok = true;
    s_chip_version = s_radio->getChipVersion();
    s_radio->setOutputPower(10);
    s_radio->startReceive();
  }
}

static void tick() {
  static uint32_t next = 0;
  uint32_t now = millis();
  if (now < next) return;
  next = now + 250;

  if (s_ok) {
    s_rssi = s_radio->getRSSI();
    if (s_rssi_lbl) {
      char buf[24];
      snprintf(buf, sizeof(buf), "%.1f dBm", s_rssi);
      lv_label_set_text(s_rssi_lbl, buf);
      value_label(s_rssi_lbl, s_rssi > -90 ? C_PASS : C_WARN);
    }
  }
  info_update();
}

static void leave() {
  if (s_radio) {
    s_radio->sleep();
    delete s_radio;
    s_radio = nullptr;
  }
  if (s_module) {
    delete s_module;
    s_module = nullptr;
  }
  s_ok = false;
}

static void enc(int delta, bool pressed) {
  if (delta) {
    s_freq_idx = (s_freq_idx + (delta > 0 ? 1 : -1) + FREQ_COUNT) % FREQ_COUNT;
    s_freq = FREQS[s_freq_idx];
    tune();
    info_update();
  }
  if (pressed && s_ok) {
    int st = s_radio->transmit("T-EMBED-DIAG");
    s_tx_count++;
    if (s_tx_lbl) {
      char buf[40];
      if (st == RADIOLIB_ERR_NONE) snprintf(buf, sizeof(buf), "sent #%d OK", s_tx_count);
      else snprintf(buf, sizeof(buf), "error %d", st);
      lv_label_set_text(s_tx_lbl, buf);
      value_label(s_tx_lbl, st == RADIOLIB_ERR_NONE ? C_PASS : C_FAIL);
    }
    app_set_status(st == RADIOLIB_ERR_NONE ? ST_PASS : ST_FAIL, "TX %s", st == RADIOLIB_ERR_NONE ? "ok" : "failed");
    s_radio->startReceive();
  }
}

extern const TestSpec test_cc1101 = {
  "cc1101", "Sub-GHz CC1101", build, enter, tick, leave, enc
};
