/* SPDX-License-Identifier: MIT */
#include <Arduino.h>

// Requires ADC support and a divided battery-sense signal on AIN0.
// Default 10-bit calibration for RAK4631 + RAK19007:
// raw = 169.30 * VBAT(V) + 1.17. See README.md for calibration details.
// This example reports voltage only; it does not shut down at low voltage.

void setup() {
  Serial.begin(115200);
  pinMode(AIN0, INPUT);
  analogReadResolution(10);
  delay(1000);
  Serial.println("AIN0 battery ADC (10-bit)");
}

void loop() {
  uint32_t sum = 0;
  for (unsigned int i = 0; i < 8; ++i) {
    int raw = analogRead(AIN0);
    if (raw < 0) {
      Serial.printf("AIN0 ADC error=%d\r\n", raw);
      delay(1000);
      return;
    }
    sum += (uint32_t)raw;
  }

  // Convert the average without discarding fractional ADC counts first.
  int64_t numerator = (int64_t)sum * 100000L - 8LL * 117000L;
  uint32_t battery_mv = numerator > 0 ?
      (uint32_t)(numerator / (8LL * 16930L)) : 0U;
  Serial.printf("AIN0 ADC raw_avg=%lu VBAT=%lu mV\r\n",
                (unsigned long)((sum + 4U) / 8U),
                (unsigned long)battery_mv);
  delay(1000);
}
