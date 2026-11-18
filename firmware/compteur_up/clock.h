// clock.h — Heure : Internet (NTP) quand disponible, horloge DS3231 sinon.
#pragma once
#include <Arduino.h>

void clockBegin();
void clockLoop();
bool clockValid();                 // heure plausible (après 2024)
bool clockRtcPresent();
uint32_t nowEpoch();               // secondes UTC
void clockSet(uint32_t epochUtc);  // réglage manuel (depuis le navigateur)
// "2026-11-18 14:32:05" (heure locale)
String fmtDateTime(uint32_t epoch);
// "14:32:05"
String fmtTime(uint32_t epoch);
// "20261118-1432"
String fmtStamp(uint32_t epoch);
