#include "app.h"
#include "spi_bus.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static const char* TEST_PATH = "/tembed_sd_test.txt";
static const char* TEST_TEXT = "T-Embed CC1101 SD write/read OK\n";

static bool s_mounted = false;
static lv_obj_t* s_type = nullptr;
static lv_obj_t* s_size = nullptr;
static lv_obj_t* s_rw = nullptr;

static void run_test() {
  if (s_mounted) { SD.end(); s_mounted = false; }
  spi_deselect_all();

  const uint32_t freqs[] = { 10000000UL, 4000000UL, 1000000UL };
  for (int i = 0; i < 3 && !s_mounted; i++) {
    if (SD.begin(BOARD_SD_CS, SPI, freqs[i])) s_mounted = true;
  }
  if (!s_mounted) {
    app_set_status(ST_FAIL, "mount failed");
    if (s_rw) { lv_label_set_text(s_rw, "mount failed"); value_label(s_rw, C_FAIL); }
    return;
  }

  uint8_t card_type = SD.cardType();
  if (card_type == CARD_NONE) {
    app_set_status(ST_FAIL, "no card");
    return;
  }

  if (s_type) {
    const char* t = "unknown";
    if (card_type == CARD_MMC) t = "MMC";
    else if (card_type == CARD_SD) t = "SDSC";
    else if (card_type == CARD_SDHC) t = "SDHC";
    lv_label_set_text(s_type, t);
    value_label(s_type, C_PASS);
  }
  if (s_size) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%llu MB  (used %llu MB)",
             (unsigned long long)(SD.cardSize() / (1024ULL * 1024ULL)),
             (unsigned long long)(SD.usedBytes() / (1024ULL * 1024ULL)));
    lv_label_set_text(s_size, buf);
  }

  // write -> read -> verify -> remove
  bool ok = false;
  File f = SD.open(TEST_PATH, FILE_WRITE);
  if (f) {
    f.print(TEST_TEXT);
    f.close();
    f = SD.open(TEST_PATH, FILE_READ);
    if (f) {
      String got = f.readStringUntil('\n');
      f.close();
      ok = got.startsWith("T-Embed CC1101 SD write/read OK");
    }
  }
  SD.remove(TEST_PATH);

  if (s_rw) {
    lv_label_set_text(s_rw, ok ? "write/read verified" : "verify failed");
    value_label(s_rw, ok ? C_PASS : C_FAIL);
  }
  app_set_status(ok ? ST_PASS : ST_FAIL, ok ? "read/write OK" : "read/write failed");
}

static void build(lv_obj_t* c) {
  s_type = kv_row(c, "Card type");
  s_size = kv_row(c, "Capacity");
  s_rw   = kv_row(c, "Read / write");
  label(c, "CS=13 on the shared SPI bus. Press to re-run the write/read "
           "verify. Unmounts on exit.", 11, C_MUTED);
  run_test();
}

static void tick() {}

static void leave() {
  if (s_mounted) {
    SD.end();
    s_mounted = false;
  }
}

static void enc(int, bool pressed) {
  if (pressed) run_test();
}

extern const TestSpec test_sd = {
  "sd", "microSD", build, nullptr, tick, leave, enc
};
