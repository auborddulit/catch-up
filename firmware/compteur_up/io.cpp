// io.cpp
#include "io.h"
#include "config.h"

static bool lastLevel = HIGH, stable = HIGH, longSent = false;
static uint32_t tChange = 0, tDown = 0;
static Beep mode = Beep::Off;
static uint32_t clickUntil = 0, ledUntil = 0;

void ioBegin() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_LED, OUTPUT);
}

Btn buttonPoll() {
  uint32_t now = millis();
  bool lv = digitalRead(PIN_BUTTON);
  if (lv != lastLevel) { lastLevel = lv; tChange = now; }
  if (now - tChange > 30 && lv != stable) {   // anti-rebond 30 ms
    stable = lv;
    if (stable == LOW) { tDown = now; longSent = false; }
    else if (!longSent) return Btn::Short;     // relâché avant 2 s
  }
  if (stable == LOW && !longSent && now - tDown >= LONG_PRESS_MS) {
    longSent = true;
    return Btn::Long;
  }
  return Btn::None;
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
