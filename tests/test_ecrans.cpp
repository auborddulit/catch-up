// Captures des écrans de l'appareil, dessinés par le vrai code (display.cpp)
// avec la bibliothèque U8g2 compilée sur PC. Écrit out/ecrans/*.pbm et vérifie
// les parcours au clavier (nouveau lot, menus, confirmations).
#include <cstdio>
#include <string>
#include <vector>
#include "Arduino.h"
#include "SD.h"
#include "u8g2.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "storage.h"
#include "app.h"
#include "display.h"

extern uint32_t g_ms, g_now;
extern ProbeState g_probe[NPROBE];
extern u8g2_t* g;
void displayDraw();
String ipText() { return "192.168.1.50"; }

static u8g2_t u8;
static int fails = 0;
static std::vector<std::string> shots;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("ÉCHEC l.%d : ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint8_t dummyGpio(u8x8_t*, uint8_t, uint8_t, void*) { return 1; }
// tampon complet SSD1309 : octet (y/8)*128 + x, bit y%8
static int pixel(int x, int y) { return (u8g2_GetBufferPtr(&u8)[(y / 8) * 128 + x] >> (y % 8)) & 1; }

static void shot(const char* name, bool draw = true) {
  if (draw) displayDraw();
  std::string p = std::string("ecrans/") + name + ".pbm";
  FILE* f = fopen(p.c_str(), "w");
  fprintf(f, "P1\n128 64\n");
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 128; x++) fputc(pixel(x, y) ? '1' : '0', f);
    fputc('\n', f);
  }
  fclose(f);
  shots.push_back(name);
}

static void setProbes(float w, float b1, float b2, float b3) {
  float v[4] = {w, b1, b2, b3};
  for (int i = 0; i < 4; i++) { g_probe[i].value = g_probe[i].raw = v[i]; g_probe[i].ok = true; }
}
static void tick(int n = 1) { for (int i = 0; i < n; i++) { g_now += 2; g_ms += 2000; app.onSample(); } }
static void key(Key k) { uiOnKey(k); g_ms += 300; }
// phase de clignotement : écran inversé (alertes) et ligne « Témoin débranchable »
static void blinkOn() { g_ms = (g_ms / 9000 + 1) * 9000; }

