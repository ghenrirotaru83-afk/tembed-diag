# Menu → peripheral / pin map

Home menu (top level) → category → test. Every pin below is defined in
`include/board_pins.h`. Values in **bold** are assumptions to verify against
your board revision (see `README.md`).

| # | Category          | Test screen          | Peripheral / signal                              | Pins / bus |
|---|-------------------|----------------------|--------------------------------------------------|------------|
| 0 | Display           | Display test         | ST7789V 170×320, backlight PWM, LVGL fonts       | CS=41, DC=16, RST=-1, BL=21, SCLK=11, MOSI=9, MISO=10 (HSPI) |
| 1 | Rotary encoder    | Rotary encoder       | Quadrature A/B + push button, debounce           | A=4, B=5, KEY=0 (BOOT) |
| 2 | RGB LEDs          | RGB LEDs (8x WS2812) | 8 addressable LEDs, walk/rainbow/brightness      | DATA=14, power via PWR_EN=15 |
| 3 | Infrared          | Infrared             | NEC TX burst + RX decode                         | TX=2, RX=1 |
| 4 | Sub-GHz CC1101    | Sub-GHz CC1101       | SPI version check, live RSSI, TX carrier         | CS=12, GDO0/IRQ=3, GDO2=38, SW0=48, SW1=47, shared SCLK/MOSI/MISO |
| 5 | NFC PN532         | NFC PN532            | I²C firmware readout + tag detect                | SDA=8, SCL=18, addr=0x24, IRQ=17, RESET=45 |
| 6 | microSD           | microSD              | Mount, capacity, write→read→verify, unmount      | CS=13, shared SCLK/MOSI/MISO |
| 7 | Audio             | Microphone           | PDM input level meter (RMS/peak)                 | DATA=42, CLK=39 (PDM) |
| 7 | Audio             | Speaker              | I²S test tone + volume sanity                    | BCLK=46, LRCLK=40, DIN=7 |
| 8 | Battery           | Battery              | BQ27220 gauge + BQ25896 charger, live values     | SDA=8, SCL=18, gauge=0x55, charger=0x6B |
| 9 | External GPIO     | External GPIO        | Per-pin drive LOW/HIGH and input read            | **8, 18, 43, 44** (editable in `board_pins.h`) |
| 10 | Wireless         | Wireless (WiFi+BLE)  | Scan for APs / BLE advertisers (radio-up check)  | internal 2.4 GHz radio |
| 11 | System info      | System info          | Chip, flash, PSRAM, heap, uptime, reset reason   | — |

## State machine per test

Each test screen has a footer status dot: **grey** = idle, **yellow** = running,
**green** = pass, **red** = fail, **blue** = informational/manual check.

| Test           | Pass condition (automated)                                   |
| -------------- | ------------------------------------------------------------ |
| Display        | Visual (colour bars/gradient/text); backlight sweeps 0–100%  |
| Encoder        | Visual+log; each detent should add exactly ±1                |
| RGB LEDs       | Visual; all 8 LEDs walk/rainbow                              |
| Infrared       | Visual (camera); RX reports a protocol/value if looped back  |
| CC1101         | Version reads `0x14`/`0x04`/`0x17`; RSSI updates; TX returns OK |
| NFC            | Firmware version non-zero; tag UID appears on a card         |
| microSD        | Mount succeeds and write→read→verify matches                 |
| Microphone     | Level bar moves on sound (flat 0 if CC1101 disables the mic) |
| Speaker        | Audible tone; volume changes with the encoder                |
| Battery        | Gauge/charger respond; SOC/voltage/current update            |
| External GPIO  | Multimeter/LED confirms the pin follows the on-screen state  |
| Wireless       | AP count ≥ 0 and BLE count ≥ 0                               |
| System info    | Always informational                                          |
