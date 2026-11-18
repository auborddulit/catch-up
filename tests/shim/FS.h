#pragma once
#include "Arduino.h"
#include <cstdarg>
#define FILE_READ "rb"
#define FILE_WRITE "wb"
#define FILE_APPEND "ab"
// Fichier sur disque du PC (racine = dossier courant + /sd)
class File {
 public:
  FILE* f = nullptr;
  bool dir = false;
  File() {}
  explicit File(FILE* x) : f(x) {}
  explicit operator bool() const { return f != nullptr; }
  size_t print(const String& s) { return fwrite(s.c_str(), 1, s.length(), f); }
  size_t print(const char* s) { return fwrite(s, 1, strlen(s), f); }
  size_t printf(const char* fmt, ...) { va_list a; va_start(a, fmt); int n = vfprintf(f, fmt, a); va_end(a); return n; }
  size_t write(const uint8_t* b, size_t n) { return fwrite(b, 1, n, f); }
  size_t read(uint8_t* b, size_t n) { return fread(b, 1, n, f); }
  int read() { return fgetc(f); }
  int available() { int c = fgetc(f); if (c == EOF) return 0; ungetc(c, f); return 1; }
  String readStringUntil(char t) { String r; int c; while ((c = fgetc(f)) != EOF && c != t) r += (char)c; return r; }
  size_t size() { long p = ftell(f); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, p, SEEK_SET); return n; }
  bool isDirectory() const { return dir; }
  void close() { if (f) fclose(f); f = nullptr; }
};