int main() {
  if (system("mkdir -p out/ecrans") || chdir("out")) return 2;
  system("rm -rf sd");
  sdBegin();
  clockBegin();
  u8g2_Setup_ssd1309_128x64_noname0_f(&u8, U8G2_R0, u8x8_byte_empty, dummyGpio);
  g = &u8;
  settings.target = 33;
  settings.lotNext = 128;
  strlcpy(settings.operateur, "Camille", sizeof settings.operateur);
  displaySplash();
  shot("00-demarrage", false);
  CHECK(pixel(14 + 71, 2 + 30) == 1 && pixel(14 + 74, 2 + 30) == 0, "logo dessiné en place (bouteille, creux du U)");
  setProbes(5.9f, 66.2f, 67.1f, 68.0f);
  tick();
  shot("01-repos");

  key(Key::Ok);
  shot("02-menu-repos");
  key(Key::Ok);                         // Démarrer un lot
  shot("03-nouveau-lot-numero");
  key(Key::Ok);
  key(Key::Up);                         // 6.0 -> 6.5 °C
  key(Key::Down);
  shot("04-nouveau-lot-temperature");
  key(Key::Ok);
  shot("05-nouveau-lot-cible");
  key(Key::Ok);
  shot("06-nouveau-lot-verifier");
  key(Key::Ok);                         // démarre
  CHECK(app.mode == Mode::Session, "session démarrée depuis l'appareil");
  CHECK(String(app.meta.lot) == String("L0128"), "n° de lot auto (%s)", app.meta.lot);
  CHECK(settings.lotNext == 129, "compteur de lots incrémenté (%d)", (int)settings.lotNext);
  tick();
  shot("07-lot-demarre");

  // remplissage : témoin réel = modèle (k par défaut)
  up::Bottle w; w.init(6);
  app.shiftPressed("bouton");
  for (int i = 0; i < 60; i++) { w.step(66.2f, 2, 0.006f, 0.006f); setProbes(w.tc, 66.2f, 67.1f, 68.0f); tick(); }
  shot("08-remplissage");
  // avance jusqu'à ce que la caisse 1 soit en place 3 et presque prête
  int guard = 0;
  while (app.sess.slot[2] < 0 && guard++ < 2000) {
    int pos = app.sess.crates[0].pos;
    float bt[3] = {66.2f, 67.1f, 68.0f};
    w.step(bt[pos], 2, 0.006f, 0.006f); setProbes(w.tc, 66.2f, 67.1f, 68.0f); tick();
    if (app.ready) app.shiftPressed("bouton");
  }
  while (!app.ready && guard++ < 4000) {
    w.step(68.0f, 2, 0.006f, 0.006f); setProbes(w.tc, 66.2f, 67.1f, 68.0f); tick();
    if (app.sess.upNow(app.sess.crates[0]) > 0.8f * 33) break;
  }
  shot("09-en-cours");
  while (!app.ready && guard++ < 6000) { w.step(68.0f, 2, 0.006f, 0.006f); setProbes(w.tc, 66.2f, 67.1f, 68.0f); tick(); }
  blinkOn();
  shot("10-decaler");
  app.shiftPressed("bouton");          // la caisse témoin sort
  tick();
  blinkOn();
  shot("11-temoin-debranchable");

  // alarme : bain place 1 trop froid
  for (int i = 0; i < 40; i++) { setProbes(w.tc, 62.0f, 67.1f, 68.0f); tick(); }
  blinkOn();
  shot("12-alarme");
  key(Key::Ok);                         // acquitter
  CHECK(app.silencedUntilMs != 0, "alarme acquittée par OK");
  setProbes(w.tc, 66.2f, 67.1f, 68.0f);
  tick(3);

  key(Key::Ok);
  shot("13-menu-lot");                  // 1er élément : « Fin des entrées »
  key(Key::Ok);
  shot("14-confirmer");
  key(Key::OkLong);                     // non : retour au menu
  CHECK(!app.sess.entriesEnded, "refus de confirmation sans effet");
  key(Key::OkLong);                     // retour à l'écran principal

  // fin de lot propre : fin des entrées puis décalages
  app.endEntries();
  guard = 0;
  while (!app.sess.finished() && guard++ < 20000) { setProbes(w.tc, 66.2f, 67.1f, 68.0f); tick(); if (app.ready) app.shiftPressed("bouton"); }
  CHECK(app.sess.finished(), "toutes les caisses sorties");
  key(Key::OkLong);
  shot("15-lot-termine");
  key(Key::Ok);                         // menu
  // le menu d'un lot terminé : « Terminer le lot » est le 1er élément
  key(Key::Ok);
  shot("16-confirmer-terminer");
  String dir = app.meta.dir;
  key(Key::Ok);
  CHECK(app.mode == Mode::Idle, "lot terminé depuis l'appareil");
  shot("17-message-fin");

  // commentaire ajouté après la clôture (page web)
  String err = app.addComment(dir, "Caisse 3 contrôlée visuellement, RAS.");
  CHECK(err.isEmpty(), "commentaire ajouté (%s)", err.c_str());
  String rep = readText(dir + "/releve.html", 400000);
  CHECK(rep.indexOf("Caisse 3 contrôlée visuellement") > 0, "commentaire dans le relevé");
  CHECK(rep.indexOf("<!--AJOUTS-->") > rep.indexOf("Caisse 3 contrôlée"), "marqueur conservé après l'ajout");
  CHECK(readText(dir + "/events.csv", 400000).indexOf("COMMENTAIRE") > 0, "commentaire tracé dans events.csv");
  CHECK(readText(dir + "/meta.json").indexOf("ajouts") > 0, "commentaire dans meta.json");
  CHECK(!app.addComment("/sessions/../x", "a").isEmpty(), "chemin refusé");

  FILE* l = fopen("ecrans/liste.txt", "w");
  for (auto& s : shots) fprintf(l, "%s\n", s.c_str());
  fclose(l);
  printf("%d écrans capturés dans out/ecrans/\n", (int)shots.size());
  if (fails) { printf("%d ÉCHEC(S)\n", fails); return 1; }
  printf("Écrans OK\n");
  return 0;
}
