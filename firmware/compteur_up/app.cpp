// app.cpp
#include "app.h"
#include <WiFi.h>
#include "config.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "storage.h"
#include "notify.h"
#include "io.h"
#include "report.h"

App app;

static const char* STATE_PATH = "/state.bin";
static const uint32_t STATE_MAGIC = 0x43555031;  // "CUP1"

struct StateHeader {
  uint32_t magic;
  uint32_t metaSize;
  uint32_t engineSize;
  uint32_t savedAt;
  uint8_t mode;
};

static String f1(float v, int d = 1) {
  if (std::isnan(v)) return String("");
  return String(v, d);
}

// ---------------------------------------------------------------------------
void App::begin() {
  restore();   // reprend une session ou une mesure interrompue par une coupure
}

// ---------------------------------------------------------------------------
// Journal
// ---------------------------------------------------------------------------
void App::logEvent(const char* type, int crate, const String& msg) {
  uint32_t now = nowEpoch();
  String line = fmtTime(now) + "  " + msg;
  logRam[logHead] = line;
  logHead = (logHead + 1) % LOG_RAM;
  if (logCount < LOG_RAM) logCount++;
  if (mode != Mode::Idle && meta.dir[0]) {
    appendLine(String(meta.dir) + "/events.csv",
               fmtDateTime(now) + "," + type + "," + (crate > 0 ? String(crate) : String("")) + "," +
                   csvField(msg));
  }
  Serial.println(line);
}

// ---------------------------------------------------------------------------
// Alarmes
// ---------------------------------------------------------------------------
void App::raise(AlarmId id, const String& msg, bool sounding) {
  Alarm& a = alarms[id];
  a.active = true;
  a.latched = true;
  a.sounding = sounding;
  a.since = nowEpoch();
  a.msg = msg;
  if (sounding) silencedUntilMs = 0;  // une nouvelle alarme réarme le buzzer
  logEvent("ALARME", -1, msg);
  if (settings.ntfyAlarms)
    notify(String("Alarme – ") + (meta.lot[0] ? meta.lot : "pasteurisation"), msg,
           sounding ? 5 : 3, "warning");
}

void App::setCond(AlarmId id, bool active, const String& msg, bool sounding) {
  Alarm& a = alarms[id];
  if (active && !a.active) {
    a.active = true;
    a.latched = false;
    a.sounding = sounding;
    a.since = nowEpoch();
    a.msg = msg;
    if (sounding) silencedUntilMs = 0;
    logEvent("ALARME", -1, msg);
    if (settings.ntfyAlarms)
      notify(String("Alarme – ") + (meta.lot[0] ? meta.lot : "pasteurisation"), msg,
             sounding ? 5 : 3, "warning");
  } else if (active) {
    a.msg = msg;  // mise à jour du texte (valeur courante)
  } else if (!active && a.active && !a.latched) {
    a.active = false;
    logEvent("FIN_ALARME", -1, a.msg);
  }
}

bool App::anySounding() const {
  for (int i = 0; i < AL_COUNT; i++)
    if (alarms[i].active && alarms[i].sounding) return true;
  return false;
}

const Alarm* App::topAlarm() const {
  for (int i = 0; i < AL_COUNT; i++)
    if (alarms[i].active && alarms[i].sounding) return &alarms[i];
  for (int i = 0; i < AL_COUNT; i++)
    if (alarms[i].active) return &alarms[i];
  return nullptr;
}

void App::acknowledge() {
  bool any = false;
  for (int i = 0; i < AL_COUNT; i++) {
    if (alarms[i].active && alarms[i].latched) { alarms[i].active = false; any = true; }
  }
  silencedUntilMs = millis() + SILENCE_MS;
  if (silencedUntilMs == 0) silencedUntilMs = 1;
  logEvent("ACQUIT", -1, any ? "Alarmes acquittées" : "Buzzer coupé 5 min");
}

