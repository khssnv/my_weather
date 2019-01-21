// Board: LOLIN(WEMOS) D1 R2 & mini
#include <stdio.h>
#include <TaskScheduler.h> // https://github.com/arkhipenko/TaskScheduler
#include <Wire.h>
#include <ESP8266WiFi.h> // esp8266 library compatable with LOLIN(Wemos) D1 R2 18650:
#include <WiFiClient.h>  // http://arduino.esp8266.com/stable/package_esp8266com_index.json
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "PMS.h" // https://github.com/fu-hsi/pms
#include "SparkFunHTU21D.h"


#ifndef STASSID
#define STASSID "SSID"
#define STAPSK  "PASSWD"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

HTU21D temp_humd_sensor;
PMS pms(Serial);
PMS::DATA data;
struct {
  float temperature_celsius = 0.0;
  float humidity_percentage = 0.0;
  uint16_t PM_1_0 = 0;
  uint16_t PM_2_5 = 0;
  uint16_t PM_10_0 = 0;
  int8_t error = -1; // -1 - undefined, 0 - OK, 1 - reading sensors
} measurements;
char msg[1024];

void cbUpdateMeasurements();
Task tUpdateMeasurements(5000, TASK_FOREVER, &cbUpdateMeasurements);
Scheduler scheduler;


void cbUpdateMeasurements() {
  measurements.error = 1;
  measurements.temperature_celsius = temp_humd_sensor.readTemperature();
  measurements.humidity_percentage = temp_humd_sensor.readHumidity();

  while (Serial.available()) { Serial.read(); }
  pms.requestRead();
  if (pms.readUntil(data)) {
    measurements.PM_1_0 = data.PM_AE_UG_1_0;
    measurements.PM_2_5 = data.PM_AE_UG_2_5;
    measurements.PM_10_0 = data.PM_AE_UG_10_0;
    measurements.error = 0;
  }
  else {
    measurements.error = -1;
  }
}


void handleRoot() {
  if (!measurements.error) {
    sprintf(msg, "Temperature: %.2f C<br>"
                 "Humidity: %.2f %%<br>"
                 "PM1.0: %d ug/m3<br>"
                 "PM2.5: %u ug/m3<br>"
                 "PM10: %u ug/m3<br>"
                 "<script>setTimeout(function(){ location.reload(); }, 5000);</script>",
                 measurements.temperature_celsius,
                 measurements.humidity_percentage,
                 measurements.PM_1_0,
                 measurements.PM_2_5,
                 measurements.PM_10_0);
    //Serial.println(msg);
    server.send(200, "text/html", msg);
  }
  else {
    server.send(500, "text/html", "Internal server error"
                "<script>setTimeout(function(){ location.reload(); }, 5000);</script>");
  }
}


void handleString() {
  if (!measurements.error) {
    sprintf(msg, "Temperature: %.2f C, "
                  "Humidity: %.2f %%, "
                  "PM1.0: %u ug/m3, "
                  "PM2.5: %u ug/m3, "
                  "PM10: %u ug/m3.",
                  measurements.temperature_celsius,
                  measurements.humidity_percentage,
                  measurements.PM_1_0,
                  measurements.PM_2_5,
                  measurements.PM_10_0);
    server.send(200, "text/plain", msg);
  }
  else {
    server.send(500, "text/plain", "Internal server error");
  }
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}


void setup(void) {
  Serial.begin(9600); // 9600 for PMS

  scheduler.init();
  scheduler.addTask(tUpdateMeasurements);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  MDNS.begin("esp8266");
  Serial.print("Connected to Wi-Fi with IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/string", handleString);
  server.onNotFound(handleNotFound);

  pms.passiveMode();
  pms.wakeUp();
  temp_humd_sensor.begin();
  tUpdateMeasurements.enable();
  server.begin();
}


void loop(void) {
  server.handleClient();
  MDNS.update();
  scheduler.execute();
}
