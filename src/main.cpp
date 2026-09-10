#include <Arduino.h>

#include "app.h"

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.printf("\n%s diagnostic firmware %s\n", "T-Embed CC1101", "1.0.0");
  app_init();
}

void loop() {
  app_loop();
}
