#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <esp_system.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static lv_obj_t* r_heap = nullptr;
static lv_obj_t* r_psram_free = nullptr;
static lv_obj_t* r_uptime = nullptr;

static const char* reset_reason_str(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:  return "power-on";
    case ESP_RST_EXT:      return "external pin";
    case ESP_RST_SW:       return "software";
    case ESP_RST_PANIC:    return "panic / abort";
    case ESP_RST_INT_WDT:  return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT:      return "other watchdog";
    case ESP_RST_DEEPSLEEP:return "deep sleep wake";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO:     return "SDIO";
    default:               return "unknown";
  }
}

static void fmt_uptime(char* buf, size_t n, uint32_t ms) {
  uint32_t s = ms / 1000;
  uint32_t h = s / 3600;
  uint32_t m = (s % 3600) / 60;
  s %= 60;
  snprintf(buf, n, "%luh %02lum %02lus", (unsigned long)h, (unsigned long)m, (unsigned long)s);
}

static void build(lv_obj_t* c) {
  label(c, BOARD_NAME, 16, C_PRIMARY, true);
  lv_obj_t* hw = kv_row(c, "Hardware / firmware");
  char hwb[48];
  snprintf(hwb, sizeof(hwb), "%s / %s", BOARD_HW_VERSION, FW_VERSION);
  lv_label_set_text(hw, hwb);

  lv_obj_t* chip = kv_row(c, "Chip");
  char cb[64];
  snprintf(cb, sizeof(cb), "%s r%d  %d cores", ESP.getChipModel(),
           ESP.getChipRevision(), ESP.getChipCores());
  lv_label_set_text(chip, cb);

  lv_obj_t* flash = kv_row(c, "Flash");
  char fb[24];
  snprintf(fb, sizeof(fb), "%u MB", (unsigned)(ESP.getFlashChipSize() / (1024 * 1024)));
  lv_label_set_text(flash, fb);

  lv_obj_t* psram = kv_row(c, "PSRAM total");
  char pb[24];
  snprintf(pb, sizeof(pb), "%u MB", (unsigned)(ESP.getPsramSize() / (1024 * 1024)));
  lv_label_set_text(psram, pb);

  r_psram_free = kv_row(c, "PSRAM free");
  r_heap = kv_row(c, "Free heap");
  r_uptime = kv_row(c, "Uptime");

  lv_obj_t* reset = kv_row(c, "Last reset");
  lv_label_set_text(reset, reset_reason_str(esp_reset_reason()));

  lv_obj_t* sdk = kv_row(c, "ESP-IDF SDK");
  lv_label_set_text(sdk, ESP.getSdkVersion());

  uint64_t mac = ESP.getEfuseMac();
  lv_obj_t* macl = kv_row(c, "MAC");
  char mb[24];
  snprintf(mb, sizeof(mb), "%02X:%02X:%02X:%02X:%02X:%02X",
           (unsigned)((mac >> 40) & 0xFF), (unsigned)((mac >> 32) & 0xFF),
           (unsigned)((mac >> 24) & 0xFF), (unsigned)((mac >> 16) & 0xFF),
           (unsigned)((mac >> 8) & 0xFF), (unsigned)(mac & 0xFF));
  lv_label_set_text(macl, mb);

  app_set_status(ST_INFO, "system");
}

static void tick() {
  static uint32_t next = 0;
  uint32_t now = millis();
  if (now < next) return;
  next = now + 1000;

  char buf[32];
  if (r_heap) {
    snprintf(buf, sizeof(buf), "%u KB", (unsigned)(ESP.getFreeHeap() / 1024));
    lv_label_set_text(r_heap, buf);
  }
  if (r_psram_free) {
    snprintf(buf, sizeof(buf), "%u KB", (unsigned)(ESP.getFreePsram() / 1024));
    lv_label_set_text(r_psram_free, buf);
  }
  if (r_uptime) {
    char ub[32];
    fmt_uptime(ub, sizeof(ub), now);
    lv_label_set_text(r_uptime, ub);
  }
}

extern const TestSpec test_sysinfo = {
  "sysinfo", "System info", build, nullptr, tick, nullptr, nullptr
};
