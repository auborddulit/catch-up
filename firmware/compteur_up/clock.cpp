// clock.cpp
#include "clock.h"
#include <RTClib.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include "config.h"

static RTC_DS3231 rtc;
static bool rtcOk = false;
static volatile bool ntpSynced = false;
static bool rtcWritten = false;

static void onSync(struct timeval*) { ntpSynced = true; }

static void setSystem(uint32_t e) {
  struct timeval tv = {(time_t)e, 0};
  settimeofday(&tv, nullptr);
}

void clockBegin() {
  setenv("TZ", TZ_PARIS, 1);
  tzset();
  rtcOk = rtc.begin();   // Wire déjà initialisé
  if (rtcOk && !rtc.lostPower()) {
    uint32_t e = rtc.now().unixtime();   // la DS3231 est tenue en UTC
    if (e > 1704067200UL) setSystem(e);
  }
  sntp_set_time_sync_notification_cb(onSync);
  configTzTime(TZ_PARIS, "pool.ntp.org", "time.google.com");
}

void clockLoop() {
  if (ntpSynced && !rtcWritten) {
    if (rtcOk) rtc.adjust(DateTime((uint32_t)time(nullptr)));
    rtcWritten = true;
  }
}

bool clockValid() { return time(nullptr) > 1704067200; }  // 1er janvier 2024
bool clockRtcPresent() { return rtcOk; }
uint32_t nowEpoch() { return (uint32_t)time(nullptr); }

void clockSet(uint32_t e) {
  setSystem(e);
  if (rtcOk) rtc.adjust(DateTime(e));
}

static String fmt(uint32_t e, const char* f) {
  time_t t = e;
  struct tm tmv;
  localtime_r(&t, &tmv);
  char b[32];
  strftime(b, sizeof b, f, &tmv);
  return String(b);
}
String fmtDateTime(uint32_t e) { return fmt(e, "%Y-%m-%d %H:%M:%S"); }
String fmtTime(uint32_t e) { return fmt(e, "%H:%M:%S"); }
String fmtStamp(uint32_t e) { return fmt(e, "%Y%m%d-%H%M"); }
