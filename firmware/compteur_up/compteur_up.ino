// compteur_up.ino — Compteur d'unités de pasteurisation (UP) open source pour ESP32.
//
// Une bouteille témoin mesure les UP réelles au cœur du produit ; trois sondes
// dans le bain alimentent une « bouteille virtuelle » par caisse pour indiquer
// quand décaler les caisses dans un tunnel manuel à 3 places.
//
// Projet : https://github.com/auborddulit/compteur-UP       Licence : MIT
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "storage.h"
#include "notify.h"
#include "io.h"
#include "display.h"
#include "web.h"
#include "app.h"

static void wifiBegin() {
  WiFi.setHostname(DEVICE_NAME);
  WiFi.setAutoReconnect(true);
  if (settings.wifiSsid[0]) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifiSsid, settings.wifiPass);
    displayMessage("Connexion Wi-Fi", settings.wifiSsid);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    // réseau de secours : se connecter à « CompteurUP » puis http://192.168.4.1
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    if (settings.wifiSsid[0]) WiFi.begin(settings.wifiSsid, settings.wifiPass);  // continue d'essayer
  }
  MDNS.begin(DEVICE_NAME);
  MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\ncompteur-up v" FW_VERSION);
  settingsLoad();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  ioBegin();
  displayBegin();
  displayMessage("Compteur UP", "v" FW_VERSION);
  if (!sdBegin()) Serial.println("Carte SD absente");
  clockBegin();
  sensorsBegin();
  wifiBegin();
  notifyBegin();
  webBegin();
  app.begin();
  Serial.printf("Prêt : http://%s.local  (IP %s)\n", DEVICE_NAME,
                WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str()
                                              : WiFi.softAPIP().toString().c_str());
}

void loop() {
  Btn b = buttonPoll();
  if (b == Btn::Short) app.onButton(false);
  else if (b == Btn::Long) app.onButton(true);
  if (sensorsUpdate()) app.onSample();
  webLoop();
  clockLoop();
  app.loop();
  buzzerLoop();
  displayLoop();
  delay(2);
}
