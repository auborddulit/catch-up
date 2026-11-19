// web.cpp
#include "web.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "config.h"
#include "app.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "storage.h"
#include "notify.h"
#include "io.h"
#include "report.h"
#include "web_ui.h"
#include "archive.h"
#include "bg.h"

static WebServer server(80);

static void sendJson(JsonDocument& d, int code = 200) {
  String s;
  serializeJson(d, s);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", s);
}

static void reply(const String& err) {
  JsonDocument d;
  d["ok"] = err.length() == 0;
  if (err.length()) d["error"] = err;
  sendJson(d, err.length() ? 400 : 200);
}

static bool body(JsonDocument& d) {
  if (!server.hasArg("plain")) return true;   // corps vide : objet vide
  return !deserializeJson(d, server.arg("plain"));
}

// Les actions de réglage demandent le mot de passe administrateur (en-tête X-Pass).
static bool admin() {
  if (server.header("X-Pass") == settings.adminPass) return true;
  JsonDocument d;
  d["ok"] = false;
  d["error"] = "Mot de passe administrateur incorrect";
  d["auth"] = true;
  sendJson(d, 401);
  return false;
}

static void hStatus() {
  JsonDocument d;
  app.statusJson(d.to<JsonObject>());
  sendJson(d);
}

static void hShift() { reply(app.shiftPressed("page web")); }
static void hUndo() { reply(app.undoShift()); }
static void hEnd() { reply(app.endEntries()); }
static void hRemove() { reply(app.removeLast()); }
static void hAck() { app.acknowledge(); reply(""); }

static void hSessionStart() {
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  reply(app.startSession(d["lot"] | "", d["produit"] | "", d["operateur"] | "", d["t0"] | NAN,
                         d["target"] | NAN));
}

static void hSessionClose() {
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  reply(app.closeSession(d["comment"] | "", d["force"] | false));
}

static void hComment() {
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  reply(app.addComment(d["dir"] | "", d["comment"] | ""));
}

static void hMesureStart() {
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  reply(app.startMesure(d["notes"] | ""));
}

static void hMesureStop() {
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  reply(app.stopMesure(d["comment"] | ""));
}

static void listDir(JsonArray arr, const char* base) {
  File root = SD.open(base);
  if (!root) return;
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    if (!e.isDirectory()) continue;
    String dir = String(base) + "/" + e.name();
    JsonDocument m;
    String s = readText(dir + "/meta.json", 4096);
    JsonObject x = arr.add<JsonObject>();
    x["dir"] = dir;
    x["id"] = e.name();
    if (s.length() && !deserializeJson(m, s)) {
      x["type"] = m["type"];
      x["lot"] = m["lot"];
      x["debut"] = m["debut"];
      x["statut"] = m["statut"];
      x["caisses"] = m["caisses"];
      x["upMin"] = m["upMin"];
      x["upBain"] = m["upBain"];
      x["notes"] = m["notes"];
    }
    x["aSauver"] = archiveIsPending(dir);
  }
}

static void hHistory() {
  JsonDocument d;
  JsonArray a = d["items"].to<JsonArray>();
  if (sdOk()) {
    listDir(a, "/sessions");
    listDir(a, "/mesures");
  }
  d["sd"] = sdOk();
  sendJson(d);
}

