/* SPDX-License-Identifier: MIT */
#include <Arduino.h>

// Open the Serial Monitor at 115200 baud and send some text.
// Each received byte is sent back unchanged, including line endings.
void setup() {
  Serial.begin(115200);
  Serial.println("Serial echo ready.");
}

void loop() {
  int incoming = Serial.read();
  if (incoming >= 0) {
    Serial.write((uint8_t)incoming);
  } else {
    delay(1);
  }
}
