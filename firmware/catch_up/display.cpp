// display.cpp — Écran 128×64 et interface sur l'appareil.
//
// Écrans :
//   principal  : repos / session (UP de la caisse en sortie en gros) / mesure
//   alertes    : écran inversé clignotant « DÉCALER » ou « ALARME »
//   menu       : OK ouvre le menu ; ▲ ▼ choisissent ; OK valide ; OK long = retour
//   nouveau lot: n° de lot (auto) → T° des bouteilles → cible → confirmation
//   confirmation en 2 temps pour les corrections (annuler, retirer, terminer…)
//
// Le dessin passe par l'API C de U8g2 : le même code tourne sur PC pour
// produire des captures d'écran (tests/test_ecrans.cpp).
#include "display.h"
#include <Arduino.h>
#include "app.h"
#include "settings.h"
#include "sensors.h"
#include "clock.h"
#include "config.h"
#include "logo.h"

#ifdef ARDUINO
#include <U8g2lib.h>
#include <WiFi.h>
#ifdef DISPLAY_SSD1309_SPI
// Si l'image est décalée de 2 colonnes, essayer la variante NONAME2.
static U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI oled(U8G2_R0, PIN_OLED_CS, PIN_OLED_DC, PIN_OLED_RST);
#else
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
#endif
static u8g2_t* g = nullptr;
static String ipText() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                       : String("AP ") + WiFi.softAPIP().toString();
}
#else
#include "u8g2.h"
u8g2_t* g = nullptr;            // fourni par le programme de test
String ipText();
#endif

// ---------------------------------------------------------------------------
// État de l'interface
// ---------------------------------------------------------------------------
enum class Scr : uint8_t { Main, Menu, WizLot, WizT0, WizTarget, WizConfirm, Confirm, Info, Message };
enum class Act : uint8_t { None, StartLot, Undo, Remove, EndEntries, Close, Ack, Info, Back };

static Scr scr = Scr::Main;
static Act menuActs[8];
static const char* menuLabels[8];
static int menuN = 0, menuSel = 0, menuTop = 0;
static Act pending = Act::None;
static int confirmStep = 0;              // 0 ou 1 (double confirmation)
static int32_t wizLot = 1;
static float wizT0 = 6, wizTarget = 15;
static String msg1, msg2;
static uint32_t msgUntil = 0, lastKey = 0, last = 0;

// ---------------------------------------------------------------------------
// Outils de dessin
// ---------------------------------------------------------------------------
#define F_SMALL u8g2_font_6x10_tf
#define F_MED   u8g2_font_helvB10_tf
#define F_BIG   u8g2_font_helvB18_tf
#define F_NUM   u8g2_font_logisoso24_tn

static void font(const uint8_t* f) { u8g2_SetFont(g, f); }
static int width(const char* s) { return u8g2_GetUTF8Width(g, s); }
static void text(int x, int y, const String& s) { u8g2_DrawUTF8(g, x, y, s.c_str()); }
static void textC(int y, const String& s) { text((128 - width(s.c_str())) / 2, y, s); }
static void textR(int y, const String& s) { text(128 - width(s.c_str()), y, s); }

static String t1(float v) { return std::isnan(v) ? String("--.-") : String(v, 1); }
static String probeTxt(int i) { const ProbeState& p = probe(i); return p.ok ? t1(p.value) : String("--.-"); }
static String mmss(float s) {
  if (s < 0) return String("--:--");
  char b[24];
  snprintf(b, sizeof b, "%d:%02d", (int)s / 60, (int)s % 60);
  return String(b);
}
static String hm() { return clockValid() ? fmtTime(nowEpoch()).substring(0, 5) : String("--:--"); }

// coupe un texte en lignes de n caractères au plus (sur les espaces)
static int wrap(const String& s, String* out, int maxLines, int n) {
  int k = 0;
  String rest = s;
  while (rest.length() && k < maxLines) {
    if ((int)rest.length() <= n) { out[k++] = rest; break; }
    int cut = n;
    while (cut > 0 && rest[cut] != ' ') cut--;
    if (cut == 0) cut = n;
    out[k++] = rest.substring(0, cut);
    rest = rest.substring(cut);
    rest.trim();
  }
  return k;
}

static void arrowsHint(int y) {          // « ▲▼ modifier   OK suite »
  u8g2_DrawTriangle(g, 1, y, 5, y - 7, 9, y);
  u8g2_DrawTriangle(g, 12, y - 7, 16, y, 20, y - 7);
  font(F_SMALL);
  text(24, y, "modifier");
  textR(y, "OK suite");
}