void App::updateAlarms() {
  uint32_t now = nowEpoch();
  bool running = mode != Mode::Idle;
  // sondes du bain
  for (int s = 0; s < up::NPOS; s++) {
    const ProbeState& p = probe(P_BAIN1 + s);
    bool bad = running && probeBadSeconds(P_BAIN1 + s) > 10;
    setCond((AlarmId)(AL_PROBE_B1 + s), bad,
            String("Sonde bain place ") + (s + 1) + " défaillante (repli sur la plus froide)");
    // bain trop froid
    bool low = running && p.ok && p.value < settings.bathMin;
    if (low) { if (!belowSince[s]) belowSince[s] = now; }
    else belowSince[s] = 0;
    bool al = low && (now - belowSince[s]) >= settings.bathMinDelayS;
    setCond((AlarmId)(AL_BATH1 + s), al,
            String("Bain place ") + (s + 1) + " trop froid : " + f1(p.value) + " °C < " +
                f1(settings.bathMin) + " °C");
  }
  // sonde témoin : utile en mesure, et en session tant que la caisse témoin est au bain
  bool needT = false;
  if (mode == Mode::Mesure) needT = true;
  if (mode == Mode::Session && sess.nCrates > 0 && sess.crates[0].pos >= 0 &&
      !sess.crates[0].witnessFault) needT = true;
  setCond(AL_PROBE_T, needT && probeBadSeconds(P_TEMOIN) > 10, "Sonde témoin : mesure invalide");
  setCond(AL_SD, running && !sdOk(), "Carte SD absente : enregistrement impossible");
}

void App::updateBuzzer() {
  bool silenced = silencedUntilMs && (int32_t)(millis() - silencedUntilMs) < 0;
  if (!silenced) silencedUntilMs = 0;
  if (anySounding() && !silenced) buzzerSet(Beep::Alarm);
  else if (ready) buzzerSet(Beep::Shift);
  else buzzerSet(Beep::Off);
}

