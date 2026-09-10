#include "bq27220_min.h"
#include "board_pins.h"

namespace gauge {

// Standard BQ27220 command registers (see vendor bq27220_def.h).
static const uint8_t CMD_TEMPERATURE   = 0x06;
static const uint8_t CMD_VOLTAGE       = 0x08;
static const uint8_t CMD_BATT_STATUS   = 0x0A;
static const uint8_t CMD_CURRENT       = 0x0C;
static const uint8_t CMD_REMAIN_CAP    = 0x10;
static const uint8_t CMD_FULL_CAP      = 0x12;
static const uint8_t CMD_AVG_CURRENT   = 0x14;
static const uint8_t CMD_SOC           = 0x2C;

static TwoWire* s_wire = &Wire;
static uint8_t s_addr = 0x55;

uint16_t raw(uint8_t command) {
  s_wire->beginTransmission(s_addr);
  s_wire->write(command);
  if (s_wire->endTransmission(false) != 0) return 0;
  if (s_wire->requestFrom((int)s_addr, 2) != 2) return 0;
  uint8_t lo = s_wire->read();
  uint8_t hi = s_wire->read();
  return (uint16_t)((hi << 8) | lo);
}

bool begin(TwoWire& wire, uint8_t addr) {
  s_wire = &wire;
  s_addr = addr;
  // Probe: control-status / device number. A stuck bus returns 0xFFFF/0x0000.
  uint16_t status = raw(0x00);
  return status != 0x0000 && status != 0xFFFF;
}

bool read(Reading& out) {
  out.present = true;
  out.voltage_mv = raw(CMD_VOLTAGE);
  out.current_ma = (int16_t)raw(CMD_CURRENT);
  out.avg_current_ma = (int16_t)raw(CMD_AVG_CURRENT);
  out.remaining_mah = raw(CMD_REMAIN_CAP);
  out.full_mah = raw(CMD_FULL_CAP);
  out.soc_pct = raw(CMD_SOC);
  out.temperature_c10 = raw(CMD_TEMPERATURE);
  out.battery_status = raw(CMD_BATT_STATUS);
  // A live gauge never reports 0 mV while present.
  if (out.voltage_mv == 0 || out.voltage_mv == 0xFFFF) {
    out.present = false;
  }
  return out.present;
}

}  // namespace gauge
