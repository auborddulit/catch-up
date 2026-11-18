// notify.h — Notifications ntfy.
#pragma once
#include <Arduino.h>

void notifyBegin();
// prio : 1 (min) .. 5 (urgent). tags : émojis ntfy, ex. "warning" ou "white_check_mark".
void notify(const String& title, const String& body, uint8_t prio = 3, const char* tags = nullptr);
int notifyLastCode();   // dernier code HTTP (200 = OK), pour le test depuis la page web
