#pragma once
#include "Arduino.h"
#define WL_CONNECTED 3
struct IP { String toString() const { return "192.168.1.50"; } };
struct WiFiStub { int status() { return WL_CONNECTED; } IP localIP() { return IP(); } IP softAPIP() { return IP(); } int RSSI() { return -60; } };
extern WiFiStub WiFi;