static void bar(int y, float ratio) {
  if (ratio < 0) ratio = 0;
  if (ratio > 1) ratio = 1;
  u8g2_DrawFrame(g, 0, y, 128, 7);
  u8g2_DrawBox(g, 1, y + 1, (int)(126 * ratio), 5);
}

// texte d'alarme abrégé pour l'écran : coupé avant « < », « – » ou « ( »
static String shortMsg(const String& m) {
  int cut = m.length();
  for (const char* sep : {" < ", " – ", " ("}) {
    int k = m.indexOf(sep);
    if (k > 0 && k < cut) cut = k;
  }
  return m.substring(0, cut);
}

// écran plein inversé et clignotant (alertes)
static void alertScreen(const char* title, const String* lines, int n) {
  bool inv = (millis() / 450) % 2 == 0;
  if (inv) { u8g2_DrawBox(g, 0, 0, 128, 64); u8g2_SetDrawColor(g, 0); }
  font(F_BIG);
  textC(22, title);
  font(F_SMALL);
  for (int i = 0; i < n; i++) textC(38 + 11 * i, lines[i]);
  u8g2_SetDrawColor(g, 1);
}

// ---------------------------------------------------------------------------
// Écran principal
// ---------------------------------------------------------------------------
static bool witnessReleasable() {
  const up::Session& s = app.sess;
  if (s.nCrates == 0 || !s.rKnown) return false;
  const up::Crate& c = s.crates[0];
  if (c.pos != -1 || !c.tOut) return false;
  // affiché tant que la sonde est branchée, 15 min au plus
  return probe(P_TEMOIN).ok && nowEpoch() - c.tOut < 900;
}

static void drawMainSession() {
  up::Session& s = app.sess;
  const Alarm* al = app.topAlarm();
  bool silenced = app.silencedUntilMs != 0;
  if (al && al->sounding && !silenced) {
    String l[3];
    int n = wrap(shortMsg(al->msg), l, 2, 21);
    l[n++] = "OK : acquitter";
    alertScreen("ALARME", l, n);
    return;
  }
  int i3 = s.slot[up::NPOS - 1];
  if (app.ready && !s.finished()) {
    String l[2];
    int n = 0;
    if (i3 >= 0) {
      const up::Crate& c = s.crates[i3];
      l[n++] = String("C") + c.id + "  " + String(s.upNow(c), 1) + " / " + String(s.p.target, 1) + " UP";
    }
    l[n++] = "puis appuyer";
    alertScreen("DÉCALER", l, n);
    return;
  }
  // bandeau du haut : lot, cadence, caisses
  font(F_SMALL);
  text(0, 8, String(app.meta.lot).substring(0, 8));
  String right = String("cad ") + mmss(s.eqS) + " C" + s.nCrates;
  if (al) right = String("! ") + right;
  textR(8, right);
  u8g2_DrawHLine(g, 0, 11, 128);

  if (!s.started) {
    font(F_MED);
    textC(30, "Témoin : caisse 1");
    textC(46, "puis DÉCALAGE");
    font(F_SMALL);
    textC(62, String("Cible ") + String(s.p.target, 1) + " UP");
    return;
  }
  if (s.finished()) {
    font(F_MED);
    textC(32, "Lot terminé");
    font(F_SMALL);
    textC(48, "OK > Terminer le lot");
    return;
  }
  if (i3 >= 0) {
    const up::Crate& c = s.crates[i3];
    float u = s.upNow(c);
    font(F_NUM);
    String big = String(u, 1) + "/" + String(s.p.target, (s.p.target == floorf(s.p.target)) ? 0 : 1);
    text(0, 39, big);
    font(F_SMALL);
    textR(23, String("C") + c.id);
    textR(39, "UP");
    bar(44, s.p.target > 0 ? u / s.p.target : 0);
  } else {                                 // remplissage du tunnel
    font(F_SMALL);
    text(0, 24, "Remplissage");
    text(0, 36, "décaler dans");
    font(F_NUM);
    textR(39, mmss(app.eta));
    float fi = s.fillInterval();
    bar(44, fi > 0 ? (fi - app.eta) / fi : 0);
  }
  font(F_SMALL);
  if (witnessReleasable() && (millis() / 3000) % 2 == 0) {
    textC(63, "Témoin débranchable");
  } else {
    text(0, 63, String("Eau ") + probeTxt(P_BAIN3));
    if (i3 >= 0) textR(63, String("~") + mmss(app.eta));
  }
}

