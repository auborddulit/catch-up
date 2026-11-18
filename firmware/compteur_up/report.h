// report.h — Relevés de fin de lot (HTML imprimable + CSV) et étalonnage du modèle.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "app.h"

bool writeCratesCsv(const RunMeta& m, const up::Session& s);
bool writeSessionReport(const RunMeta& m, const up::Session& s, const String& comment);
bool writeMesureReport(const RunMeta& m, const up::Measure& me, const String& comment);
// Ajuste le modèle sur la mesure enregistrée dans « dir ». Renvoie "" si OK.
String fitFromDir(const String& dir, JsonObject out);
