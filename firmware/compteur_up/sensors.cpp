// sensors.cpp
#include "sensors.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

static const uint8_t PINS[NPROBE] = {PIN_T_TEMOIN, PIN_T_BAIN1, PIN_T_BAIN2, PIN_T_BAIN3, PIN_T_AUX};
static OneWire* ow[NPROBE];
static DallasTemperature* dt[NPROBE];
static DeviceAddress addr[NPROBE];
static ProbeState st[NPROBE];
static uint32_t tRequest = 0;
static bool pending = false;
static uint8_t spike[NPROBE];

static bool detect(int i) {
  dt[i]->begin();
  if (!dt[i]->getAddress(addr[i], 0)) { st[i].present = false; return false; }
  dt[i]->setResolution(addr[i], 12);
  st[i].present = true;
  return true;
}

void sensorsBegin() {
  for (int i = 0; i < NPROBE; i++) {
    ow[i] = new OneWire(PINS[i]);
    dt[i] = new DallasTemperature(ow[i]);
    detect(i);
    dt[i]->setWaitForConversion(false);
    dt[i]->setCheckForConversion(false);
    st[i].badSinceMs = millis();
  }
}

static void markBad(int i, uint32_t now) {
  st[i].ok = false;
  st[i].errors++;
  if (!st[i].badSinceMs) st[i].badSinceMs = now ? now : 1;
}

bool sensorsUpdate() {
  uint32_t now = millis();
  if (!pending && (tRequest == 0 || now - tRequest >= SAMPLE_MS)) {
    for (int i = 0; i < NPROBE; i++) {
      if (!st[i].present) detect(i);   // branchement à chaud
      if (st[i].present) dt[i]->requestTemperatures();
    }
    tRequest = now;
    pending = true;
    return false;
  }
  if (pending && now - tRequest >= 800) {   // conversion 12 bits : 750 ms
    pending = false;
    for (int i = 0; i < NPROBE; i++) {
      if (!st[i].present) { markBad(i, now); continue; }
      float v = dt[i]->getTempC(addr[i]);
      // -127 : sonde absente ; 85 : valeur de mise sous tension (jamais réelle ici)
      if (v == DEVICE_DISCONNECTED_C || v == 85.0f || v < -30 || v > 110) {
        st[i].present = (v != DEVICE_DISCONNECTED_C);
        markBad(i, now);
        continue;
      }
      // rejet d'un pic isolé (> 8 °C en 2 s), accepté s'il se confirme
      if (st[i].ok && fabsf(v - st[i].raw) > 8.0f && spike[i] < 1) {
        spike[i]++;
        markBad(i, now);
        continue;
      }
      spike[i] = 0;
      st[i].raw = v;
      st[i].value = v + settings.offset[i];
      st[i].ok = true;
      st[i].badSinceMs = 0;
    }
    return true;
  }
  return false;
}

const ProbeState& probe(int i) { return st[i]; }

float probeBadSeconds(int i) {
  if (!st[i].badSinceMs) return 0;
  return (millis() - st[i].badSinceMs) / 1000.0f;
}