static void drawMainIdle() {
  font(F_SMALL);
  text(0, 8, "REPOS");
  textR(8, hm());
  u8g2_DrawHLine(g, 0, 11, 128);
  text(0, 24, String("Eau ") + probeTxt(P_BAIN3) + " " + probeTxt(P_BAIN2) + " " + probeTxt(P_BAIN1));
  text(0, 36, String("Témoin ") + probeTxt(P_TEMOIN) + " °C");
  text(0, 48, ipText());
  textC(63, clockValid() ? String("OK : menu") : String("Heure à régler"));
}

static void drawMainMesure() {
  const up::Measure& m = app.mes;
  const Alarm* al = app.topAlarm();
  if (al && al->sounding && app.silencedUntilMs == 0) {
    String l[3];
    int n = wrap(shortMsg(al->msg), l, 2, 21);
    l[n++] = "OK : acquitter";
    alertScreen("ALARME", l, n);
    return;
  }
  if (app.ready) { String l[1] = {"décalage nominal"}; alertScreen("DÉCALER", l, 1); return; }
  font(F_SMALL);
  String pos = m.pos >= 0 ? String("place ") + (m.pos + 1) : String(m.pos == -2 ? "avant entrée" : "sorti");
  text(0, 8, String("MESURE ") + pos);
  textR(8, hm());
  u8g2_DrawHLine(g, 0, 11, 128);
  font(F_NUM);
  text(0, 39, String(m.upBath, 1));
  font(F_SMALL);
  textR(23, String("Tém. ") + probeTxt(P_TEMOIN));
  textR(39, "UP bain");
  text(0, 52, String("Total ") + String(m.upTotal, 1) + "  mod. " + String(m.upPred, 1));
  textC(63, m.pos >= 0 ? String("Nominal ") + mmss(app.eta) : String("Arrêt : page web"));
}

// ---------------------------------------------------------------------------
// Menus et assistants
// ---------------------------------------------------------------------------
static void addItem(Act a, const char* label) {
  if (menuN < 8) { menuActs[menuN] = a; menuLabels[menuN] = label; menuN++; }
}

static void openMenu() {
  menuN = 0;
  menuSel = menuTop = 0;
  uint32_t now = nowEpoch();
  if (app.mode == Mode::Idle) {
    addItem(Act::StartLot, "Démarrer un lot");
  } else if (app.mode == Mode::Session) {
    const up::Session& s = app.sess;
    // l'action la plus probable en premier ; « Annuler » jamais en tête
    if (s.finished()) addItem(Act::Close, "Terminer le lot");
    if (s.started && !s.entriesEnded) addItem(Act::EndEntries, "Fin des entrées");
    if (s.u.valid && now - s.u.t <= UNDO_MAX_S) addItem(Act::Undo, "Annuler décalage");
    if (s.nCrates > 1 && !s.finished()) addItem(Act::Remove, "Retirer dern. caisse");
    if (!s.finished()) addItem(Act::Close, "Terminer le lot");
  }
  if (app.topAlarm()) addItem(Act::Ack, "Acquitter alarmes");
  addItem(Act::Info, "Infos appareil");
  addItem(Act::Back, "Retour");
  scr = Scr::Menu;
}

static void showMessage(const String& a, const String& b = String("")) {
  msg1 = a;
  msg2 = b;
  msgUntil = millis() + 3000;
  scr = Scr::Message;
}

static const char* confirmText(Act a, int step) {
  switch (a) {
    case Act::Undo: return "Annuler le dernier décalage ?";
    case Act::Remove: return "Retirer la dernière caisse entrée ?";
    case Act::EndEntries: return "Plus aucune caisse à entrer ?";
    case Act::Close:
      if (app.sess.finished()) return "Terminer le lot et faire le relevé ?";
      return step == 0 ? "Des caisses sont encore au bain !"
                       : "Elles seront NON CONFORMES. Terminer ?";
    default: return "";
  }
}

static void execute(Act a) {
  String err;
  switch (a) {
    case Act::Undo: err = app.undoShift(); break;
    case Act::Remove: err = app.removeLast(); break;
    case Act::EndEntries: err = app.endEntries(); break;
    case Act::Close: {
      String lot = app.meta.lot;
      bool conf = app.sess.lotConforme();
      err = app.closeSession("", !app.sess.finished());
      if (!err.length()) {
        showMessage(String("Lot ") + lot, conf ? "CONFORME - relevé sur la page web" : "NON CONFORME - voir le relevé");
        return;
      }
      break;
    }
    default: break;
  }
  if (err.length()) showMessage("Impossible", err);
  else showMessage("Fait");
}

