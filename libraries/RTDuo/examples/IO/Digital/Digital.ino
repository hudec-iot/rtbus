/* SPDX-License-Identifier: MIT */
#include <Arduino.h>

// Connect a push button between buttonPin and GND.
// Connect an LED anode to ledPin through a current-limiting resistor
// and its cathode to GND. Choose GPIOs exposed by your board if needed.
const uint32_t buttonPin = IO2;
const uint32_t ledPin = IO3;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  // The internal pull-up holds the input HIGH until the button is pressed.
  bool pressed = digitalRead(buttonPin) == LOW;
  digitalWrite(ledPin, pressed ? HIGH : LOW);
  delay(10);
}
