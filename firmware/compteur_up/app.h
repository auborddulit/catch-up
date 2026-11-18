// app.h — Logique de l'appareil : modes, actions, alarmes, journal, reprise.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "up_core.h"

enum class Mode : uint8_t { Idle = 0, Session = 1, Mesure = 2 };

struct RunMeta {
  char id[48] = "";
  char dir[64] = "";
  char lot[32] = "";
  char produit[48] = "";
  char operateur[32] = "";
  char notes[96] = "";
  float t0 = 6;
  float target = 15;
  uint32_t tStart = 0;
};

enum AlarmId {
  AL_BATH1, AL_BATH2, AL_BATH3,
  AL_PROBE_T, AL_PROBE_B1, AL_PROBE_B2, AL_PROBE_B3,
  AL_EARLY, AL_RATIO, AL_WITNESS, AL_BATHLOST, AL_OVERMAX, AL_SD,
  AL_COUNT
};

struct Alarm {
  bool active = false;
  bool latched = false;    // reste affichée jusqu'à acquittement
  bool sounding = true;    // fait sonner le buzzer
  uint32_t since = 0;
  String msg;
};

constexpr int LOG_RAM = 12;

class App {
 public:
  void begin();
  void loop();
  void onSample();
  void onButton(bool longPress);

  // Actions (bouton ou page web). Renvoient "" si OK, sinon un message d'erreur.
  String startSession(const String& lot, const String& produit, const String& op,
                      float t0, float target);
  String startMesure(const String& notes);
  String shiftPressed(const char* source);
  String undoShift();
  String endEntries();
  String removeLast();
  String closeSession(const String& comment, bool force);
  String stopMesure(const String& comment);
  void acknowledge();
  // Ajoute un commentaire / une action corrective à un lot déjà clôturé :
  // horodaté dans events.csv et meta.json, et inséré dans le relevé.
  String addComment(const String& dir, const String& text);

  void statusJson(JsonObject o);
  bool anySounding() const;
  const Alarm* topAlarm() const;

  Mode mode = Mode::Idle;
  RunMeta meta;
  up::Session sess;
  up::Measure mes;
  up::Inputs in;
  Alarm alarms[AL_COUNT];
  bool ready = false;
  float eta = -1;
  uint32_t silencedUntilMs = 0;
  String lastClosedDir;
  String logRam[LOG_RAM];
  int logHead = 0, logCount = 0;

 private:
  void drainEvents();
  void raise(AlarmId id, const String& msg, bool sounding = true);
  void setCond(AlarmId id, bool active, const String& msg, bool sounding = true);
  void updateAlarms();
  void logEvent(const char* type, int crate, const String& msg);
  void writeDataLine();
  void persist();
  bool restore();
  void clearState();
  void writeMeta(const char* statut, const String& comment);
  void updateBuzzer();

  uint32_t lastSampleMs = 0;
  uint32_t lastPersist = 0;
  uint32_t belowSince[up::NPOS] = {0, 0, 0};
  bool lastReady = false;
};

extern App app;
