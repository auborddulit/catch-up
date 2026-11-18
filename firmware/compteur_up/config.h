// config.h — Brochage et constantes matérielles.
#pragma once

#define FW_VERSION "1.1.0"
#define DEVICE_NAME "compteur-up"   // nom réseau : http://compteur-up.local

// --- Sondes DS18B20 : une broche (un bus 1-Wire) par sonde -----------------
// Chaque sonde est identifiée par sa prise : pas d'adresse à configurer.
// Résistance de tirage 4,7 kΩ entre DATA et 3,3 V sur chaque broche.
#define PIN_T_TEMOIN  4    // bouteille témoin
#define PIN_T_BAIN1   16   // bain, place 1 = entrée (droite)
#define PIN_T_BAIN2   17   // bain, place 2 = milieu
#define PIN_T_BAIN3   25   // bain, place 3 = sortie (gauche)

// --- Bus I2C : écran OLED SH1106 1,3" + horloge DS3231 ----------------------
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

// --- Carte microSD (SPI) -----------------------------------------------------
#define PIN_SD_CS     5    // SCK 18, MISO 19, MOSI 23
#define PIN_SD_SCK    18
#define PIN_SD_MISO   19
#define PIN_SD_MOSI   23

// --- Interface ---------------------------------------------------------------
#define PIN_BUTTON    32   // gros bouton DÉCALAGE (vers GND, pull-up interne)
#define PIN_BUZZER    27   // buzzer actif via transistor NPN
#define PIN_LED       2    // LED de la carte (clignote à chaque mesure)

// --- Temps -------------------------------------------------------------------
#define SAMPLE_MS        2000UL   // période de mesure
#define PERSIST_EVERY_S  30       // sauvegarde de l'état pour reprise
#define UNDO_MAX_S       60       // délai pour annuler un décalage
#define LONG_PRESS_MS    2000UL   // appui long = acquitter / couper l'alarme
#define SILENCE_MS       (5UL * 60UL * 1000UL)
#define TZ_PARIS         "CET-1CEST,M3.5.0,M10.5.0/3"

// --- Réseau de secours (si le Wi-Fi configuré est absent) --------------------
#define AP_SSID   "CompteurUP"
#define AP_PASS   "pasteur2026"     // 8 caractères minimum
