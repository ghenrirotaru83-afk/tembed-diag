#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <lvgl.h>
#include <cmath>
#include <cstdio>

using namespace ui;

static const i2s_port_t MIC_PORT = I2S_NUM_0;
static const int SAMPLE_RATE = 16000;
static const int SAMPLES = 256;

static int16_t s_buf[SAMPLES];
static bool s_installed = false;
static int s_level = 0;
static int s_peak = 0;
static lv_obj_t* s_bar = nullptr;
static lv_obj_t* s_value = nullptr;
static lv_obj_t* s_peak_lbl = nullptr;

static void install() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 128;
  cfg.use_apll = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_PIN_NO_CHANGE;
  pins.ws_io_num = BOARD_MIC_CLK;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = BOARD_MIC_DATA;

  if (i2s_driver_install(MIC_PORT, &cfg, 0, nullptr) != ESP_OK) return;
  if (i2s_set_pin(MIC_PORT, &pins) != ESP_OK) return;
  i2s_set_clk(MIC_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  s_installed = true;
}

static void build(lv_obj_t* c) {
  label(c, "Input level", 12, C_MUTED);
  s_bar = ui::bar(c, DISPLAY_WIDTH - 40, 22, C_PASS);
  s_value = label(c, "0", 24, C_PASS, true);
  s_peak_lbl = kv_row(c, "Peak hold");
  label(c, "PDM mic on DATA=42 / CLK=39. If a CC1101 daughter board is "
           "attached the onboard mic is disabled by hardware - a flat 0 here "
           "is expected in that configuration, not a fault.", 11, C_MUTED);
}

static void enter() {
  s_peak = 0;
  if (!s_installed) install();
}

static void tick() {
  if (!s_installed) {
    app_set_status(ST_FAIL, "I2S mic unavailable");
    return;
  }
  size_t bytes = 0;
  if (i2s_read(MIC_PORT, s_buf, sizeof(s_buf), &bytes, 0) != ESP_OK || bytes == 0) return;

  int n = bytes / sizeof(int16_t);
  double acc = 0;
  int local_peak = 0;
  for (int i = 0; i < n; i++) {
    int v = s_buf[i];
    acc += (double)v * v;
    if (abs(v) > local_peak) local_peak = abs(v);
  }
  int rms = (n > 0) ? (int)sqrt(acc / n) : 0;
  s_level = constrain(rms / 120, 0, 100);
  if (local_peak > s_peak) s_peak = local_peak;

  if (s_bar) lv_bar_set_value(s_bar, s_level, LV_ANIM_OFF);
  if (s_value) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", s_level);
    lv_label_set_text(s_value, buf);
  }
  if (s_peak_lbl) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s_peak);
    lv_label_set_text(s_peak_lbl, buf);
  }
  app_set_status(s_level > 3 ? ST_PASS : ST_INFO,
                 s_level > 3 ? "signal detected" : "listening...");
}

static void leave() {
  if (s_installed) {
    i2s_driver_uninstall(MIC_PORT);
    s_installed = false;
  }
}

extern const TestSpec test_mic = {
  "mic", "Microphone", build, enter, tick, leave, nullptr
};
