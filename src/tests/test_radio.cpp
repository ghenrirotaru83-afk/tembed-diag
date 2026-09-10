#include "app.h"
#include "theme.h"

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static volatile int s_wifi_count = -1;
static volatile int s_ble_count = -1;
static volatile bool s_busy = false;
static char s_best_ssid[33] = "--";
static volatile int s_best_rssi = -127;
static TaskHandle_t s_task = nullptr;

static lv_obj_t* s_wifi = nullptr;
static lv_obj_t* s_ble = nullptr;
static lv_obj_t* s_best = nullptr;

static void scan_task(void*) {
  // Wi-Fi scan
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(150);
  int n = WiFi.scanNetworks(false, true);
  s_wifi_count = n;
  if (n > 0) {
    int best = 0;
    for (int i = 1; i < n; i++) {
      if (WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
    }
    snprintf(s_best_ssid, sizeof(s_best_ssid), "%s", WiFi.SSID(best).c_str());
    s_best_rssi = WiFi.RSSI(best);
  }
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);

  // BLE scan
  BLEDevice::init("tembed-diag");
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  BLEScanResults res = scan->start(3, false);
  s_ble_count = res.getCount();
  scan->clearResults();
  BLEDevice::deinit(true);

  s_busy = false;
  s_task = nullptr;
  vTaskDelete(nullptr);
}

static void build(lv_obj_t* c) {
  s_wifi = kv_row(c, "Wi-Fi APs found");
  s_ble = kv_row(c, "BLE devices found");
  s_best = kv_row(c, "Strongest AP");
  label(c, "A scan runs on a background task so the UI stays responsive. "
           "Press to re-scan.", 11, C_MUTED);
}

static void enter() {
  if (s_busy) return;
  s_wifi_count = -1;
  s_ble_count = -1;
  s_best_rssi = -127;
  snprintf(s_best_ssid, sizeof(s_best_ssid), "--");
  s_busy = true;
  xTaskCreatePinnedToCore(scan_task, "radio_scan", 1024 * 6, nullptr, 1, &s_task, 0);
}

static void tick() {
  if (s_wifi) {
    if (s_busy && s_wifi_count < 0) { lv_label_set_text(s_wifi, "scanning..."); value_label(s_wifi, C_WARN); }
    else {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", s_wifi_count);
      lv_label_set_text(s_wifi, buf);
      value_label(s_wifi, s_wifi_count >= 0 ? C_PASS : C_FAIL);
    }
  }
  if (s_ble) {
    if (s_busy && s_ble_count < 0) { lv_label_set_text(s_ble, "scanning..."); value_label(s_ble, C_WARN); }
    else if (s_ble_count < 0) { lv_label_set_text(s_ble, "--"); }
    else {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", s_ble_count);
      lv_label_set_text(s_ble, buf);
      value_label(s_ble, C_PASS);
    }
  }
  if (s_best) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%s (%d dBm)", s_best_ssid, s_best_rssi);
    lv_label_set_text(s_best, buf);
  }
  app_set_status(s_busy ? ST_RUN : ((s_wifi_count >= 0 || s_ble_count >= 0) ? ST_PASS : ST_IDLE),
                 s_busy ? "scanning" : "done");
}

static void leave() {
  // Task self-deletes; if still running it will finish in the background.
}

static void enc(int, bool pressed) {
  if (pressed && !s_busy) enter();
}

extern const TestSpec test_radio = {
  "radio", "Wireless (WiFi + BLE)", build, enter, tick, leave, enc
};
