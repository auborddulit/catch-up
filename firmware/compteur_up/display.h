// display.h — Écran OLED 1,3" (SH1106, 128×64, I2C).
#pragma once
void displayBegin();
void displayLoop();   // rafraîchit l'écran (au plus 4 fois par seconde)
void displayMessage(const char* l1, const char* l2 = nullptr);
