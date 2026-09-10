// Rotary encoder input (GPIO A=4, B=5, KEY=0) with debounced short/long press.
#pragma once

struct EncoderInput {
  int delta;        // net detents since last poll (negative = CCW)
  bool shortPress;  // released before the long-press threshold
  bool longPress;   // held past the threshold (fires once)
};

void encoder_init();
EncoderInput encoder_poll();  // consumes pending events
