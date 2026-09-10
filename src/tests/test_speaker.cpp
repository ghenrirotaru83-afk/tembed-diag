#include "app.h"
#include "theme.h"
#include "board_pins.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <lvgl.h>
#include <cmath>
#include <cstdio>

using namespace ui;

static const i2s_port_t SPK_PORT = I2S_NUM_1;
static const int SAMPLE_RATE = 44100;
static const int CHUNK = 256;
static const int AMPLITUDE = 9000;

static const int FREQS[] = { 440, 880, 1000, 2000, 4000 };
static const int FREQ_COUNT = 5;

static bool s_installed = false;
static bool s_on = false;
static int s_freq_idx = 0;
static int s_volume = 60;
static float s_phase = 0.0f;
static lv_obj_t* s_state = nullptr;
static lv_obj_t* s_freq_lbl = nullptr;
static lv_obj_t* s_vol_lbl = nullptr;

static void install() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = CHUNK;
  cfg.use_apll = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = BOARD_VOICE_BCLK;
  pins.ws_io_num = BOARD_VOICE_LRCLK;
  pins.data_out_num = BOARD_VOICE_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(SPK_PORT, &cfg, 0, nullptr) != ESP_OK) return;
  if (i2s_set_pin(SPK_PORT, &pins) != ESP_OK) return;
  i2s_set_clk(SPK_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  i2s_zero_dma_buffer(SPK_PORT);
  s_installed = true;
}

static void build(lv_obj_t* c) {
  s_state = kv_row(c, "Output");
  s_freq_lbl = kv_row(c, "Frequency");
  s_vol_lbl = kv_row(c, "Amplitude");
  label(c, "I2S speaker on BCLK=46 / LRCLK=40 / DIN=7. Rotate to pick a tone, "
           "press to start/stop.", 11, C_MUTED);
}

static void enter() {
  s_on = false;
  s_freq_idx = 0;
  s_volume = 60;
  s_phase = 0.0f;
  if (!s_installed) install();
}

static void tick() {
  if (s_state) {
    lv_label_set_text(s_state, s_installed ? (s_on ? "playing" : "stopped") : "unavailable");
    value_label(s_state, !s_installed ? C_FAIL : (s_on ? C_PASS : C_MUTED));
  }
  if (s_freq_lbl) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d Hz", FREQS[s_freq_idx]);
    lv_label_set_text(s_freq_lbl, buf);
  }
  if (s_vol_lbl) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", s_volume);
    lv_label_set_text(s_vol_lbl, buf);
  }
  if (!s_installed || !s_on) return;

  int16_t buf[CHUNK];
  const float step = 2.0f * (float)M_PI * FREQS[s_freq_idx] / SAMPLE_RATE;
  const int amp = AMPLITUDE * s_volume / 100;
  for (int i = 0; i < CHUNK; i++) {
    s_phase += step;
    if (s_phase > 2.0f * (float)M_PI) s_phase -= 2.0f * (float)M_PI;
    buf[i] = (int16_t)(sinf(s_phase) * amp);
  }
  size_t written = 0;
  i2s_write(SPK_PORT, buf, sizeof(buf), &written, 0);
  app_set_status(ST_PASS, "tone %d Hz", FREQS[s_freq_idx]);
}

static void leave() {
  s_on = false;
  if (s_installed) {
    i2s_zero_dma_buffer(SPK_PORT);
    i2s_driver_uninstall(SPK_PORT);
    s_installed = false;
  }
}

static void enc(int delta, bool pressed) {
  if (delta) {
    if (s_on) {
      s_volume = constrain(s_volume + delta * 5, 0, 100);
      app_set_status(ST_RUN, "volume %d%%", s_volume);
    } else {
      s_freq_idx = (s_freq_idx + (delta > 0 ? 1 : -1) + FREQ_COUNT) % FREQ_COUNT;
    }
  }
  if (pressed) {
    s_on = !s_on;
    if (s_on) s_phase = 0.0f;
    app_set_status(s_on ? ST_PASS : ST_IDLE, s_on ? "tone on" : "tone off");
  }
}

extern const TestSpec test_speaker = {
  "speaker", "Speaker", build, enter, tick, leave, enc
};
