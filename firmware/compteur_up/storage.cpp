// storage.cpp
#include "storage.h"
#include <SPI.h>
#include "config.h"

static bool ok = false;

bool sdBegin() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  ok = SD.begin(PIN_SD_CS, SPI, 10000000);
  if (ok) {
    if (!SD.exists("/sessions")) SD.mkdir("/sessions");
    if (!SD.exists("/mesures")) SD.mkdir("/mesures");
  }
  return ok;
}

bool sdOk() { return ok && SD.cardType() != CARD_NONE; }

bool appendLine(const String& path, const String& line) {
  if (!ok) return false;
  File f = SD.open(path, FILE_APPEND);
  if (!f) return false;
  f.print(line);
  f.print("\r\n");
  f.close();
  return true;
}

bool writeText(const String& path, const String& text) {
  if (!ok) return false;
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;
  f.print(text);
  f.close();
  return true;
}

String readText(const String& path, size_t maxLen) {
  String s;
  if (!ok) return s;
  File f = SD.open(path, FILE_READ);
  if (!f) return s;
  size_t n = f.size();
  if (n > maxLen) n = maxLen;
  s.reserve(n + 1);
  while (f.available() && s.length() < n) s += (char)f.read();
  f.close();
  return s;
}

bool writeBinaryAtomic(const char* path, const uint8_t* parts[], const size_t sizes[], int n) {
  if (!ok) return false;
  String tmp = String(path) + ".tmp";
  File f = SD.open(tmp, FILE_WRITE);
  if (!f) return false;
  for (int i = 0; i < n; i++) {
    if (f.write(parts[i], sizes[i]) != sizes[i]) { f.close(); return false; }
  }
  f.close();
  SD.remove(path);
  return SD.rename(tmp, path);
}

String csvField(const String& s) {
  if (s.indexOf(',') < 0 && s.indexOf('"') < 0 && s.indexOf('\n') < 0) return s;
  String r = "\"";
  for (size_t i = 0; i < s.length(); i++) {
    if (s[i] == '"') r += '"';
    r += s[i];
  }
  return r + "\"";
}

String htmlEsc(const String& s) {
  String r;
  r.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') r += "&lt;";
    else if (c == '>') r += "&gt;";
    else if (c == '&') r += "&amp;";
    else if (c == '"') r += "&quot;";
    else if (c == '\n') r += "<br>";
    else r += c;
  }
  return r;
}

String safeName(const String& s) {
  String r;
  for (size_t i = 0; i < s.length() && r.length() < 24; i++) {
    char c = s[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_') r += c;
    else if (c == ' ' || c == '/' || c == '.') r += '_';
  }
  return r;
}
