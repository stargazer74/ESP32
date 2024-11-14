#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <OneWire.h>
#include <DallasTemperature.h>

/* Conversion factor for micro seconds to minutes */
#define uS_TO_M_FACTOR 60000000
/* Time ESP32 will go to sleep (in minutes) */
#define TIME_TO_SLEEP 10

RTC_DATA_ATTR int bootCount = 0; /* boot count not deleted on deep sleep */

// WiFi Credentials
const char *ssid = "####";
const char *password = "####";

// GPIO where the DS18B20 is connected to
const int oneWireBus = 14;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// MQTT settings
const char *mqttServer = "docker-runner.privat";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void connectMQTT()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("eso32Client"))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      const char *payload = "Hello from ESP32! Boot count is: ";
      const char *count = std::to_string(bootCount).c_str();

      char *s = new char[strlen(payload) + strlen(count) + 1];
      strcpy(s, payload);
      strcat(s, count);

      client.publish("esp/wakeup", s);
      // ... and resubscribe
      // client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // disable brownout detector
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
First we configure the wake up source
We set our ESP32 to wake up every 5 seconds
*/
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_M_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi ...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  // Print ESP Local IP Address
  Serial.println(" connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);

  delay(1500);

  if (!client.connected())
  {
    connectMQTT();
  }
  delay(1500);

  sensors.begin();

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  int roundedTemperatureC = round(temperatureC);

  delay(5000);

  const char *temperature = std::to_string(roundedTemperatureC).c_str();

  client.publish("esp/temperature", temperature);

  const char *sleepMessage = "Going to sleep now";
  Serial.println(sleepMessage);
  client.publish("esp/goingdown", sleepMessage);
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop()
{
}