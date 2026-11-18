// io.h — Gros bouton DÉCALAGE, boutons ▲ ▼ OK et buzzer.
#pragma once
#include <Arduino.h>

enum class Btn : uint8_t { None, Short, Long };
enum class Key : uint8_t { None, Up, Down, Ok, OkLong };
enum class Beep : uint8_t { Off, Shift, Alarm };

void ioBegin();
Btn  buttonPoll();          // bouton DÉCALAGE : un événement au plus par appel
Key  keyPoll();             // ▲ ▼ (avec répétition si maintenu), OK court / long
void buzzerSet(Beep mode);  // motif continu
void buzzerClick();         // bip court de confirmation
void buzzerLoop();
void ledBlink();
