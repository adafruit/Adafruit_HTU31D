#include <Adafruit_SH110X.h>
#include <Adafruit_HTU31D.h>
#include <Fonts/FreeSans9pt7b.h>

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_HTU31D htu = Adafruit_HTU31D();

void setup() {
  Serial.begin(115200);
  //while (!Serial);

  Serial.println("HTU31 OLED test");

  Serial.println("128x64 OLED FeatherWing test");
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");
  display.display();
  delay(500); // Pause for half second

  if (htu.begin(0x40)) {
    Serial.println("Found HTU31");
  } else {
    Serial.println("Didn't find HTU31");
    while (1);
  }

  display.setRotation(1);
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(SH110X_WHITE);
}

void loop() {
  display.clearDisplay();
  sensors_event_t humidity, temp;

  htu.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  display.setCursor(0,20);
  display.print("HTU31 Demo");
  display.setCursor(0,40);
  display.print("Temp: "); display.print(temp.temperature); display.println(" C");
  display.setCursor(0,60);
  display.print("Hum: "); display.print(humidity.relative_humidity); display.println(" %");
  Serial.print("Temperature: ");Serial.print(temp.temperature);Serial.println(" degrees C");
  Serial.print("Pressure: ");Serial.print(humidity.relative_humidity);Serial.println(" RH %");

  yield();
  display.display();
  delay(100);
}
