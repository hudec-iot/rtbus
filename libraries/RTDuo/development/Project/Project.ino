/* SPDX-License-Identifier: MPL-2.0 */
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)
// Relative to the generated source in build/<BOARD>/sketch/.
#define ROOT(path) STRINGIFY(../../../../../../../path)


/***** Project Example *****/ 
// #include ROOT(.project/hx711/sample.ino)

/***** BLE Example *****/ 
// #include ROOT(libraries/RTDuo/examples/BLE/Peripheral.ino)

/***** IO Example *****/ 
// #include ROOT(libraries/RTDuo/examples/IO/Analog/Analog.ino)
// #include ROOT(libraries/RTDuo/examples/IO/Digital/Digital.ino)

/***** Interface Example *****/ 
// #include ROOT(libraries/RTDuo/examples/Interface/Scale/Scale.ino)

#ifndef RTDUO_EXAMPLE

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  delay(1000);
  Serial.println("RTDuo serial echo + @RTBUS CLI");

  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, HIGH);

  
  pinMode(AIN1, OUTPUT);
  digitalWrite(AIN1, LOW);
  pinMode(IO2, OUTPUT);
  digitalWrite(IO2, LOW);
}

void loop() {
  while (Serial.available() > 0) {
    int value = Serial.read();

    if (value >= 0) {
      Serial.write((uint8_t)value);
    }
  }

  delay(10);
}

#endif
