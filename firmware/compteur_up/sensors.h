// sensors.h — Lecture des 5 sondes DS18B20 (un bus 1-Wire par sonde).
#pragma once
#include <Arduino.h>
#include "settings.h"

struct ProbeState {
  float raw = NAN;      // valeur brute (°C)
  float value = NAN;    // valeur étalonnée (°C)
  bool ok = false;      // mesure valide à ce pas
  bool present = false; // sonde détectée sur le bus
  uint32_t badSinceMs = 0;  // début de la période invalide (0 = valide)
  uint32_t errors = 0;
};

void sensorsBegin();
// À appeler dans loop(). Renvoie true quand un nouveau jeu de mesures est prêt.
bool sensorsUpdate();
const ProbeState& probe(int i);
// secondes depuis lesquelles la sonde est invalide (0 si valide)
float probeBadSeconds(int i);
