// display.cpp
#include "display.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include "app.h"
#include "sensors.h"
#include "clock.h"

// SH1106 1,3". Pour un SSD1306 0,96", remplacer par :
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
static uint32_t last = 0;

void displayBegin() {
  oled.begin();
  oled.enableUTF8Print();
}

void displayMessage(const char* l1, const char* l2) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_helvB10_tf);
  oled.drawUTF8(0, 24, l1);
  if (l2) { oled.setFont(u8g2_font_6x10_tf); oled.drawUTF8(0, 44, l2); }
  oled.sendBuffer();
}

static String t1(float v) {
  if (std::isnan(v)) return String("--.-");
  return String(v, 1);
}

static String probeTxt(int i) {
  const ProbeState& p = probe(i);
  return p.ok ? t1(p.value) : String("--.-");
}

static String mmss(float s) {
  if (s < 0) return String("--:--");
  char b[24];
  snprintf(b, sizeof b, "%d:%02d", (int)s / 60, (int)s % 60);
  return String(b);
}

// Bandeau du bas : grand message (DÉCALER, alarme...) ou compte à rebours
static void footer(const char* big, const String& small) {
  if (big) {
    bool blink = (millis() / 400) % 2;
    if (blink) {
      oled.drawBox(0, 47, 128, 17);
      oled.setDrawColor(0);
    }
    oled.setFont(u8g2_font_helvB12_tf);
    int w = oled.getUTF8Width(big);
    oled.drawUTF8((128 - w) / 2, 62, big);
    oled.setDrawColor(1);
  } else {
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawUTF8(0, 60, small.c_str());
  }
}

void displayLoop() {
  if (millis() - last < 250) return;
  last = millis();
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  uint32_t now = nowEpoch();
  String hm = clockValid() ? fmtTime(now).substring(0, 5) : String("--:--");

  // ligne eau, commune à tous les modes (ordre physique : sortie à gauche)
  String bath = String("Eau ") + probeTxt(P_BAIN3) + " " + probeTxt(P_BAIN2) + " " + probeTxt(P_BAIN1);

  const Alarm* al = app.topAlarm();
  if (app.mode == Mode::Idle) {
    oled.drawUTF8(0, 9, (String("REPOS  ") + hm).c_str());
    oled.drawUTF8(0, 21, bath.c_str());
    oled.drawUTF8(0, 33, (String("Témoin ") + probeTxt(P_TEMOIN) + "  Aux " + probeTxt(P_AUX)).c_str());
    String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                              : String("AP ") + WiFi.softAPIP().toString();
    oled.drawUTF8(0, 45, ip.c_str());
    footer(nullptr, clockValid() ? String("Prêt") : String("Heure à régler"));
  } else if (app.mode == Mode::Session) {
    oled.drawUTF8(0, 9, (String("Lot ") + app.meta.lot).substring(0, 15).c_str());
    oled.drawUTF8(98, 9, hm.c_str());
    oled.drawUTF8(0, 21, bath.c_str());
    int i = app.sess.slot[up::NPOS - 1];
    String l3, l4;
    if (!app.sess.started) {
      l3 = "Témoin -> caisse 1";
      l4 = "puis appuyer";
    } else if (i >= 0) {
      const up::Crate& c = app.sess.crates[i];
      l3 = String("Sortie C") + c.id + " " + String(app.sess.upNow(c), 1) + "/" + String(app.sess.p.target, 0) + " UP";
    } else {
      l3 = "Remplissage du tunnel";
    }
    if (app.sess.started && app.sess.nCrates > 0 && app.sess.crates[0].pos >= 0) {
      const up::Crate& w = app.sess.crates[0];
      l4 = String("Témoin ") + probeTxt(P_TEMOIN) + " " + String(w.upMeas, 1) + "UP";
    } else if (app.sess.rKnown) {
      l4 = String("Témoin/modèle ") + String(app.sess.r, 2);
    }
    oled.drawUTF8(0, 33, l3.c_str());
    oled.drawUTF8(0, 44, l4.c_str());
    if (al && al->sounding) footer("ALARME", "");
    else if (app.sess.finished()) footer(nullptr, "Terminé : clôturer");
    else if (app.ready) footer("DÉCALER", "");
    else footer(nullptr, String("Décaler ~") + mmss(app.eta) + "  C" + app.sess.nCrates);
  } else {
    const char* where[] = {"avant entrée", "sorti"};
    String pos = app.mes.pos >= 0 ? String("place ") + (app.mes.pos + 1)
                                  : String(app.mes.pos == -2 ? where[0] : where[1]);
    oled.drawUTF8(0, 9, (String("MESURE ") + pos).c_str());
    oled.drawUTF8(98, 9, hm.c_str());
    oled.drawUTF8(0, 21, bath.c_str());
    oled.drawUTF8(0, 33, (String("Témoin ") + probeTxt(P_TEMOIN) + " °C").c_str());
    oled.drawUTF8(0, 44, (String("UP bain ") + String(app.mes.upBath, 1) + " tot " + String(app.mes.upTotal, 1)).c_str());
    if (al && al->sounding) footer("ALARME", "");
    else if (app.ready) footer("DÉCALER", "");
    else if (app.mes.pos >= 0) footer(nullptr, String("Décalage nominal ") + mmss(app.eta));
    else footer(nullptr, app.mes.pos == -2 ? String("Appuyer : entrée") : String("Arrêter la mesure"));
  }
  oled.sendBuffer();
}
