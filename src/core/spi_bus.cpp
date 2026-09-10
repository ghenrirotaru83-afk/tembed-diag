#include "spi_bus.h"
#include "board_pins.h"
#include <Arduino.h>
#include <SPI.h>

void spi_bus_init() {
  // Every device on the shared bus idles deselected.
  pinMode(DISPLAY_CS, OUTPUT);    digitalWrite(DISPLAY_CS, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT);   digitalWrite(BOARD_SD_CS, HIGH);
  pinMode(BOARD_LORA_CS, OUTPUT); digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_NRF24_CS, OUTPUT);digitalWrite(BOARD_NRF24_CS, HIGH);
  pinMode(BOARD_NRF24_CE, OUTPUT);digitalWrite(BOARD_NRF24_CE, LOW);
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
}

void spi_deselect_all() {
  digitalWrite(DISPLAY_CS, HIGH);
  digitalWrite(BOARD_SD_CS, HIGH);
  digitalWrite(BOARD_LORA_CS, HIGH);
  digitalWrite(BOARD_NRF24_CS, HIGH);
}
