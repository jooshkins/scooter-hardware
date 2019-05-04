#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "poopssid"
#define STAPSK  "donkey"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = 10;

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266! Goto /on to turn on LED or /off to turn off LED");
}

void handleNotFound() {
  digitalWrite(led, 1);
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
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);

  server.on("/off", []() {
    digitalWrite(led, 0);
    server.send(200, "text/plain", "turned LED off!");
  });

  server.on("/on", []() {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "turned LED on!");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
