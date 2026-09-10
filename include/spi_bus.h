// Shared SPI bus helpers for the display / SD / CC1101 (all share SCLK=11,
// MOSI=9, MISO=10).
#pragma once

void spi_bus_init();       // configure CS lines idle-high and bring up SPI
void spi_deselect_all();   // drive every CS high before a transaction
