// storage.h — Carte microSD : relevés, journaux, état de reprise.
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>

bool sdBegin();
bool sdOk();
bool appendLine(const String& path, const String& line);
bool writeText(const String& path, const String& text);
String readText(const String& path, size_t maxLen = 8192);
bool writeBinaryAtomic(const char* path, const uint8_t* parts[], const size_t sizes[], int n);
// Échappe un champ CSV (séparateur virgule).
String csvField(const String& s);
// Échappe un texte pour l'insérer dans du HTML.
String htmlEsc(const String& s);
// Nom de fichier sûr à partir d'un texte libre (n° de lot).
String safeName(const String& s);
