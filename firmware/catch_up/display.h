// display.h — Écran et interface sur l'appareil (boutons ▲ ▼ OK, menus).
#pragma once
#include "io.h"

void displayBegin();
void displayLoop();                    // rafraîchit l'écran (au plus 5 fois par seconde)
void displayMessage(const char* l1, const char* l2 = nullptr);
void displaySplash();                  // logo « Catch-UP » au démarrage
void uiOnKey(Key k);                   // touche ▲ ▼ OK