static void hFile() {
  String p = server.arg("p");
  if (p.indexOf("..") >= 0 || !(p.startsWith("/sessions/") || p.startsWith("/mesures/"))) {
    server.send(403, "text/plain", "Interdit");
    return;
  }
  File f = SD.open(p, FILE_READ);
  if (!f || f.isDirectory()) { server.send(404, "text/plain", "Introuvable"); return; }
  String type = "text/plain; charset=utf-8";
  if (p.endsWith(".html")) type = "text/html; charset=utf-8";
  else if (p.endsWith(".json")) type = "application/json";
  else if (p.endsWith(".csv")) {
    type = "text/csv; charset=utf-8";
    // nom de téléchargement : <dossier>_<fichier>.csv
    int a = p.lastIndexOf('/'), b = p.lastIndexOf('/', a - 1);
    server.sendHeader("Content-Disposition",
                      "attachment; filename=\"" + p.substring(b + 1, a) + "_" + p.substring(a + 1) + "\"");
  }
  // envoi par blocs : le bouton DÉCALAGE et les mesures restent actifs
  server.setContentLength(f.size());
  server.send(200, type, "");
  static uint8_t buf[1024];
  size_t n;
  while ((n = f.read(buf, sizeof buf)) > 0) {
    server.sendContent((const char*)buf, n);
    if (!server.client().connected()) break;
    backgroundTick();
  }
  f.close();
}

static void hZip() {
  String d = server.arg("d");
  if (d.indexOf("..") >= 0 || !(d.startsWith("/sessions/") || d.startsWith("/mesures/")) || !SD.exists(d)) {
    server.send(404, "text/plain", "Lot introuvable");
    return;
  }
  if (app.mode != Mode::Idle && d == app.meta.dir) {
    server.send(409, "text/plain; charset=utf-8", "Lot en cours : le télécharger après la clôture");
    return;
  }
  size_t total = zipFolder(d, [](const uint8_t*, size_t) { return true; }, true);
  if (!total) { server.send(500, "text/plain", "Archive impossible"); return; }
  server.setContentLength(total);
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + zipName(d) + "\"");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/zip", "");
  size_t sent = zipFolder(d, [](const uint8_t* b, size_t n) {
    server.sendContent((const char*)b, n);
    return (bool)server.client().connected();
  });
  if (sent == total && server.client().connected()) {
    if (d.startsWith("/sessions/")) {
      appendLine(d + "/events.csv", fmtDateTime(nowEpoch()) + ",SAUVEGARDE,,Sauvegarde .zip téléchargée");
    }
    archiveDownloaded(d);
  }
}

static void hFit() {
  JsonDocument d, out;
  if (!body(d)) return reply("Requête invalide");
  if (app.mode != Mode::Idle) return reply("Arrêter la session ou la mesure en cours d'abord");
  String err = fitFromDir(d["dir"] | "", out.to<JsonObject>());
  if (err.length()) out["error"] = err;
  sendJson(out, err.length() && !out["k1"].is<float>() ? 400 : 200);
}

static void hModel() {
  if (!admin()) return;
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  float k1 = d["k1"] | 0.0f, k2 = d["k2"] | 0.0f;
  if (!(k1 > 0 && k1 < 1 && k2 > 0 && k2 < 1)) return reply("Valeurs invalides");
  if (app.mode != Mode::Idle) return reply("Impossible pendant une session ou une mesure");
  settings.k1 = k1;
  settings.k2 = k2;
  strlcpy(settings.modelSource, d["source"] | "réglage manuel", sizeof settings.modelSource);
  String date = fmtDateTime(nowEpoch()).substring(0, 10);
  strlcpy(settings.modelDate, date.c_str(), sizeof settings.modelDate);
  settingsSave();
  reply("");
}

static void hSettingsGet() {
  if (!admin()) return;
  JsonDocument d;
  settings.toJson(d.to<JsonObject>(), false);
  sendJson(d);
}

static void hSettingsPost() {
  if (!admin()) return;
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  if (app.mode != Mode::Idle) return reply("Réglages verrouillés pendant une session ou une mesure");
  String oldSsid = settings.wifiSsid;
  settings.fromJson(d.as<JsonObjectConst>());
  settingsSave();
  JsonDocument r;
  r["ok"] = true;
  r["reboot"] = oldSsid != settings.wifiSsid || d["wifiPass"].is<const char*>();
  sendJson(r);
}

