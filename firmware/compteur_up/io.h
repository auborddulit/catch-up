// io.h — Gros bouton DÉCALAGE et buzzer.
#pragma once
#include <Arduino.h>

enum class Btn : uint8_t { None, Short, Long };
enum class Beep : uint8_t { Off, Shift, Alarm };

void ioBegin();
Btn  buttonPoll();          // à appeler souvent ; renvoie un événement au plus
void buzzerSet(Beep mode);  // motif continu
void buzzerClick();         // bip court de confirmation
void buzzerLoop();
void ledBlink();