// ---------------------------------------------------------------------------
// Événements des moteurs de calcul
// ---------------------------------------------------------------------------
void App::drainEvents() {
  up::Event e;
  if (mode == Mode::Session) {
    while (sess.ev.pop(e)) {
      const up::Crate* c = (e.crate > 0 && e.crate <= sess.nCrates) ? &sess.crates[e.crate - 1] : nullptr;
      switch (e.type) {
        case up::Ev::CrateIn:
          logEvent("ENTREE", e.crate, String("Caisse ") + e.crate + " entrée en place 1" +
                                          (c && c->witness ? " (avec le témoin)" : ""));
          break;
        case up::Ev::CrateOut:
          logEvent("SORTIE", e.crate,
                   String("Caisse ") + e.crate + " sortie : " + f1(e.value, 2) + " UP " +
                       (c && c->measured ? "mesurées" : "estimées") + " – " +
                       (e.value2 > 0.5f ? "CONFORME" : "NON CONFORME"));
          break;
        case up::Ev::EarlyExit:
          raise(AL_EARLY, String("Caisse ") + e.crate + " sortie avant la cible : " + f1(e.value, 2) +
                              " UP < " + f1(sess.p.target, 1) + " – isoler la caisse");
          break;
        case up::Ev::OverMax:
          raise(AL_OVERMAX, String("Caisse ") + e.crate + " au-dessus du maximum : " + f1(e.value, 1) +
                                " UP > " + f1(sess.p.upMax, 0), false);
          break;
        case up::Ev::WitnessResult: {
          String m = String("Témoin (caisse 1) : ") + f1(e.value, 2) + " UP mesurées, modèle " +
                     f1(e.value2, 2) + " UP, rapport " + f1(sess.r, 2) +
                     (sess.r < 1 ? " → cible des caisses suivantes durcie" : " → pas de correction");
          logEvent("TEMOIN", e.crate, m);
          notify(String("Témoin – lot ") + meta.lot, m, 3, "thermometer");
          break;
        }
        case up::Ev::RatioLow:
          raise(AL_RATIO, String("Écart témoin / modèle important (rapport ") + f1(e.value, 2) +
                              ") : refaire un étalonnage en mode mesure");
          break;
        case up::Ev::WitnessFault:
          raise(AL_WITNESS, "Témoin défaillant : la caisse 1 est évaluée par le modèle, lot à vérifier");
          break;
        case up::Ev::BathLost:
          raise(AL_BATHLOST, String("Caisse ") + e.crate + " : aucune sonde de bain valide, UP non comptées");
          break;
        case up::Ev::Undo:
          logEvent("ANNULATION", -1, "Dernier décalage annulé");
          break;
        case up::Ev::Removed:
          logEvent("RETRAIT", e.crate, String("Caisse ") + e.crate + " retirée du suivi (caisse vide, appui de trop)");
          break;
        case up::Ev::EntriesEnded:
          logEvent("FIN_ENTREES", -1, "Fin des entrées de caisses");
          break;
        case up::Ev::Done:
          logEvent("TERMINE", -1, "Toutes les caisses sont sorties : clôturer la session");
          if (settings.ntfyEnd)
            notify(String("Lot ") + meta.lot, "Toutes les caisses sont sorties. Clôturer la session.", 3,
                   "checkered_flag");
          break;
        default: break;
      }
    }
  } else if (mode == Mode::Mesure) {
    while (mes.ev.pop(e)) {
      if (e.type == up::Ev::MeasureMove) {
        if (e.crate >= 0) logEvent("TEMOIN", -1, String("Témoin en place ") + (e.crate + 1));
        else logEvent("TEMOIN", -1, String("Témoin sorti du bain : ") + f1(mes.upBath, 2) + " UP (bain)");
      } else if (e.type == up::Ev::Undo) {
        logEvent("ANNULATION", -1, "Dernier appui annulé");
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Mesures
// ---------------------------------------------------------------------------
void App::onSample() {
  ledBlink();
  for (int s = 0; s < up::NPOS; s++) {
    const ProbeState& p = probe(P_BAIN1 + s);
    in.bath[s] = p.value;
    in.bathOk[s] = p.ok;
  }
  in.witness = probe(P_TEMOIN).value;
  in.witnessOk = probe(P_TEMOIN).ok;
  in.aux = probe(P_AUX).value;
  in.auxOk = probe(P_AUX).ok;

  uint32_t ms = millis();
  float dt = lastSampleMs ? (ms - lastSampleMs) / 1000.0f : 0;
  lastSampleMs = ms;
  uint32_t now = nowEpoch();

  if (mode == Mode::Session) {
    sess.step(now, dt, in);
    drainEvents();
    ready = sess.ready(now);
    eta = sess.etaSeconds(now, in);
  } else if (mode == Mode::Mesure) {
    mes.step(now, dt, in);
    drainEvents();
    // minuteur de décalage nominal pour reproduire la cadence habituelle
    bool inBath = mes.pos >= 0;
    float el = inBath ? (float)(now - mes.lastPress) : 0;
    ready = inBath && el >= settings.nominalS;
    eta = inBath ? fmaxf(0, settings.nominalS - el) : -1;
  } else {
    ready = false;
    eta = -1;
  }
  if (ready && !lastReady && settings.ntfyShift && mode == Mode::Session)
    notify(String("Lot ") + meta.lot, "Décaler maintenant", 4, "arrow_left");
  lastReady = ready;

  updateAlarms();
  if (mode != Mode::Idle) {
    writeDataLine();
    if (now - lastPersist >= PERSIST_EVERY_S) persist();
  }
  updateBuzzer();
}

void App::writeDataLine() {
  uint32_t now = nowEpoch();
  String l;
  l.reserve(120);
  l += fmtDateTime(now); l += ',';
  l += String(now - meta.tStart); l += ',';
  l += in.witnessOk ? f1(in.witness, 2) : String(""); l += ',';
  for (int s = 0; s < up::NPOS; s++) { l += in.bathOk[s] ? f1(in.bath[s], 2) : String(""); l += ','; }
  l += in.auxOk ? f1(in.aux, 2) : String("");
  if (mode == Mode::Session) {
    for (int s = 0; s < up::NPOS; s++) {
      l += ',';
      if (sess.slot[s] >= 0) l += String(sess.crates[sess.slot[s]].id);
    }
    l += ',';
    if (sess.slot[up::NPOS - 1] >= 0) l += f1(sess.upNow(sess.crates[sess.slot[up::NPOS - 1]]), 3);
    l += ',';
    if (sess.nCrates > 0 && sess.crates[0].pos >= 0) l += f1(sess.crates[0].upMeas, 3);
  } else {
    int place = mes.pos == -2 ? 0 : (mes.pos == -1 ? 4 : mes.pos + 1);
    l += ','; l += String(place);
    l += ','; l += f1(mes.upBath, 3);
    l += ','; l += f1(mes.upTotal, 3);
    l += ','; l += f1(mes.upPred, 3);
    l += ','; l += f1(mes.upAuxBath, 3);
    l += ','; l += f1(mes.upAuxTotal, 3);
  }
  appendLine(String(meta.dir) + "/data.csv", l);
}

// ---------------------------------------------------------------------------
// Bouton
// ---------------------------------------------------------------------------
void App::onButton(bool longPress) {
  if (longPress) { acknowledge(); buzzerClick(); return; }
  String err = shiftPressed("bouton");
  if (err.length()) logEvent("INFO", -1, err);
}

String App::shiftPressed(const char* source) {
  uint32_t now = nowEpoch();
  if (mode == Mode::Session) {
    if (sess.finished()) return "Session terminée : la clôturer";
    if (sess.nCrates >= up::MAX_CRATES && !sess.entriesEnded) return "Nombre maximum de caisses atteint";
    logEvent("DECALAGE", -1, String("Décalage (") + source + ")");
    sess.shift(now);
    drainEvents();
    ready = sess.ready(now);
    lastReady = ready;
    buzzerClick();
    persist();
    updateBuzzer();
    return "";
  }
  if (mode == Mode::Mesure) {
    if (mes.pos == -1) return "Témoin déjà sorti : arrêter la mesure";
    logEvent("DECALAGE", -1, String("Appui (") + source + ")");
    mes.press(now);
    drainEvents();
    ready = false;
    lastReady = false;
    buzzerClick();
    persist();
    updateBuzzer();
    return "";
  }
  return "Aucune session ni mesure en cours";
}

String App::undoShift() {
  uint32_t now = nowEpoch();
  bool ok = false;
  if (mode == Mode::Session) ok = sess.undo(now, UNDO_MAX_S);
  else if (mode == Mode::Mesure) ok = mes.undo(now, UNDO_MAX_S);
  else return "Rien à annuler";
  if (!ok) return "Annulation impossible (plus de 60 s, ou déjà annulé)";
  drainEvents();
  // une sortie « trop tôt » annulée n'est plus une non-conformité
  if (mode == Mode::Session && alarms[AL_EARLY].active) {
    bool stillEarly = false;
    for (int i = 0; i < sess.nCrates; i++)
      if (sess.crates[i].pos == -1 && sess.crates[i].early) stillEarly = true;
    if (!stillEarly) {
      alarms[AL_EARLY].active = false;
      logEvent("FIN_ALARME", -1, "Sortie avant la cible annulée : caisse remise en place 3");
      if (settings.ntfyAlarms)
        notify(String("Lot ") + meta.lot, "Décalage annulé : la caisse est remise en place 3", 3, "leftwards_arrow_with_hook");
    }
  }
  persist();
  return "";
}

String App::removeLast() {
  if (mode != Mode::Session) return "Aucune session en cours";
  if (!sess.removeLast()) return "Impossible : la dernière caisse est sortie ou porte le témoin";
  drainEvents();
  persist();
  return "";
}

String App::endEntries() {
  if (mode != Mode::Session) return "Aucune session en cours";
  sess.endEntries();
  drainEvents();
  persist();
  return "";
}

// ---------------------------------------------------------------------------
// Démarrage / arrêt
// ---------------------------------------------------------------------------
String App::startSession(const String& lot, const String& produit, const String& op, float t0,
                         float target) {
  if (mode != Mode::Idle) return "Une session ou une mesure est déjà en cours";
  if (!clockValid()) return "Heure non réglée : régler l'heure (Réglages) avant de démarrer";
  if (!sdOk()) return "Carte SD absente : impossible d'enregistrer le relevé";
  if (lot.length() == 0) return "Numéro de lot obligatoire";
  if (!(target > 0)) return "Cible d'UP invalide";
  if (!probe(P_TEMOIN).ok) return "Sonde témoin absente ou invalide";
  int nb = 0;
  for (int s = 0; s < up::NPOS; s++) nb += probe(P_BAIN1 + s).ok;
  if (nb == 0) return "Aucune sonde de bain valide";

  uint32_t now = nowEpoch();
  meta = RunMeta();
  String id = fmtStamp(now) + "_" + safeName(lot);
  strlcpy(meta.id, id.c_str(), sizeof meta.id);
  String dir = String("/sessions/") + id;
  strlcpy(meta.dir, dir.c_str(), sizeof meta.dir);
  strlcpy(meta.lot, lot.c_str(), sizeof meta.lot);
  strlcpy(meta.produit, produit.c_str(), sizeof meta.produit);
  strlcpy(meta.operateur, op.c_str(), sizeof meta.operateur);
  meta.t0 = t0;
  meta.target = target;
  meta.tStart = now;
  SD.mkdir(dir);
  appendLine(dir + "/data.csv",
             "horodatage,t_s,temoin,bain1,bain2,bain3,aux,caisse_p1,caisse_p2,caisse_p3,up_sortie,up_temoin");
  appendLine(dir + "/events.csv", "horodatage,type,caisse,message");

  up::Params p = settings.params();
  p.target = target;
  sess.begin(p, t0, now);
  for (int i = 0; i < AL_COUNT; i++) alarms[i] = Alarm();
  mode = Mode::Session;
  lastSampleMs = 0;
  ready = lastReady = false;
  writeMeta("en cours", "");
  logEvent("DEBUT", -1, String("Session démarrée – lot ") + lot + ", cible " + f1(target, 1) +
                            " UP, bouteilles à " + f1(t0, 1) + " °C");
  logEvent("INFO", -1, "Mettre le témoin dans la 1re caisse, entrer la caisse et appuyer sur DÉCALAGE");
  persist();
  return "";
}

String App::startMesure(const String& notes) {
  if (mode != Mode::Idle) return "Une session ou une mesure est déjà en cours";
  if (!clockValid()) return "Heure non réglée : régler l'heure (Réglages) avant de démarrer";
  if (!sdOk()) return "Carte SD absente : impossible d'enregistrer la mesure";
  if (!probe(P_TEMOIN).ok) return "Sonde témoin absente ou invalide";
  uint32_t now = nowEpoch();
  meta = RunMeta();
  String id = fmtStamp(now) + "_mesure";
  String dir = String("/mesures/") + id;
  if (SD.exists(dir)) { id += "-" + String(now % 100); dir = String("/mesures/") + id; }
  strlcpy(meta.id, id.c_str(), sizeof meta.id);
  strlcpy(meta.dir, dir.c_str(), sizeof meta.dir);
  strlcpy(meta.notes, notes.c_str(), sizeof meta.notes);
  strlcpy(meta.lot, "mesure", sizeof meta.lot);
  meta.tStart = now;
  meta.target = settings.target;
  SD.mkdir(dir);
  appendLine(dir + "/data.csv",
             "horodatage,t_s,temoin,bain1,bain2,bain3,aux,place_temoin,up_bain,up_total,up_modele,up_aux_bain,up_aux_total");
  appendLine(dir + "/events.csv", "horodatage,type,caisse,message");
  mes.begin(settings.params(), now);
  for (int i = 0; i < AL_COUNT; i++) alarms[i] = Alarm();
  mode = Mode::Mesure;
  lastSampleMs = 0;
  ready = lastReady = false;
  writeMeta("en cours", "");
  logEvent("DEBUT", -1, String("Mesure démarrée") + (notes.length() ? " – " + notes : String("")));
  logEvent("INFO", -1, "Appui 1 : témoin en place 1 ; 2 : place 2 ; 3 : place 3 ; 4 : sortie du bain");
  persist();
  return "";
}

String App::closeSession(const String& comment, bool force) {
  if (mode != Mode::Session) return "Aucune session en cours";
  if (!sess.finished() && !force) return "Des caisses sont encore dans le tunnel (forcer la clôture ?)";
  if (!sess.finished()) logEvent("CLOTURE_FORCEE", -1, "Clôture forcée : caisses encore au bain non conformes");
  bool conf = sess.lotConforme();
  logEvent("FIN", -1, String("Session clôturée – lot ") + (conf ? "CONFORME" : "NON CONFORME"));
  writeCratesCsv(meta, sess);
  writeSessionReport(meta, sess, comment);
  writeMeta(conf ? "CONFORME" : "NON CONFORME", comment);
  if (settings.ntfyEnd) {
    float mn = 1e9;
    for (int i = 0; i < sess.nCrates; i++)
      if (sess.crates[i].pos == -1) mn = fminf(mn, sess.crates[i].upFinal);
    notify(String("Lot ") + meta.lot + (conf ? " CONFORME" : " NON CONFORME"),
           String(sess.nCrates) + " caisses, UP min " + (mn < 1e8 ? f1(mn, 1) : String("-")),
           conf ? 3 : 5, conf ? "white_check_mark" : "x");
  }
  lastClosedDir = meta.dir;
  clearState();
  mode = Mode::Idle;
  ready = lastReady = false;
  for (int i = 0; i < AL_COUNT; i++) alarms[i] = Alarm();
  updateBuzzer();
  return "";
}

String App::stopMesure(const String& comment) {
  if (mode != Mode::Mesure) return "Aucune mesure en cours";
  logEvent("FIN", -1, String("Mesure arrêtée : ") + f1(mes.upBath, 2) + " UP bain, " +
                          f1(mes.upTotal, 2) + " UP bain + air");
  writeMesureReport(meta, mes, comment);
  writeMeta("terminée", comment);
  lastClosedDir = meta.dir;
  clearState();
  mode = Mode::Idle;
  ready = lastReady = false;
  for (int i = 0; i < AL_COUNT; i++) alarms[i] = Alarm();
  updateBuzzer();
  return "";
}

void App::writeMeta(const char* statut, const String& comment) {
  JsonDocument d;
  d["type"] = mode == Mode::Mesure ? "mesure" : "session";
  d["id"] = meta.id;
  d["lot"] = meta.lot;
  d["produit"] = meta.produit;
  d["operateur"] = meta.operateur;
  d["notes"] = meta.notes;
  d["debut"] = fmtDateTime(meta.tStart);
  d["fin"] = strcmp(statut, "en cours") ? fmtDateTime(nowEpoch()) : "";
  d["statut"] = statut;
  d["t0"] = meta.t0;
  d["cible"] = meta.target;
  d["k1"] = settings.k1;
  d["k2"] = settings.k2;
  d["modele"] = settings.modelSource;
  d["modeleDate"] = settings.modelDate;
  d["etalonnageSondes"] = settings.calDate;
  d["firmware"] = FW_VERSION;
  d["commentaire"] = comment;
  if (mode == Mode::Session) {
    d["caisses"] = sess.nCrates;
    float mn = 1e9, mx = -1;
    int nc = 0;
    for (int i = 0; i < sess.nCrates; i++) {
      const up::Crate& c = sess.crates[i];
      if (c.pos != -1) continue;
      mn = fminf(mn, c.upFinal); mx = fmaxf(mx, c.upFinal);
      if (!c.conforme) nc++;
    }
    if (mx >= 0) { d["upMin"] = mn; d["upMax"] = mx; }
    d["nonConformes"] = nc;
    if (sess.rKnown) d["rapportTemoin"] = sess.r;
  } else {
    d["upBain"] = mes.upBath;
    d["upTotal"] = mes.upTotal;
    d["upModele"] = mes.upPred;
  }
  String s;
  serializeJsonPretty(d, s);
  writeText(String(meta.dir) + "/meta.json", s);
}

// ---------------------------------------------------------------------------
// Reprise après coupure
// ---------------------------------------------------------------------------
void App::persist() {
  if (mode == Mode::Idle) return;
  StateHeader h;
  h.magic = STATE_MAGIC;
  h.metaSize = sizeof(RunMeta);
  h.engineSize = mode == Mode::Session ? sizeof(up::Session) : sizeof(up::Measure);
  h.savedAt = nowEpoch();
  h.mode = (uint8_t)mode;
  const uint8_t* parts[3] = {(const uint8_t*)&h, (const uint8_t*)&meta,
                             mode == Mode::Session ? (const uint8_t*)&sess : (const uint8_t*)&mes};
  size_t sizes[3] = {sizeof h, sizeof meta, h.engineSize};
  writeBinaryAtomic(STATE_PATH, parts, sizes, 3);
  lastPersist = h.savedAt;
}

void App::clearState() {
  if (sdOk()) SD.remove(STATE_PATH);
}

bool App::restore() {
  if (!sdOk() || !SD.exists(STATE_PATH)) return false;
  File f = SD.open(STATE_PATH, FILE_READ);
  if (!f) return false;
  StateHeader h;
  bool ok = f.read((uint8_t*)&h, sizeof h) == sizeof h && h.magic == STATE_MAGIC &&
            h.metaSize == sizeof(RunMeta) &&
            ((h.mode == (uint8_t)Mode::Session && h.engineSize == sizeof(up::Session)) ||
             (h.mode == (uint8_t)Mode::Mesure && h.engineSize == sizeof(up::Measure)));
  if (ok) ok = f.read((uint8_t*)&meta, sizeof meta) == sizeof meta;
  if (ok) {
    uint8_t* dst = h.mode == (uint8_t)Mode::Session ? (uint8_t*)&sess : (uint8_t*)&mes;
    ok = f.read(dst, h.engineSize) == h.engineSize;
  }
  f.close();
  if (!ok) {
    Serial.println("État de reprise illisible : ignoré");
    SD.rename(STATE_PATH, "/state.bad");
    return false;
  }
  mode = (Mode)h.mode;
  lastSampleMs = 0;   // la coupure n'est pas intégrée : aucune UP comptée (prudent)
  uint32_t now = nowEpoch();
  long gap = clockValid() ? (long)now - (long)h.savedAt : -1;
  logEvent("REPRISE", -1, String("Reprise après redémarrage") +
                              (gap >= 0 ? String(", interruption d'environ ") + gap + " s (UP non comptées)" : String("")));
  return true;
}

// ---------------------------------------------------------------------------
void App::loop() {
  updateBuzzer();
}

// ---------------------------------------------------------------------------
// État pour la page web
// ---------------------------------------------------------------------------
void App::statusJson(JsonObject o) {
  uint32_t now = nowEpoch();
  o["fw"] = FW_VERSION;
  o["mode"] = mode == Mode::Session ? "session" : (mode == Mode::Mesure ? "mesure" : "repos");
  o["heure"] = clockValid() ? fmtDateTime(now) : String("heure non réglée");
  o["epoch"] = now;
  o["clockOk"] = clockValid();
  o["sd"] = sdOk();
  o["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  o["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  o["ready"] = ready;
  o["eta"] = eta;
  o["silenced"] = silencedUntilMs != 0;
  o["lastClosed"] = lastClosedDir;
  JsonObject df = o["def"].to<JsonObject>();
  df["target"] = settings.target;
  df["t0"] = settings.t0;
  df["produit"] = settings.produit;
  JsonArray pr = o["probes"].to<JsonArray>();
  for (int i = 0; i < NPROBE; i++) {
    const ProbeState& p = probe(i);
    JsonObject x = pr.add<JsonObject>();
    x["name"] = PROBE_NAMES[i];
    if (p.ok) { x["v"] = p.value; x["raw"] = p.raw; }
    x["ok"] = p.ok;
    x["offset"] = settings.offset[i];
  }
  JsonArray al = o["alarms"].to<JsonArray>();
  for (int i = 0; i < AL_COUNT; i++) {
    if (!alarms[i].active) continue;
    JsonObject x = al.add<JsonObject>();
    x["msg"] = alarms[i].msg;
    x["sounding"] = alarms[i].sounding;
    x["latched"] = alarms[i].latched;
    x["since"] = fmtTime(alarms[i].since);
  }
  JsonArray lg = o["log"].to<JsonArray>();
  for (int k = 0; k < logCount; k++) {
    int idx = (logHead - 1 - k + LOG_RAM) % LOG_RAM;
    lg.add(logRam[idx]);
  }
  if (mode == Mode::Idle) return;
  JsonObject m = o["run"].to<JsonObject>();
  m["id"] = meta.id;
  m["dir"] = meta.dir;
  m["lot"] = meta.lot;
  m["produit"] = meta.produit;
  m["operateur"] = meta.operateur;
  m["notes"] = meta.notes;
  m["debut"] = fmtDateTime(meta.tStart);
  m["t0"] = meta.t0;
  m["target"] = meta.target;
  if (mode == Mode::Session) {
    m["started"] = sess.started;
    m["entriesEnded"] = sess.entriesEnded;
    m["finished"] = sess.finished();
    m["nCrates"] = sess.nCrates;
    m["rKnown"] = sess.rKnown;
    m["r"] = sess.r;
    m["rc"] = sess.rc();
    m["upMax"] = sess.p.upMax;
    m["canUndo"] = sess.u.valid && now - sess.u.t <= UNDO_MAX_S;
    m["sinceShift"] = sess.started ? now - sess.lastShift : 0;
    JsonArray sl = m["slots"].to<JsonArray>();
    for (int s = 0; s < up::NPOS; s++) {
      JsonObject x = sl.add<JsonObject>();
      float tw = up::effectiveBath(in, s);
      if (!std::isnan(tw)) x["tw"] = tw;
      int i = sess.slot[s];
      if (i < 0) continue;
      const up::Crate& c = sess.crates[i];
      x["id"] = c.id;
      x["up"] = sess.upNow(c);
      x["witness"] = c.witness && !c.witnessFault;
      x["tc"] = c.b.tc;
      x["since"] = now - c.tIn;
    }
    JsonArray ex = m["exited"].to<JsonArray>();
    int shown = 0;
    for (int i = sess.nCrates - 1; i >= 0 && shown < 8; i--) {
      const up::Crate& c = sess.crates[i];
      if (c.pos != -1) continue;
      JsonObject x = ex.add<JsonObject>();
      x["id"] = c.id;
      x["up"] = c.upFinal;
      x["measured"] = c.measured;
      x["conforme"] = c.conforme;
      x["sortie"] = fmtTime(c.tOut);
      shown++;
    }
    int nc = 0, nout = 0;
    for (int i = 0; i < sess.nCrates; i++)
      if (sess.crates[i].pos == -1) { nout++; if (!sess.crates[i].conforme) nc++; }
    m["nOut"] = nout;
    m["nNonConformes"] = nc;
  } else {
    m["pos"] = mes.pos;
    m["upBath"] = mes.upBath;
    m["upTotal"] = mes.upTotal;
    m["upPred"] = mes.upPred;
    m["upAuxBath"] = mes.upAuxBath;
    m["upAuxTotal"] = mes.upAuxTotal;
    if (!std::isnan(mes.t0)) m["tw0"] = mes.t0;
    m["canUndo"] = mes.lastPress && now - mes.lastPress <= UNDO_MAX_S;
    m["sincePress"] = mes.lastPress ? now - mes.lastPress : 0;
    JsonArray tp = m["tPos"].to<JsonArray>();
    for (int s = 0; s < up::NPOS; s++) tp.add(mes.tPos[s]);
  }
}