static void hCalib() {
  if (!admin()) return;
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  if (app.mode != Mode::Idle) return reply("Étalonnage impossible pendant une session ou une mesure");
  if (d["reset"] | false) {
    for (int i = 0; i < NPROBE; i++) settings.offset[i] = 0;
    settings.calDate[0] = 0;
    settingsSave();
    return reply("");
  }
  float ref = d["ref"] | NAN;
  if (std::isnan(ref)) return reply("Température de référence manquante");
  JsonArrayConst sel = d["sel"];
  int n = 0;
  String err;
  for (JsonVariantConst v : sel) {
    int i = v.as<int>();
    if (i < 0 || i >= NPROBE) continue;
    const ProbeState& p = probe(i);
    if (!p.ok) { err += String(PROBE_NAMES[i]) + " invalide. "; continue; }
    float off = ref - p.raw;
    if (fabsf(off) > 3) { err += String(PROBE_NAMES[i]) + " : écart > 3 °C refusé. "; continue; }
    settings.offset[i] = off;
    n++;
  }
  if (n) {
    String date = fmtDateTime(nowEpoch());
    strlcpy(settings.calDate, date.c_str(), sizeof settings.calDate);
    settingsSave();
  }
  reply(n ? "" : (err.length() ? err : String("Aucune sonde sélectionnée")));
}

static void hTime() {
  // réglage libre tant que l'heure n'est pas valide ; ensuite mot de passe requis
  if (clockValid() && !admin()) return;
  JsonDocument d;
  if (!body(d)) return reply("Requête invalide");
  uint32_t e = d["epoch"] | 0UL;
  if (e < 1704067200UL) return reply("Heure invalide");
  if (app.mode != Mode::Idle && clockValid()) return reply("Impossible pendant une session ou une mesure");
  clockSet(e);
  reply("");
}

static void hTestNtfy() {
  if (!admin()) return;
  notify("Test Catch-UP", "Les notifications fonctionnent.", 3, "bell");
  reply("");
}

static void hTestBuzzer() { buzzerClick(); reply(""); }

static void hReboot() {
  if (!admin()) return;
  reply("");
  delay(300);
  ESP.restart();
}

void webBegin() {
  const char* hdrs[] = {"X-Pass"};
  server.collectHeaders(hdrs, 1);
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", WEB_UI);
  });
  server.on("/api/status", HTTP_GET, hStatus);
  server.on("/api/shift", HTTP_POST, hShift);
  server.on("/api/undo", HTTP_POST, hUndo);
  server.on("/api/endentries", HTTP_POST, hEnd);
  server.on("/api/removelast", HTTP_POST, hRemove);
  server.on("/api/ack", HTTP_POST, hAck);
  server.on("/api/session/start", HTTP_POST, hSessionStart);
  server.on("/api/session/close", HTTP_POST, hSessionClose);
  server.on("/api/comment", HTTP_POST, hComment);
  server.on("/api/mesure/start", HTTP_POST, hMesureStart);
  server.on("/api/mesure/stop", HTTP_POST, hMesureStop);
  server.on("/api/history", HTTP_GET, hHistory);
  server.on("/f", HTTP_GET, hFile);
  server.on("/zip", HTTP_GET, hZip);
  server.on("/api/fit", HTTP_POST, hFit);
  server.on("/api/model", HTTP_POST, hModel);
  server.on("/api/settings", HTTP_GET, hSettingsGet);
  server.on("/api/settings", HTTP_POST, hSettingsPost);
  server.on("/api/calib", HTTP_POST, hCalib);
  server.on("/api/time", HTTP_POST, hTime);
  server.on("/api/test/ntfy", HTTP_POST, hTestNtfy);
  server.on("/api/test/buzzer", HTTP_POST, hTestBuzzer);
  server.on("/api/reboot", HTTP_POST, hReboot);
  server.onNotFound([] { server.send(404, "text/plain", "Introuvable"); });
  server.begin();
}

void webLoop() { server.handleClient(); }
