#include "app.h"
#include "theme.h"
#include "board_pins.h"
#include "bq27220_min.h"

#include <Arduino.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <lvgl.h>
#include <cstdio>

using namespace ui;

static XPowersPPM s_pmu;
static bool s_pmu_ok = false;
static bool s_gauge_ok = false;

static lv_obj_t* r_soc = nullptr;
static lv_obj_t* r_vbat = nullptr;
static lv_obj_t* r_i = nullptr;
static lv_obj_t* r_cap = nullptr;
static lv_obj_t* r_temp = nullptr;
static lv_obj_t* r_chg = nullptr;
static lv_obj_t* r_vbus = nullptr;

static const char* charge_state_label() {
  if (!s_pmu_ok) return "n/a";
  if (!s_pmu.isVbusIn()) return "on battery";
  switch ((int)s_pmu.chargeStatus()) {
    case 1: return "pre-charge";
    case 2: return "fast charging";
    case 3: return "charge done";
    default: return "USB, idle";
  }
}

static void build(lv_obj_t* c) {
  label(c, "Battery", 16, C_PRIMARY, true);
  r_soc  = kv_row(c, "State of charge");
  r_vbat = kv_row(c, "Battery voltage");
  r_i    = kv_row(c, "Current (avg)");
  r_cap  = kv_row(c, "Capacity left");
  r_temp = kv_row(c, "Temperature");
  label(c, "Charger (BQ25896)", 16, C_PRIMARY, true);
  r_chg  = kv_row(c, "Charge state");
  r_vbus = kv_row(c, "VBUS voltage");
  label(c, "Gauge BQ27220 @0x55, charger BQ25896 @0x6B on the shared I2C bus.",
        11, C_MUTED);
}

static void enter() {
  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  s_pmu_ok = s_pmu.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS);
  s_gauge_ok = gauge::begin(Wire, BOARD_I2C_ADDR_BQ27220);
}

static void tick() {
  static uint32_t next = 0;
  uint32_t now = millis();
  if (now < next) return;
  next = now + 500;

  char buf[48];

  if (r_soc) {
    if (s_gauge_ok) {
      gauge::Reading g;
      if (gauge::read(g)) {
        snprintf(buf, sizeof(buf), "%u%%", g.soc_pct);
        lv_label_set_text(r_soc, buf);
        value_label(r_soc, g.soc_pct > 20 ? C_PASS : C_WARN);
      } else {
        lv_label_set_text(r_soc, "read error");
        value_label(r_soc, C_FAIL);
      }
    } else {
      lv_label_set_text(r_soc, "gauge not found");
      value_label(r_soc, C_FAIL);
    }
  }

  if (s_gauge_ok) {
    gauge::Reading g;
    if (gauge::read(g)) {
      if (r_vbat) { snprintf(buf, sizeof(buf), "%u mV", g.voltage_mv); lv_label_set_text(r_vbat, buf); }
      if (r_i) {
        snprintf(buf, sizeof(buf), "%d mA (avg %d)", g.current_ma, g.avg_current_ma);
        lv_label_set_text(r_i, buf);
        value_label(r_i, g.current_ma > 5 ? C_PASS : (g.current_ma < -5 ? C_WARN : C_TEXT));
      }
      if (r_cap) { snprintf(buf, sizeof(buf), "%u / %u mAh", g.remaining_mah, g.full_mah); lv_label_set_text(r_cap, buf); }
      if (r_temp) {
        float c = g.temperature_c10 * 0.1f - 273.15f;
        snprintf(buf, sizeof(buf), "%.1f C", c);
        lv_label_set_text(r_temp, buf);
      }
    }
  }

  if (r_chg) {
    const char* s = charge_state_label();
    lv_label_set_text(r_chg, s);
    value_label(r_chg, (!s_pmu_ok) ? C_FAIL : (s_pmu.isVbusIn() ? C_PASS : C_MUTED));
  }
  if (r_vbus) {
    if (s_pmu_ok) {
      snprintf(buf, sizeof(buf), "%u mV", s_pmu.getVbusVoltage());
      lv_label_set_text(r_vbus, buf);
    } else {
      lv_label_set_text(r_vbus, "charger not found");
      value_label(r_vbus, C_FAIL);
    }
  }

  app_set_status((s_pmu_ok || s_gauge_ok) ? ST_PASS : ST_FAIL,
                 "gauge:%s charger:%s", s_gauge_ok ? "ok" : "no", s_pmu_ok ? "ok" : "no");
}

static void leave() {}

extern const TestSpec test_battery = {
  "battery", "Battery", build, enter, tick, leave, nullptr
};
