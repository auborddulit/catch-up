// Tests sur PC du cœur de calcul.
//   g++ -std=c++17 -O2 -I../firmware/catch_up test_core.cpp ../firmware/catch_up/up_core.cpp -o test_core && ./test_core
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <random>
#include <vector>
#include "up_core.h"

using namespace up;

static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { fails++; printf("ÉCHEC %s:%d : ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define NEAR(a, b, tol) (fabs((a) - (b)) <= (tol))

// Tunnel « réel » simulé : eau 66 / 67 / 68 °C (entrée -> sortie)
static Inputs bathInputs(float b0, float b1, float b2) {
  Inputs in;
  in.bath[0] = b0; in.bath[1] = b1; in.bath[2] = b2;
  for (int i = 0; i < NPOS; i++) in.bathOk[i] = true;
  return in;
}

static void testPU() {
  Params p;
  // 1 min à 60 °C = 1 UP
  CHECK(NEAR(puPerSecond(60, p) * 60, 1.0, 1e-5), "1 min à 60 °C");
  // +6,94 °C = ×10
  CHECK(NEAR(puPerSecond(66.94f, p) * 60, 10.0, 1e-3), "z = 6,94");
  // sous le seuil : 0
  CHECK(puPerSecond(59.9f, p) == 0, "sous le seuil");
  // 1 °C de plus = ×1,393
  CHECK(NEAR(puPerSecond(61, p) / puPerSecond(60, p), 1.3935, 1e-3), "facteur 1,393");
}

static void testBottle() {
  Bottle b; b.init(6);
  for (int i = 0; i < 3600; i++) b.step(66, 1, 0.006f, 0.006f);
  CHECK(NEAR(b.tc, 66, 0.01), "convergence vers l'eau (%.3f)", b.tc);
  // pas de 1 s ou de 10 s : même résultat (mise à jour exacte)
  Bottle a, c; a.init(6); c.init(6);
  for (int i = 0; i < 600; i++) a.step(66, 1, 0.006f, 0.004f);
  for (int i = 0; i < 60; i++) c.step(66, 10, 0.006f, 0.004f);
  CHECK(NEAR(a.tc, c.tc, 0.05), "indépendance au pas (%.3f / %.3f)", a.tc, c.tc);
}

// Session nominale : le témoin réel suit exactement le modèle « vrai » (k vrais)
// et le firmware a des k différents ; on vérifie la cadence et la correction.
static void runSession(float kTrue1, float kTrue2, float kFw1, float kFw2, int nCrates,
                       bool verbose, int& outEarly, float& outR, float& minUp,
                       float& meanInterval) {
  Params p;
  p.target = 10;
  p.k1 = kFw1; p.k2 = kFw2;
  Session s;
  uint32_t t = 1000000;
  s.begin(p, 6, t);
  Bottle truth;            // vraie bouteille témoin
  bool witnessIn = false;
  int pressed = 0, exited = 0;
  outEarly = 0;
  minUp = 1e9;
  uint32_t lastPress = 0; double sumInt = 0; int nInt = 0;
  Inputs in = bathInputs(66, 67, 68);
  // 1er appui : la caisse 1 (témoin) entre
  s.shift(t); pressed++; lastPress = t;
  truth.init(6); witnessIn = true;
  int witnessPos = 0;
  for (int k = 0; k < 20000 && !s.finished(); k++) {
    t += 2;
    if (witnessIn) {
      truth.step(in.bath[witnessPos], 2, kTrue1, kTrue2);
      in.witness = truth.tc; in.witnessOk = true;
    }
    s.step(t, 2, in);
    if (s.ready(t)) {
      if (pressed >= nCrates && !s.entriesEnded) s.endEntries();
      int out = s.shift(t);
      if (verbose && pressed <= 2) printf("  décalage %d (remplissage) après %u s, cadence d'équilibre %.0f s\n", pressed, t - lastPress, s.eqS);
      sumInt += t - lastPress; nInt++; lastPress = t;
      pressed++;
      if (witnessIn) { witnessPos++; if (witnessPos >= NPOS) witnessIn = false; }
      if (out > 0) {
        exited++;
        const Crate& c = s.crates[out - 1];
        if (c.upFinal < minUp) minUp = c.upFinal;
        if (verbose) printf("  caisse %2d sortie : UP %s %.1f (modèle brut %.1f), bain min %.1f, %s\n",
                            c.id, c.measured ? "mesurées" : "estimées", c.upFinal, c.upModel,
                            c.minBath, c.conforme ? "CONFORME" : "NON CONFORME");
      }
    }
    Event e;
    while (s.ev.pop(e)) if (e.type == Ev::EarlyExit) outEarly++;
  }
  CHECK(s.finished(), "session terminée");
  CHECK(exited == nCrates, "toutes les caisses sorties (%d/%d)", exited, nCrates);
  outR = s.r;
  meanInterval = nInt ? (float)(sumInt / nInt) : 0;
}

static void testSession() {
  int early; float r, minUp, interv;
  printf("Session, modèle juste :\n");
  runSession(0.006f, 0.006f, 0.006f, 0.006f, 8, true, early, r, minUp, interv);
  CHECK(early == 0, "aucune sortie précoce");
  CHECK(NEAR(r, 1.0, 0.05), "r ≈ 1 (%.3f)", r);
  CHECK(minUp >= 10 - 0.05, "UP min >= cible (%.2f)", minUp);
  printf("  intervalle moyen %.0f s, r = %.3f\n", interv, r);

  printf("Session, modèle trop optimiste (bouteille réelle plus lente) :\n");
  runSession(0.0045f, 0.0045f, 0.006f, 0.006f, 8, true, early, r, minUp, interv);
  CHECK(r < 1.0, "r < 1 (%.3f)", r);
  CHECK(early == 0, "aucune sortie précoce");
  printf("  intervalle moyen %.0f s, r = %.3f\n", interv, r);

  printf("Session, modèle pessimiste (bouteille réelle plus rapide) :\n");
  runSession(0.008f, 0.008f, 0.006f, 0.006f, 8, true, early, r, minUp, interv);
  CHECK(r > 1.0, "r > 1 (%.3f) — et la correction n'est pas assouplie", r);
  printf("  intervalle moyen %.0f s, r = %.3f\n", interv, r);
}

static void testEarlyAndUndo() {
  Params p; p.target = 10;
  Session s; uint32_t t = 0;
  s.begin(p, 6, t);
  Inputs in = bathInputs(66, 67, 68);
  in.witness = 6; in.witnessOk = true;
  s.shift(t);  // caisse 1 entre
  for (int i = 0; i < 3; i++) { t += 60; s.step(t, 60, in); s.shift(t); }  // trop vite
  // la caisse 1 est sortie trop tôt
  CHECK(s.crates[0].pos == -1, "caisse 1 sortie");
  CHECK(s.crates[0].early && !s.crates[0].conforme, "caisse 1 non conforme");
  // annulation
  CHECK(s.undo(t + 10), "annulation possible");
  CHECK(s.crates[0].pos == 2 && s.slot[2] == 0, "caisse 1 revenue en place 3");
  CHECK(s.nCrates == 3, "caisse ajoutée retirée (%d)", s.nCrates);
  CHECK(!s.undo(t + 11), "pas de double annulation");
  // annulation trop tardive
  t += 10; s.shift(t);
  CHECK(!s.undo(t + 120), "annulation refusée après 60 s");
  // appui de trop en fin de lot : caisse vide retirée
  int n = s.nCrates;
  s.endEntries();
  CHECK(s.removeLast(), "retrait de la dernière caisse");
  CHECK(s.nCrates == n - 1 && s.slot[0] == -1, "caisse retirée du tunnel");
  Session w; w.begin(p, 6, 0); w.shift(0);
  CHECK(!w.removeLast(), "la caisse témoin ne peut pas être retirée");
}

static void testEquilibrium() {
  Params p; p.target = 10;
  Session s; s.begin(p, 6, 0);
  float tw[NPOS] = {66, 67, 68};
  float T = s.equilibriumInterval(tw);
  // vérification directe : une bouteille passant T par place reçoit la cible
  Bottle b; b.init(6); float upv = 0;
  for (int k = 0; k < NPOS; k++)
    for (float t = 0; t < T; t += 1) { float o = b.tc; b.step(tw[k], 1, p.k1, p.k2); upv += 0.5f * (puPerSecond(o, p) + puPerSecond(b.tc, p)); }
  printf("Cadence d'équilibre : %.0f s pour %.0f UP (contrôle : %.2f UP)\n", T, p.target, upv);
  CHECK(fabs(upv - 10) < 0.3, "cadence d'équilibre cohérente (%.2f UP)", upv);
  p.target = 20; s.begin(p, 6, 0);
  CHECK(s.equilibriumInterval(tw) > T, "cible plus haute -> cadence plus lente");
  float cold[NPOS] = {55, 55, 55};
  CHECK(s.equilibriumInterval(cold) < 0, "cible inatteignable -> -1");
}

static void testFaults() {
  Params p; p.target = 5;
  Session s; uint32_t t = 0;
  s.begin(p, 6, t);
  Inputs in = bathInputs(66, 67, 68);
  in.bathOk[2] = false;  // sonde place 3 HS
  CHECK(NEAR(effectiveBath(in, 2), 66, 1e-6), "repli sur la sonde la plus froide");
  in.bathOk[0] = in.bathOk[1] = false;
  CHECK(std::isnan(effectiveBath(in, 2)), "aucune sonde : NAN");
  // défaut témoin
  in = bathInputs(66, 67, 68);
  in.witnessOk = false;
  s.shift(t);
  for (int i = 0; i < 20; i++) { t += 2; s.step(t, 2, in); }
  CHECK(s.crates[0].witnessFault, "défaut témoin après 30 s");
  CHECK(!s.lotConforme(), "lot non conforme si témoin défaillant");
}

static void testFit() {
  // courbe synthétique avec bruit de mesure, puis ajustement
  Params p;
  const float k1 = 0.0072f, k2 = 0.0041f, dt = 2;
  std::vector<float> tw, tm;
  Bottle b; b.init(5.5f);
  std::mt19937 rng(1);
  std::normal_distribution<float> noise(0, 0.06f);
  for (int i = 0; i < 600; i++) {
    float w = i < 200 ? 66 : (i < 400 ? 67 : 68);
    w += 0.3f * sinf(i / 30.0f);  // fluctuation du bain
    tw.push_back(w + noise(rng));
    tm.push_back(b.tc + noise(rng));
    b.step(w, dt, k1, k2);
  }
  FitResult r = fitModel(tw.data(), tm.data(), (int)tm.size(), dt, p);
  printf("Ajustement : k1=%.5f k2=%.5f (vrais %.5f %.5f), RMSE %.3f °C, UP mesurées %.2f / prévues %.2f\n",
         r.k1, r.k2, k1, k2, r.rmse, r.upMeas, r.upPred);
  CHECK(r.ok, "ajustement OK");
  CHECK(r.rmse < 0.2f, "RMSE faible");
  CHECK(fabs(r.upPred - r.upMeas) / r.upMeas < 0.08f, "UP prévues proches des mesurées");
}

static void testMeasure() {
  Params p;
  Measure m; uint32_t t = 0;
  m.begin(p, t);
  Inputs in = bathInputs(66, 67, 68);
  Bottle b; b.init(6);
  m.press(t);  // entre en place 1
  for (int pos = 0; pos < 3; pos++) {
    for (int i = 0; i < 200; i++) {
      t += 2; b.step(in.bath[pos], 2, p.k1, p.k2);
      in.witness = b.tc; in.witnessOk = true;
      m.step(t, 2, in);
    }
    m.press(t);
  }
  CHECK(m.pos == -1, "témoin sorti");
  float bath = m.upBath;
  // refroidissement à l'air (approx.)
  for (int i = 0; i < 390; i++) { t += 2; b.step(25, 2, 0.0015f, 0.003f); in.witness = b.tc; m.step(t, 2, in); }
  printf("Mesure : UP bain %.2f, UP bain+air %.2f, prévision modèle %.2f\n", m.upBath, m.upTotal, m.upPred);
  CHECK(NEAR(m.upBath, bath, 1e-6), "UP bain figées après la sortie");
  CHECK(m.upTotal > m.upBath, "UP air ajoutées au total");
  CHECK(fabs(m.upPred - m.upBath) / m.upBath < 0.05f, "prévision cohérente");
}

int main() {
  testPU();
  testBottle();
  testSession();
  testEarlyAndUndo();
  testEquilibrium();
  testFaults();
  testFit();
  testMeasure();
  if (fails) { printf("\n%d test(s) en échec\n", fails); return 1; }
  printf("\nTous les tests passent.\n");
  return 0;
}
