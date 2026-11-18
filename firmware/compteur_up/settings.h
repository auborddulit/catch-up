// settings.h — Réglages persistants (mémoire NVS de l'ESP32).
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "up_core.h"

constexpr int NPROBE = 4;
enum ProbeId { P_TEMOIN = 0, P_BAIN1, P_BAIN2, P_BAIN3 };
extern const char* const PROBE_NAMES[NPROBE];

struct Settings {
  // réseau
  char wifiSsid[33] = "";
  char wifiPass[65] = "";
  char adminPass[33] = "pasteur";   // à changer à la mise en service !
  // notifications ntfy
  char ntfyServer[96] = "https://ntfy.sh";
  char ntfyTopic[64] = "";          // vide = notifications désactivées
  bool ntfyAlarms = true;
  bool ntfyShift = false;
  bool ntfyEnd = true;
  // calcul des UP
  float z = 6.94f;
  float tRef = 60.0f;
  float tCount = 60.0f;
  float target = 15.0f;     // cible par défaut proposée au démarrage d'un lot
  float upMax = 60.0f;
  float nominalS = 400.0f;  // 6 min 40
  float rAlarm = 0.85f;
  // modèle (bouteille virtuelle)
  float k1 = 0.0060f;
  float k2 = 0.0060f;
  char modelDate[24] = "";
  char modelSource[64] = "valeurs par défaut (non étalonné)";
  // alarme bain
  float bathMin = 64.0f;
  uint16_t bathMinDelayS = 60;
  // étalonnage des sondes : valeur = brute + offset
  float offset[NPROBE] = {0, 0, 0, 0};
  char calDate[24] = "";
  // valeurs par défaut du formulaire de lot
  char produit[48] = "";
  char operateur[32] = "";
  float t0 = 6.0f;
  // numérotation automatique des lots : préfixe + numéro sur 4 chiffres
  char lotPrefix[12] = "L";
  int32_t lotNext = 1;

  up::Params params() const;
  void toJson(JsonObject o, bool withSecrets) const;
  void fromJson(JsonObjectConst o);
};

extern Settings settings;
void settingsLoad();
void settingsSave();
// Prochain n° de lot proposé (ex. « L0128 ») et n° de lot formaté pour un numéro donné
String lotName(int32_t n);
// À appeler quand un lot démarre : si son n° suit le format automatique,
// le compteur passe au suivant.
void noteLotUsed(const String& lot);
