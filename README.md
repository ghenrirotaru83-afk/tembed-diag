# tembed-diag — T-Embed CC1101 hardware diagnostics firmware

A standalone, rotary-encoder-driven diagnostic firmware for the **LilyGO
T-Embed CC1101** (ESP32-S3). It verifies every onboard peripheral and the
external header, with a polished LVGL UI and pass/fail colour coding — a
focused alternative to Bruce's stock factory-test screen.

This is its own firmware. Flash it *instead of* Bruce (or any other firmware);
it is not a menu inside another app.

```
T-Embed CC1101 (HW v1.0-241103)
fw 1.0.0
```

## Build

Toolchain: **Arduino framework via PlatformIO** (chosen because every
peripheral library this board uses — TFT_eSPI, LVGL 8.3, RadioLib, Adafruit
PN532, XPowersLib, IRremoteESP8266 — is Arduino-first in the vendor repo).

```sh
# one-time: install PlatformIO Core
pipx install platformio          # or: python -m pip install --user platformio

cd tembed-diag
pio run                          # compile
pio run -t upload                # flash over the native USB-C port
pio device monitor               # 115200 baud, optional
```

Build result on this machine (Arduino-ESP32 2.0.17 / IDF 4.4, no WiFi config):

| Resource | Used      | Limit    |
| -------- | --------- | -------- |
| Flash    | 1.81 MB   | 3 MB app |
| RAM      | 21.2 %    | 320 KB   |

### Flashing over USB

The board flashes through its native USB-C port with no serial adapter. If the
port does not appear or upload is refused, the user must be in the serial
group (Arch/CachyOS: `uucp`):

```sh
sudo usermod -aG uucp "$USER"
# log out and back in, then:
ls -l /dev/ttyACM*   # should exist after plugging in
```

If the board is not detected, hold the **encoder button (BOOT / GPIO0)** while
plugging USB to force the ROM downloader, then release and re-run upload.

### Configuration notes (why platformio.ini looks unusual)

- `boards/T_Embed_PN532.json` is vendored from the vendor repo; PlatformIO has
  no board matching this exact 16 MB/8 MB OPI variant.
- The bundled Arduino-ESP32 core libraries (`WiFi`, `BLE`, `SD`) declare
  `architectures=esp32`, which the `espressif32` platform's compatibility check
  rejects, and PlatformIO does not expose them to direct project includes
  (`#include <WiFi.h>`). They are therefore listed explicitly in `lib_deps`
  using `${platformio.packages_dir}`.
- TFT_eSPI is driven entirely through the `-DUSER_SETUP_LOADED` build flags,
  mirroring the vendor's `Setup214_LilyGo_T_Embed_PN532.h` (ST7789, HSPI,
  inversion on, shared SPI pins).

## Controls

| Input                | Action                          |
| -------------------- | ------------------------------- |
| Turn encoder         | move selection / adjust a value |
| Press encoder        | open / run / toggle             |
| **Long-press (0.7 s)** | go back one level             |

Navigation is three levels: **home → category → test**. Back returns to the
category with its selection intact, and home keeps its place.

## Layout

```
tembed-diag/
  platformio.ini
  boards/T_Embed_PN532.json      vendor board definition
  include/
    board_pins.h                 EVERY pin in one place (edit for a revision)
    lv_conf.h                    LVGL 8.3 config (PSRAM alloc, partial redraw)
    app.h  tests.h  theme.h ...  framework interfaces
  src/
    main.cpp
    core/
      spi_bus.cpp                shared-SPI CS handling
      display.cpp                TFT_eSPI + LVGL partial-buffer flush
      encoder.cpp                rotary encoder + debounced short/long press
      app.cpp                    menu stack, chrome, registry
      theme.cpp                  palette + widget helpers
      bq27220_min.cpp            minimal fuel-gauge register reader
    tests/
      test_display.cpp  test_encoder.cpp  test_leds.cpp
      test_ir.cpp  test_cc1101.cpp  test_nfc.cpp
      test_sd.cpp  test_mic.cpp  test_speaker.cpp
      test_battery.cpp  test_gpio.cpp  test_radio.cpp  test_sysinfo.cpp
  docs/MENU_MAP.md               screen -> peripheral/pins
```

Each test is one `TestSpec` (build/enter/tick/leave/enc) in its own file.
Adding a peripheral is one new file plus one entry in `src/core/app.cpp`.

## Assumptions to verify against your board

These are the places where the vendor documentation is incomplete or where the
physical revision can differ. **Check before trusting a failing result.**

1. **External header GPIOs** — only ~4 GPIO are broken out and the vendor does
   not enumerate them in text. `board_pins.h` assumes
   `{ 8, 18, 43, 44 }` (I²C SDA/SCL + the two nRF24 footprint pins). Change
   `BOARD_EXT_GPIOS[]` to match your silkscreen/schematic.
2. **IR transmitter** — the vendor defines `BOARD_IR_EN = 2` and
   `BOARD_IR_RX = 1`; the TX LED is assumed to be driven by GPIO2. Confirmed
   against vendor `infrared_send_test`/`factory` code, but not a datasheet pin.
3. **PN532 bus** — **I²C**, address `0x24`, on SDA=8/SCL=18. This is
   authoritative from the vendor README (the board's PN532 is *not* on SPI).
4. **Microphone vs CC1101** — the mic is PDM on DATA=42/CLK=39. If a CC1101
   daughter board is fitted, the mic is disabled by hardware; the mic screen
   says so and a flat 0 is expected, not a fault.
5. **Fuel gauge registers** — `bq27220_min.cpp` reads standard BQ27220 command
   registers directly (no unseal/config). If your gauge leaves the factory
   unconfigured, voltage/SOC may read 0; that is a gauge state, not a wiring
   fault.
6. **CC1101 band switch** — SW0/SW1 select the RF path (315 / 434 / 868-915).
   The test drives them, but the actual achievable frequency depends on the
   populated matching network and antenna.

## Safety notes

- The **External GPIO** screen can drive GPIO8/18, which are the shared I²C bus.
  Driving them disrupts the PN532 and battery gauge until you leave the screen.
  The UI warns about this.
- `BOARD_PWR_EN` (GPIO15) is driven HIGH at boot to power the CC1101 and the
  WS2812 rail; the GPIO test does not touch it.
- The CC1101 carrier test transmits a short packet. Transmitting on sub-GHz
  bands may be regulated in your region — keep output power low and use a
  dummy load/antenna.

## Known gaps

- The UI is validated by compilation, not on hardware (no board attached during
  development). Pin timings/levels should be sanity-checked on first boot.
- No touch input by design; encoder only.
- WiFi/BLE test only counts nearby APs/devices (radio-up check), not a full
  RF validation.
