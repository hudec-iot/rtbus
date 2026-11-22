/* SPDX-License-Identifier: MIT */
#include <Arduino.h>

// Connect HX711 DOUT to dataPin and PD_SCK to clockPin, with a shared GND.
// Use a module supply and logic levels compatible with your board's GPIOs.
// Change the pin constants to match the GPIOs exposed by your board.
const uint32_t dataPin = IO1;
const uint32_t clockPin = IO2;
const unsigned long readyTimeoutMs = 2000;
const unsigned int sampleCount = 8;
const long calibrationMassGrams = 100;

static int32_t zeroOffset;
static int32_t calibrationCounts;
static bool hasTare;

static bool readSample(int32_t &sample) {
  unsigned long started = millis();
  while (digitalRead(dataPin) != LOW) {
    if (millis() - started >= readyTimeoutMs) {
      return false;
    }
    delay(1);
  }

  uint32_t bits = 0;
  // Keep application callbacks out of the clock sequence.
  noInterrupts();
  for (unsigned int bit = 0; bit < 24; ++bit) {
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(1);
    bits = (bits << 1) | (digitalRead(dataPin) == HIGH ? 1U : 0U);
    digitalWrite(clockPin, LOW);
    delayMicroseconds(1);
  }

  // The 25th pulse selects channel A at gain 128 for the next conversion.
  digitalWrite(clockPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(clockPin, LOW);
  interrupts();

  sample = (int32_t)bits;
  if (bits & 0x800000U) {
    sample -= 0x1000000;
  }
  return true;
}

void setup() {
  pinMode(clockPin, OUTPUT);
  digitalWrite(clockPin, LOW);
  pinMode(dataPin, INPUT);
  Serial.begin(115200);
  Serial.println("HX711 scale");
  Serial.println("Remove all weight, then send t to tare.");
  Serial.printf("Place a %ld g reference mass, then send c to calibrate.\r\n",
                calibrationMassGrams);
}

void loop() {
  // Ignore line endings sent by the Serial Monitor.
  int command = 0;
  while (Serial.available() > 0) {
    int next = Serial.read();
    if (next == 't' || next == 'c') {
      command = next;
      break;
    }
  }

  int32_t sum = 0;
  for (unsigned int i = 0; i < sampleCount; ++i) {
    int32_t sample;
    if (!readSample(sample)) {
      Serial.println("HX711 timed out waiting for DOUT.");
      return;
    }
    sum += sample;
  }
  int32_t average = sum / (int32_t)sampleCount;

  if (command == 't') {
    zeroOffset = average;
    hasTare = true;
    Serial.println("Tare set.");
  } else if (command == 'c') {
    if (!hasTare) {
      Serial.println("Tare before calibrating.");
    } else if (average == zeroOffset) {
      Serial.println("Calibration failed. Check the reference mass.");
    } else {
      calibrationCounts = average - zeroOffset;
      Serial.println("Calibration set.");
    }
  }

  Serial.printf("Raw: %ld", (long)average);
  if (calibrationCounts != 0) {
    double grams = (double)(average - zeroOffset) * calibrationMassGrams /
                   calibrationCounts;
    Serial.print(", weight (g): ");
    Serial.println(grams, 1);
  } else {
    Serial.println();
  }
}