static void drawMenu() {
  font(F_SMALL);
  text(0, 8, app.mode == Mode::Session ? "MENU DU LOT" : "MENU");
  textR(8, hm());
  u8g2_DrawHLine(g, 0, 11, 128);
  if (menuSel < menuTop) menuTop = menuSel;
  if (menuSel > menuTop + 3) menuTop = menuSel - 3;
  for (int r = 0; r < 4 && menuTop + r < menuN; r++) {
    int i = menuTop + r, y = 14 + 12 * r;
    if (i == menuSel) { u8g2_DrawBox(g, 0, y, 128, 12); u8g2_SetDrawColor(g, 0); }
    text(4, y + 10, menuLabels[i]);
    u8g2_SetDrawColor(g, 1);
  }
}

static void drawWizard() {
  font(F_SMALL);
  if (scr == Scr::WizConfirm) {
    text(0, 8, "Vérifier le lot");
    u8g2_DrawHLine(g, 0, 11, 128);
    font(F_MED);
    text(0, 26, lotName(wizLot));
    font(F_SMALL);
    text(0, 38, String("Bouteilles ") + String(wizT0, 1) + " °C");
    text(0, 49, String("Cible ") + String(wizTarget, 1) + " UP");
    u8g2_DrawBox(g, 0, 53, 128, 11);
    u8g2_SetDrawColor(g, 0);
    textC(62, "OK : DÉMARRER");
    u8g2_SetDrawColor(g, 1);
    return;
  }
  int k = scr == Scr::WizLot ? 0 : (scr == Scr::WizT0 ? 1 : 2);
  const char* steps[] = {"Nouveau lot 1/3", "Nouveau lot 2/3", "Nouveau lot 3/3"};
  const char* label[] = {"N° de lot", "T° des bouteilles", "Cible (bain seul)"};
  text(0, 8, steps[k]);
  u8g2_DrawHLine(g, 0, 11, 128);
  text(0, 23, label[k]);
  font(F_BIG);
  String v = k == 0 ? lotName(wizLot) : (k == 1 ? String(wizT0, 1) + " °C" : String(wizTarget, 1) + " UP");
  textC(46, v);
  arrowsHint(63);
}

static void drawConfirm() {
  font(F_SMALL);
  text(0, 8, "CONFIRMER");
  u8g2_DrawHLine(g, 0, 11, 128);
  String l[3];
  int n = wrap(confirmText(pending, confirmStep), l, 3, 21);
  for (int i = 0; i < n; i++) textC(25 + 11 * i, l[i]);
  u8g2_DrawBox(g, 0, 53, 128, 11);
  u8g2_SetDrawColor(g, 0);
  textC(62, "OK oui  OK long non");
  u8g2_SetDrawColor(g, 1);
}

static void drawInfo() {
  font(F_SMALL);
  text(0, 8, "INFOS");
  textR(8, String("v") + FW_VERSION);
  u8g2_DrawHLine(g, 0, 11, 128);
  text(0, 23, ipText());
  text(0, 34, String("Tém ") + probeTxt(P_TEMOIN) + "  B1 " + probeTxt(P_BAIN1));
  text(0, 45, String("B2 ") + probeTxt(P_BAIN2) + "  B3 " + probeTxt(P_BAIN3));
  text(0, 56, clockValid() ? fmtDateTime(nowEpoch()).substring(0, 16) : String("heure non réglée"));
}

static void drawMessage() {
  font(F_MED);
  textC(26, msg1);
  font(F_SMALL);
  String l[3];
  int n = wrap(msg2, l, 3, 21);
  for (int i = 0; i < n; i++) textC(42 + 10 * i, l[i]);
}

