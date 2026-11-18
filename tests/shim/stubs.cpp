// Remplacements des modules matériels pour la simulation sur PC.
#include "Arduino.h"
#include "SD.h"
#include "WiFi.h"
#include <ctime>
#include "sensors.h"
#include "clock.h"
#include "notify.h"
#include "io.h"

SerialStub Serial;
SDStub SD;
SPIClass SPI;
WiFiStub WiFi;
uint32_t g_ms = 1000;
uint32_t g_now = 1795000000;   // novembre 2026
uint32_t millis() { return g_ms; }

ProbeState g_probe[NPROBE];
const ProbeState& probe(int i) { return g_probe[i]; }
float probeBadSeconds(int i) { return g_probe[i].ok ? 0 : 60; }
void sensorsBegin() {}
bool sensorsUpdate() { return false; }

void clockBegin() { setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); tzset(); }
void clockLoop() {}
bool clockValid() { return true; }
bool clockRtcPresent() { return true; }
uint32_t nowEpoch() { return g_now; }
void clockSet(uint32_t e) { g_now = e; }
static String fmt(uint32_t e, const char* f) { time_t t = e; struct tm tmv; localtime_r(&t, &tmv); char b[32]; strftime(b, sizeof b, f, &tmv); return String(b); }
String fmtDateTime(uint32_t e) { return fmt(e, "%Y-%m-%d %H:%M:%S"); }
String fmtTime(uint32_t e) { return fmt(e, "%H:%M:%S"); }
String fmtStamp(uint32_t e) { return fmt(e, "%Y%m%d-%H%M"); }

int g_notifs = 0;
void notifyBegin() {}
void notify(const String& t, const String& b, uint8_t p, const char*, const NotifyAction* a, int na) {
  g_notifs++;
  printf("  [ntfy p%d] %s : %s", p, t.c_str(), b.c_str());
  for (int i = 0; i < na; i++) printf(" [%s → %s]", a[i].label, a[i].url.c_str());
  printf("\n");
}
void backgroundTick() {}
int notifyLastCode() { return 200; }

int g_clicks = 0;
void ioBegin() {}
Btn buttonPoll() { return Btn::None; }
void buzzerSet(Beep) {}
void buzzerClick() { g_clicks++; }
void buzzerLoop() {}
void ledBlink() {}
Key keyPoll() { return Key::None; }
