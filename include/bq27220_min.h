// Minimal BQ27220 fuel-gauge reader.
//
// The vendor ships a full config/data-memory driver, but the gauge is already
// configured on the board; the tests only need the standard command registers
// (voltage, current, capacity, state of charge). Reading them needs no unseal.
#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace gauge {
struct Reading {
  bool present = false;
  uint16_t voltage_mv = 0;
  int16_t current_ma = 0;         // + charging, - discharging
  int16_t avg_current_ma = 0;
  uint16_t remaining_mah = 0;
  uint16_t full_mah = 0;
  uint16_t soc_pct = 0;
  uint16_t temperature_c10 = 0;   // 0.1 C units
  uint16_t battery_status = 0;
};

bool begin(TwoWire& wire = Wire, uint8_t addr = 0x55);
bool read(Reading& out);
uint16_t raw(uint8_t command);  // exposed for diagnostics
}  // namespace gauge
