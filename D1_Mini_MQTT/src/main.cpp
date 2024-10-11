#include <Arduino.h>

void setup()
{
  Serial.begin(115200);
  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi ...");

}
void loop() {
  Serial.println("Foooo");
  delay(10000);
}