// archive.h — Sauvegarde des lots sur le téléphone : fichier .zip par lot,
// notification « Télécharger / Commenter », rappel des lots non sauvegardés.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

// Écrit le .zip d'un dossier de lot ou de mesure (fichiers stockés sans
// compression) dans « sink ». Renvoie la taille totale, 0 en cas d'erreur.
// Avec dryRun, calcule seulement la taille (et les CRC) sans rien écrire.
using ZipSink = std::function<bool(const uint8_t*, size_t)>;
size_t zipFolder(const String& dir, ZipSink sink, bool dryRun = false);

// Nom du fichier proposé au téléchargement, ex. « 20261119-1432_L0128.zip »
String zipName(const String& dir);

// Liste des lots à sauvegarder (fichier /a_sauvegarder.txt sur la carte SD)
void archivePending(const String& dir, const String& lot, bool conforme, bool updated);
void archiveDownloaded(const String& dir);
bool archiveIsPending(const String& dir);
void archiveList(JsonArray out);
void archiveLoop();   // rappels : 24 h après la clôture, puis chaque jour

// URL de l'appareil vue du téléphone (adresse IP sur le Wi-Fi de l'atelier)
String deviceUrl();
