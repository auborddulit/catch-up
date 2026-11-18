// io.cpp
#include "io.h"
#include "config.h"

// Bouton avec anti-rebond (actif à l'état bas)
struct Button {
  uint8_t pin;
  bool last = HIGH, stable = HIGH, longSent = false;
  uint32_t tChange = 0, tDown = 0, tRepeat = 0;
  // met à jour l'état ; renvoie +1 à l'appui, -1 au relâchement, 0 sinon
  int update(uint32_t now) {
    bool lv = digitalRead(pin);
    if (lv != last) { last = lv; tChange = now; }
    if (now - tChange > 30 && lv != stable) {
      stable = lv;
      if (stable == LOW) { tDown = now; tRepeat = now; longSent = false; return +1; }
      return -1;
    }
    return 0;
  }
  bool down() const { return stable == LOW; }
};

static Button bShift{PIN_BUTTON}, bUp{PIN_KEY_UP}, bDown{PIN_KEY_DOWN}, bOk{PIN_KEY_OK};
static Beep mode = Beep::Off;
static uint32_t clickUntil = 0, ledUntil = 0;

void ioBegin() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  // 34, 35, 39 : entrée seule, sans pull-up interne (10 kΩ externes)
  pinMode(PIN_KEY_UP, INPUT);
  pinMode(PIN_KEY_DOWN, INPUT);
  pinMode(PIN_KEY_OK, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_LED, OUTPUT);
}

Btn buttonPoll() {
  uint32_t now = millis();
  int e = bShift.update(now);
  if (e < 0 && !bShift.longSent) return Btn::Short;       // relâché avant 2 s
  if (bShift.down() && !bShift.longSent && now - bShift.tDown >= LONG_PRESS_MS) {
    bShift.longSent = true;
    return Btn::Long;
  }
  return Btn::None;
}

Key keyPoll() {
  uint32_t now = millis();
  // ▲ ▼ : action à l'appui, puis répétition (après 500 ms, toutes les 120 ms)
  for (Button* b : {&bUp, &bDown}) {
    int e = b->update(now);
    Key k = (b == &bUp) ? Key::Up : Key::Down;
    if (e > 0) return k;
    if (b->down() && now - b->tDown > 500 && now - b->tRepeat > 120) {
      b->tRepeat = now;
      return k;
    }
  }
  // OK : court au relâchement, long (retour) au bout de 1 s
  int e = bOk.update(now);
  if (e < 0 && !bOk.longSent) return Key::Ok;
  if (bOk.down() && !bOk.longSent && now - bOk.tDown >= OK_LONG_MS) {
    bOk.longSent = true;
    return Key::OkLong;
  }
  return Key::None;
}

void buzzerSet(Beep m) { mode = m; }
void buzzerClick() { clickUntil = millis() + 60; }
void ledBlink() { ledUntil = millis() + 40; }

void buzzerLoop() {
  uint32_t now = millis();
  bool on = false;
  if (mode == Beep::Shift) {
    on = (now % 1000) < 180;                       // bip par seconde
  } else if (mode == Beep::Alarm) {
    uint32_t ph = now % 1200;                      // 3 bips rapides
    on = (ph < 600) && ((ph % 200) < 110);
  }
  if (now < clickUntil) on = true;
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  digitalWrite(PIN_LED, now < ledUntil ? HIGH : LOW);
}
