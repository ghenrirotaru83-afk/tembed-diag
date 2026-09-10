#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

// I2C wiring: SDA=8, SCL=18, IRQ=17, RESET=45 (addr 0x24).
static Adafruit_PN532 s_nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);

static bool s_ready = false;
static uint32_t s_fw = 0;
static uint32_t s_tags = 0;
static char s_uid[32] = "--";
static lv_obj_t* s_fw_lbl = nullptr;
static lv_obj_t* s_tag_lbl = nullptr;
static lv_obj_t* s_count_lbl = nullptr;

static void build(lv_obj_t* c) {
  s_fw_lbl    = kv_row(c, "PN532 firmware");
  s_tag_lbl   = kv_row(c, "Tag UID");
  s_count_lbl = kv_row(c, "Tags seen");
  label(c, "PN532 is on the shared I2C bus (addr 0x24). Hold a 13.56MHz "
           "card near the coil.", 11, C_MUTED);
}

static void enter() {
  s_ready = false;
  s_fw = 0;
  s_tags = 0;
  snprintf(s_uid, sizeof(s_uid), "--");

  pinMode(BOARD_PN532_RF_REST, OUTPUT);
  digitalWrite(BOARD_PN532_RF_REST, HIGH);
  pinMode(BOARD_PN532_IRQ, INPUT_PULLUP);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  s_nfc.begin();
  Wire.setClock(100000);
  Wire.setTimeOut(20);

  s_fw = s_nfc.getFirmwareVersion();
  if (s_fw) {
    s_nfc.SAMConfig();
    s_ready = true;
  }
}

static void tick() {
  static uint32_t next = 0;
  uint32_t now = millis();
  if (now >= next) {
    next = now + 250;
    if (s_ready) {
      uint8_t uid[7] = {0};
      uint8_t uid_len = 0;
      if (s_nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_len, 50) && uid_len > 0) {
        s_tags++;
        char buf[32];
        int n = 0;
        for (uint8_t i = 0; i < uid_len && n < (int)sizeof(buf) - 3; i++) {
          n += snprintf(buf + n, sizeof(buf) - n, "%02X ", uid[i]);
        }
        snprintf(s_uid, sizeof(s_uid), "%s", buf);
      }
    }
  }

  if (s_fw_lbl) {
    if (!s_ready) {
      lv_label_set_text(s_fw_lbl, "not detected");
      value_label(s_fw_lbl, C_FAIL);
    } else {
      char buf[32];
      snprintf(buf, sizeof(buf), "IC 0x%02X  v%d.%d",
               (unsigned)((s_fw >> 24) & 0xFF),
               (int)((s_fw >> 16) & 0xFF), (int)((s_fw >> 8) & 0xFF));
      lv_label_set_text(s_fw_lbl, buf);
      value_label(s_fw_lbl, C_PASS);
    }
  }
  if (s_tag_lbl) {
    lv_label_set_text(s_tag_lbl, s_uid);
    value_label(s_tag_lbl, s_tags ? C_PASS : C_TEXT);
  }
  if (s_count_lbl) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)s_tags);
    lv_label_set_text(s_count_lbl, buf);
  }

  app_set_status(s_ready ? (s_tags ? ST_PASS : ST_RUN) : ST_FAIL,
                 s_ready ? (s_tags ? "tag detected" : "waiting for tag") : "PN532 not found");
}

static void leave() {
  s_ready = false;
}

extern const TestSpec test_nfc = {
  "nfc", "NFC PN532", build, enter, tick, leave, nullptr
};
