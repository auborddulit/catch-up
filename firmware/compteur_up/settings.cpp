// settings.cpp
#include "settings.h"
#include <Preferences.h>

const char* const PROBE_NAMES[NPROBE] = {"Témoin", "Bain place 1 (entrée)", "Bain place 2",
                                         "Bain place 3 (sortie)", "Auxiliaire"};

Settings settings;

up::Params Settings::params() const {
  up::Params p;
  p.z = z; p.tRef = tRef; p.tCount = tCount; p.target = target; p.upMax = upMax;
  p.k1 = k1; p.k2 = k2; p.nominalS = nominalS; p.rAlarm = rAlarm;
  return p;
}

static void cp(char* dst, size_t n, JsonVariantConst v) {
  if (v.isNull()) return;
  strlcpy(dst, v.as<const char*>() ? v.as<const char*>() : "", n);
}
template <typename T> static void num(T& dst, JsonVariantConst v) {
  if (!v.isNull()) dst = v.as<T>();
}

void Settings::toJson(JsonObject o, bool withSecrets) const {
  o["wifiSsid"] = wifiSsid;
  o["wifiPass"] = withSecrets ? wifiPass : (wifiPass[0] ? "********" : "");
  o["adminPass"] = withSecrets ? adminPass : "********";
  o["ntfyServer"] = ntfyServer;
  o["ntfyTopic"] = ntfyTopic;
  o["ntfyAlarms"] = ntfyAlarms;
  o["ntfyShift"] = ntfyShift;
  o["ntfyEnd"] = ntfyEnd;
  o["z"] = z; o["tRef"] = tRef; o["tCount"] = tCount;
  o["target"] = target; o["upMax"] = upMax; o["nominalS"] = nominalS; o["rAlarm"] = rAlarm;
  o["k1"] = k1; o["k2"] = k2; o["modelDate"] = modelDate; o["modelSource"] = modelSource;
  o["bathMin"] = bathMin; o["bathMinDelayS"] = bathMinDelayS;
  JsonArray a = o["offset"].to<JsonArray>();
  for (int i = 0; i < NPROBE; i++) a.add(offset[i]);
  o["calDate"] = calDate;
  o["produit"] = produit;
  o["t0"] = t0;
}

void Settings::fromJson(JsonObjectConst o) {
  cp(wifiSsid, sizeof wifiSsid, o["wifiSsid"]);
  // les mots de passe masqués ne sont pas réécrits
  if (!o["wifiPass"].isNull() && strcmp(o["wifiPass"] | "", "********") != 0)
    cp(wifiPass, sizeof wifiPass, o["wifiPass"]);
  if (!o["adminPass"].isNull() && strcmp(o["adminPass"] | "", "********") != 0 &&
      strlen(o["adminPass"] | "") >= 4)
    cp(adminPass, sizeof adminPass, o["adminPass"]);
  cp(ntfyServer, sizeof ntfyServer, o["ntfyServer"]);
  cp(ntfyTopic, sizeof ntfyTopic, o["ntfyTopic"]);
  num(ntfyAlarms, o["ntfyAlarms"]); num(ntfyShift, o["ntfyShift"]); num(ntfyEnd, o["ntfyEnd"]);
  num(z, o["z"]); num(tRef, o["tRef"]); num(tCount, o["tCount"]);
  num(target, o["target"]); num(upMax, o["upMax"]); num(nominalS, o["nominalS"]);
  num(rAlarm, o["rAlarm"]);
  num(k1, o["k1"]); num(k2, o["k2"]);
  cp(modelDate, sizeof modelDate, o["modelDate"]);
  cp(modelSource, sizeof modelSource, o["modelSource"]);
  num(bathMin, o["bathMin"]); num(bathMinDelayS, o["bathMinDelayS"]);
  JsonArrayConst a = o["offset"];
  if (!a.isNull())
    for (int i = 0; i < NPROBE && i < (int)a.size(); i++) offset[i] = a[i].as<float>();
  cp(calDate, sizeof calDate, o["calDate"]);
  cp(produit, sizeof produit, o["produit"]);
  num(t0, o["t0"]);
  // garde-fous
  if (z < 1 || z > 30) z = 6.94f;
  if (k1 <= 0 || k1 > 1) k1 = 0.006f;
  if (k2 <= 0 || k2 > 1) k2 = 0.006f;
  if (nominalS < 30) nominalS = 30;
  if (target < 0.1f) target = 0.1f;
}

void settingsLoad() {
  Preferences pr;
  pr.begin("cup", true);
  String s = pr.getString("cfg", "");
  pr.end();
  if (s.isEmpty()) return;
  JsonDocument doc;
  if (deserializeJson(doc, s)) return;
  // les secrets sont stockés en clair dans la NVS (comme tout firmware Wi-Fi)
  settings.fromJson(doc.as<JsonObjectConst>());
  if (doc["wifiPass"].is<const char*>()) strlcpy(settings.wifiPass, doc["wifiPass"], sizeof settings.wifiPass);
  if (doc["adminPass"].is<const char*>()) strlcpy(settings.adminPass, doc["adminPass"], sizeof settings.adminPass);
}

void settingsSave() {
  JsonDocument doc;
  settings.toJson(doc.to<JsonObject>(), true);
  String s;
  serializeJson(doc, s);
  Preferences pr;
  pr.begin("cup", false);
  pr.putString("cfg", s);
  pr.end();
}
