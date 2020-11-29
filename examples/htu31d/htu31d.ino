/***************************************************
  This is an example for the HTU31 Humidity & Temp Sensor

  Designed specifically to work with the HTU31 sensor from Adafruit
  ----> https://www.adafruit.com/products/4832

  These displays use I2C to communicate, 2 pins are required to
  interface
 ****************************************************/

#include <Wire.h>
#include "Adafruit_HTU31D.h"

Adafruit_HTU31D htu = Adafruit_HTU31D();
uint32_t timestamp;
bool heaterEnabled = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait till serial port opens
  }
  Serial.println("Adafruit HTU31D test");

  if (!htu.begin(0x41)) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  timestamp = millis();
}

void loop() {
  float temp = htu.readTemperature();
  float rel_hum = htu.readHumidity();
  Serial.print("Temp: "); Serial.print(temp); Serial.print(" C");
  Serial.print("\t\t");
  Serial.print("Humidity: "); Serial.print(rel_hum); Serial.println(" \%");

  // every 5 seconds
  if ((millis() - timestamp) > 5000) {
    // toggle the heater
    heaterEnabled = !heaterEnabled;
    if (heaterEnabled) {
      Serial.println("Turning on heater");
    } else {
      Serial.println("Turning off heater");
    }
    if (! htu.enableHeater(heaterEnabled)) {
      Serial.println("Command failed");
    }

    timestamp = millis();
  }

  delay(500);
}
