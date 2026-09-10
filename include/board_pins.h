// Board pin map for LilyGO T-Embed CC1101 (HW v1.0-241103).
//
// Source: vendor repo Xinyuan-LilyGO/T-Embed-CC1101 (examples/utilities.h) and
// lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h.
//
// Every pin used by the firmware is centralised here so a future board
// revision ("CC1101 Plus") is a single-file change.

#pragma once

// ── Board identity ───────────────────────────────────────────────────────────
#define BOARD_NAME       "LilyGO T-Embed CC1101"
#define BOARD_HW_VERSION "v1.0-241103"
#define FW_VERSION       "1.0.0"

// ── Power / keys ─────────────────────────────────────────────────────────────
#define BOARD_USER_KEY   6
#define BOARD_PWR_EN     15   // must be driven HIGH to power CC1101 + WS2812

// ── WS2812 RGB ───────────────────────────────────────────────────────────────
#define WS2812_NUM_LEDS  8
#define WS2812_DATA_PIN  14

// ── Infrared ─────────────────────────────────────────────────────────────────
// GPIO2 is the IR LED driver enable/transmit line; GPIO1 is the receiver.
#define BOARD_IR_TX      2
#define BOARD_IR_RX      1

// ── Microphone (PDM) ─────────────────────────────────────────────────────────
#define BOARD_MIC_DATA   42
#define BOARD_MIC_CLK    39

// ── Speaker (I2S, MAX98357-style) ────────────────────────────────────────────
#define BOARD_VOICE_BCLK   46
#define BOARD_VOICE_LRCLK  40
#define BOARD_VOICE_DIN    7

// ── Display (ST7789 170x320, shared SPI) ─────────────────────────────────────
#define DISPLAY_WIDTH    170
#define DISPLAY_HEIGHT   320
#define DISPLAY_BL       21
#define DISPLAY_CS       41
#define DISPLAY_MISO     10
#define DISPLAY_MOSI     9
#define DISPLAY_SCLK     11
#define DISPLAY_DC       16
#define DISPLAY_RST      -1

// ── Rotary encoder ───────────────────────────────────────────────────────────
#define ENCODER_INA      4
#define ENCODER_INB      5
#define ENCODER_KEY      0    // also the BOOT strapping pin

// ── I2C (shared by PN532, BQ27220, BQ25896 and the external header) ──────────
#define BOARD_I2C_SDA    8
#define BOARD_I2C_SCL    18

#define BOARD_I2C_ADDR_PN532    0x24
#define BOARD_I2C_ADDR_BQ27220  0x55
#define BOARD_I2C_ADDR_BQ25896  0x6B

// ── PN532 NFC (I2C) ──────────────────────────────────────────────────────────
#define BOARD_PN532_RF_REST  45
#define BOARD_PN532_IRQ      17

// ── SPI (shared by display, SD, CC1101) ──────────────────────────────────────
#define BOARD_SPI_SCK    11
#define BOARD_SPI_MOSI   9
#define BOARD_SPI_MISO   10

// ── microSD (shares SPI) ─────────────────────────────────────────────────────
#define BOARD_SD_CS      13
#define BOARD_SD_SCK     BOARD_SPI_SCK
#define BOARD_SD_MOSI    BOARD_SPI_MOSI
#define BOARD_SD_MISO    BOARD_SPI_MISO

// ── CC1101 sub-GHz (shares SPI) ──────────────────────────────────────────────
#define BOARD_LORA_CS    12
#define BOARD_LORA_SCK   BOARD_SPI_SCK
#define BOARD_LORA_MOSI  BOARD_SPI_MOSI
#define BOARD_LORA_MISO  BOARD_SPI_MISO
#define BOARD_LORA_IO2   38   // GDO2
#define BOARD_LORA_IO0   3    // GDO0 / IRQ
#define BOARD_LORA_SW1   47   // band select: SW1:1/SW0:1 = 434MHz
#define BOARD_LORA_SW0   48

// nRF24 module footprints (not populated on the CC1101 board; held HIGH so the
// shared bus is never driven by an absent device).
#define BOARD_NRF24_CS   44
#define BOARD_NRF24_CE   43

// ── External extension header GPIOs ──────────────────────────────────────────
// ASSUMPTION TO VERIFY against your board silkscreen/schematic:
// only ~4 GPIO are broken out. This build treats SDA/SCL plus the two nRF24
// footprint pins as the header pins. Change this list if your revision differs.
//   * GPIO8 / GPIO18 are the shared I2C bus (toggling them disrupts the PN532
//     and the battery fuel gauge) — flagged in the GPIO test UI.
#define BOARD_EXT_GPIO_COUNT 4
static const int BOARD_EXT_GPIOS[BOARD_EXT_GPIO_COUNT] = { 8, 18, 43, 44 };
