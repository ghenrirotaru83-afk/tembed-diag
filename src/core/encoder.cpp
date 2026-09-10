#include "encoder.h"
#include "board_pins.h"

#include <Arduino.h>
#include <RotaryEncoder.h>

static RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static void encoder_isr() {
  portENTER_CRITICAL_ISR(&mux);
  encoder.tick();
  portEXIT_CRITICAL_ISR(&mux);
}

static int last_pos = 0;
static bool raw_state = false;   // hardware level (true = pressed)
static bool stable_state = false;
static uint32_t last_change_ms = 0;
static uint32_t press_start_ms = 0;
static bool long_fired = false;

static const uint32_t DEBOUNCE_MS = 15;
static const uint32_t LONGPRESS_MS = 700;

void encoder_init() {
  pinMode(ENCODER_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_INA), encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_INB), encoder_isr, CHANGE);
  last_pos = encoder.getPosition();
  raw_state = stable_state = (digitalRead(ENCODER_KEY) == LOW);
}

EncoderInput encoder_poll() {
  EncoderInput in{0, false, false};
  const uint32_t now = millis();

  int pos;
  portENTER_CRITICAL(&mux);
  encoder.tick();
  pos = encoder.getPosition();
  portEXIT_CRITICAL(&mux);
  in.delta = pos - last_pos;
  last_pos = pos;

  const bool now_pressed = (digitalRead(ENCODER_KEY) == LOW);
  if (now_pressed != raw_state) {
    raw_state = now_pressed;
    last_change_ms = now;
  }

  if ((now - last_change_ms) >= DEBOUNCE_MS && stable_state != raw_state) {
    stable_state = raw_state;
    if (stable_state) {
      press_start_ms = now;
      long_fired = false;
    } else if (!long_fired && (now - press_start_ms) < LONGPRESS_MS) {
      in.shortPress = true;
    }
  }

  if (stable_state && !long_fired && (now - press_start_ms) >= LONGPRESS_MS) {
    in.longPress = true;
    long_fired = true;
  }

  return in;
}
