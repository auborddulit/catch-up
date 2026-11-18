// Simulation complète du firmware sur PC : mesure d'essai, étalonnage du modèle,
// puis session d'un lot de 10 caisses avec fluctuations du bain.
// Produit des relevés réels dans tests/out/sd/.
#include <cstdio>
#include <vector>
#include "Arduino.h"
#include "SD.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "storage.h"
#include "app.h"
#include "report.h"

extern uint32_t g_ms, g_now;
extern ProbeState g_probe[NPROBE];
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("ÉCHEC l.%d : ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

// « Physique » : bain qui oscille (hystérésis de la cuve) + creux ponctuel
static float bathT(int place, uint32_t t, bool dip) {
  float base[3] = {66.0f, 67.0f, 68.0f};
  float v = base[place] + 0.8f * sinf(t / 450.0f + place);
  if (dip && place == 0) v -= 3.5f;
  return v;
}
// vraie bouteille (plus lente que le modèle par défaut)
static const float K1 = 0.0055f, K2 = 0.0046f;

static void setProbes(float w, bool wOk, uint32_t t, bool dip) {
  for (int s = 0; s < 3; s++) { g_probe[P_BAIN1 + s].value = g_probe[P_BAIN1 + s].raw = bathT(s, t, dip); g_probe[P_BAIN1 + s].ok = true; }
  g_probe[P_TEMOIN].value = g_probe[P_TEMOIN].raw = w; g_probe[P_TEMOIN].ok = wOk;
  g_probe[P_AUX].ok = false;
}

static void tick() { g_now += 2; g_ms += 2000; app.onSample(); }

int main() {
  if (system("rm -rf out && mkdir -p out") || chdir("out")) return 2;
  sdBegin();
  clockBegin();
  settings.target = 15;
  printf("=== Mesure d'essai (cadence habituelle 6 min 40) ===\n");
  setProbes(6, true, g_now, false);
  CHECK(app.startMesure("cadence 6:40, bain 66/68").isEmpty(), "démarrage mesure");
  up::Bottle b; b.init(6);
  int pos = -1;
  for (int k = 0; k < 30; k++) { setProbes(b.tc, true, g_now, false); tick(); }      // avant entrée
  app.shiftPressed("bouton"); pos = 0;
  for (int stage = 0; stage < 3; stage++) {
    for (int k = 0; k < 200; k++) { b.step(bathT(pos, g_now, false), 2, K1, K2); setProbes(b.tc, true, g_now, false); tick(); }
    app.shiftPressed("bouton"); pos++;
  }
  for (int k = 0; k < 390; k++) { b.step(22, 2, 0.0012f, 0.003f); setProbes(b.tc, true, g_now, false); tick(); }  // 13 min à l'air
  float upBath = app.mes.upBath;
  printf("UP bain %.2f, bain+air %.2f, modèle par défaut %.2f\n", app.mes.upBath, app.mes.upTotal, app.mes.upPred);
  CHECK(app.stopMesure("essai de référence").isEmpty(), "arrêt mesure");
  String mdir = app.lastClosedDir;

  printf("=== Étalonnage du modèle ===\n");
  JsonDocument fd;
  String err = fitFromDir(mdir, fd.to<JsonObject>());
  printf("k1 %.5f k2 %.5f RMSE %.3f, UP mesurées %.2f / prévues %.2f (ancien modèle %.2f) %s\n",
         fd["k1"].as<float>(), fd["k2"].as<float>(), fd["rmse"].as<float>(), fd["upMeas"].as<float>(),
         fd["upPred"].as<float>(), fd["upCurrent"].as<float>(), err.c_str());
  CHECK(err.isEmpty(), "ajustement");
  // on applique volontairement un modèle un peu optimiste pour tester la correction
  settings.k1 = fd["k1"].as<float>() * 1.08f; settings.k2 = fd["k2"].as<float>() * 1.08f;
  strlcpy(settings.modelSource, "étalonné sur la mesure (×1,08 pour le test)", sizeof settings.modelSource);
  strlcpy(settings.modelDate, "2026-11-18", sizeof settings.modelDate);
  strlcpy(settings.calDate, "2026-11-18 09:12:00", sizeof settings.calDate);

  printf("=== Session : lot L2611-A, cible %.1f UP (référence %.2f + 10 %%) ===\n", upBath * 1.1f, upBath);
  g_now += 1800; g_ms += 1800000;
  setProbes(6, true, g_now, false);
  float target = roundf(upBath * 1.1f * 10) / 10;
  CHECK(app.startSession("L2611-A", "Infusion verveine pétillante", "Camille", 6.0f, target).isEmpty(), "démarrage session");
  const int NCR = 10;
  std::vector<up::Bottle> truth;   // vraie bouteille de chaque caisse (pour vérifier)
  std::vector<int> place;          // place réelle de chaque caisse
  b.init(6);
  int presses = 0, waitAfterReady = 0;
  bool undoneOnce = false;
  for (int k = 0; k < 20000 && !app.sess.finished(); k++) {
    bool dip = g_now % 3600 > 1500 && g_now % 3600 < 1740 ;   // creux de 4 min sur la place 1
    // témoin
    bool wIn = app.sess.nCrates > 0 && app.sess.crates[0].pos >= 0;
    if (wIn) b.step(bathT(app.sess.crates[0].pos, g_now, dip), 2, K1, K2);
    setProbes(b.tc, true, g_now, dip);
    tick();
    if (presses == 0 && k == 5) { app.shiftPressed("bouton"); presses++; continue; }
    if (app.ready) {
      if (++waitAfterReady >= 5) {   // l'opérateur réagit en 10 s
        if (app.sess.nCrates >= NCR && !app.sess.entriesEnded) app.endEntries();
        app.shiftPressed("bouton"); presses++; waitAfterReady = 0;
        if (presses == 6 && !undoneOnce) {   // appui en double par erreur, puis annulation
          app.shiftPressed("bouton");
          CHECK(app.undoShift().isEmpty(), "annulation");
          undoneOnce = true;
        }
      }
    } else waitAfterReady = 0;
  }
  CHECK(app.sess.finished(), "toutes les caisses sorties");
  printf("Rapport témoin / modèle : %.3f\n", app.sess.r);
  for (int i = 0; i < app.sess.nCrates; i++) {
    const up::Crate& c = app.sess.crates[i];
    printf("  C%-2d %s %.2f UP (modèle brut %.2f) place 1/2/3 : %.0f/%.0f/%.0f s, eau min %.1f  %s\n", c.id,
           c.measured ? "mesurées" : "estimées", c.upFinal, c.upModel, c.tPos[0], c.tPos[1], c.tPos[2], c.minBath,
           c.conforme ? "CONFORME" : "NON CONFORME");
  }
  CHECK(app.sess.nCrates == NCR, "%d caisses", app.sess.nCrates);
  CHECK(app.sess.lotConforme(), "lot conforme");
  String sdir = app.meta.dir;
  CHECK(app.closeSession("RAS.", false).isEmpty(), "clôture");
  CHECK(SD.exists(sdir + "/releve.html") && SD.exists(sdir + "/caisses.csv") && SD.exists(sdir + "/meta.json"), "fichiers du relevé");
  CHECK(!SD.exists("/state.bin"), "état de reprise supprimé");
  printf("Relevés : out/sd%s/ et out/sd%s/\n", mdir.c_str(), sdir.c_str());
  printf("%s\n", fails ? "ÉCHECS" : "Simulation OK");
  return fails ? 1 : 0;
}