// ---------------------------------------------------------------------------
void uiOnKey(Key k) {
  if (k == Key::None) return;
  lastKey = millis();
  switch (scr) {
    case Scr::Main:
      if (k == Key::Ok) {
        const Alarm* al = app.topAlarm();
        if (al && al->sounding && app.silencedUntilMs == 0) { app.acknowledge(); return; }
        openMenu();
      }
      break;
    case Scr::Menu:
      if (k == Key::Up) menuSel = (menuSel + menuN - 1) % menuN;
      else if (k == Key::Down) menuSel = (menuSel + 1) % menuN;
      else if (k == Key::OkLong) scr = Scr::Main;
      else if (k == Key::Ok) {
        Act a = menuActs[menuSel];
        if (a == Act::Back) scr = Scr::Main;
        else if (a == Act::Info) scr = Scr::Info;
        else if (a == Act::Ack) { app.acknowledge(); scr = Scr::Main; }
        else if (a == Act::StartLot) {
          if (!clockValid()) { showMessage("Heure non réglée", "à régler sur la page web"); break; }
          wizLot = settings.lotNext;
          wizT0 = roundf(settings.t0 * 2) / 2;
          wizTarget = roundf(settings.target * 2) / 2;
          scr = Scr::WizLot;
        } else { pending = a; confirmStep = 0; scr = Scr::Confirm; }
      }
      break;
    case Scr::WizLot:
      if (k == Key::Up) wizLot++;
      else if (k == Key::Down && wizLot > 1) wizLot--;
      else if (k == Key::Ok) scr = Scr::WizT0;
      else if (k == Key::OkLong) openMenu();
      break;
    case Scr::WizT0:
      if (k == Key::Up && wizT0 < 25) wizT0 += 0.5f;
      else if (k == Key::Down && wizT0 > 0) wizT0 -= 0.5f;
      else if (k == Key::Ok) scr = Scr::WizTarget;
      else if (k == Key::OkLong) scr = Scr::WizLot;
      break;
    case Scr::WizTarget:
      if (k == Key::Up && wizTarget < 200) wizTarget += 0.5f;
      else if (k == Key::Down && wizTarget > 1) wizTarget -= 0.5f;
      else if (k == Key::Ok) scr = Scr::WizConfirm;
      else if (k == Key::OkLong) scr = Scr::WizT0;
      break;
    case Scr::WizConfirm:
      if (k == Key::Ok) {
        String err = app.startSession(lotName(wizLot), settings.produit, settings.operateur, wizT0, wizTarget);
        if (err.length()) showMessage("Impossible", err);
        else scr = Scr::Main;
      } else if (k == Key::OkLong) scr = Scr::WizTarget;
      break;
    case Scr::Confirm:
      if (k == Key::Ok) {
        if (pending == Act::Close && !app.sess.finished() && confirmStep == 0) { confirmStep = 1; break; }
        execute(pending);
      } else if (k == Key::OkLong) openMenu();
      break;
    case Scr::Info:
    case Scr::Message:
      if (k == Key::Ok || k == Key::OkLong) scr = Scr::Main;
      break;
  }
}

void displayDraw() {
  u8g2_ClearBuffer(g);
  u8g2_SetFontMode(g, 1);
  u8g2_SetDrawColor(g, 1);
  switch (scr) {
    case Scr::Main:
      if (app.mode == Mode::Session) drawMainSession();
      else if (app.mode == Mode::Mesure) drawMainMesure();
      else drawMainIdle();
      break;
    case Scr::Menu: drawMenu(); break;
    case Scr::WizLot: case Scr::WizT0: case Scr::WizTarget: case Scr::WizConfirm: drawWizard(); break;
    case Scr::Confirm: drawConfirm(); break;
    case Scr::Info: drawInfo(); break;
    case Scr::Message: drawMessage(); break;
  }
}

void displayLoop() {
  uint32_t now = millis();
  // retours automatiques
  if (scr == Scr::Message && now > msgUntil) scr = Scr::Main;
  if (scr != Scr::Main && scr != Scr::Message && now - lastKey > MENU_TIMEOUT_MS) scr = Scr::Main;
  // une confirmation de session n'a plus de sens si le lot a été clôturé ailleurs (page web)
  if (scr == Scr::Confirm && app.mode != Mode::Session) scr = Scr::Main;
  if (now - last < 200) return;
  last = now;
  displayDraw();
  u8g2_SendBuffer(g);
}

void displayBegin() {
#ifdef ARDUINO
  oled.begin();
#ifdef DISPLAY_SSD1309_SPI
  oled.setBusClock(8000000);
#endif
  g = oled.getU8g2();
#endif
}

void displayMessage(const char* l1, const char* l2) {
  if (!g) return;
  u8g2_ClearBuffer(g);
  u8g2_SetFontMode(g, 1);
  u8g2_SetDrawColor(g, 1);
  font(F_MED);
  textC(28, l1);
  if (l2) { font(F_SMALL); textC(46, l2); }
  u8g2_SendBuffer(g);
}

// Écran de démarrage : logo en pixels (tools/logo.py), nom et version au-dessus de « Catch ».
void displaySplash() {
  if (!g) return;
  u8g2_ClearBuffer(g);
  u8g2_SetFontMode(g, 1);
  u8g2_SetDrawColor(g, 1);
  const int x = (128 - LOGO_W) / 2, y = 64 - LOGO_H - 2;
  u8g2_DrawXBMP(g, x, y, LOGO_W, LOGO_H, LOGO_BITS);
  font(F_SMALL);
  text(x, y + 10, "compteur UP");
  text(x, y + 21, "v" FW_VERSION);
  u8g2_SendBuffer(g);
}
