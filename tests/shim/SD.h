#pragma once
#include "FS.h"
#include <sys/stat.h>
#include <unistd.h>
#define CARD_NONE 0
#define CARD_SD 2
struct SPIClass { void begin(int, int, int, int) {} };
extern SPIClass SPI;
struct SDStub {
  std::string root = "sd";
  std::string p(const String& x) { return root + x.s; }
  bool begin(int, SPIClass&, int) { ::mkdir(root.c_str(), 0755); return true; }
  int cardType() { return CARD_SD; }
  File open(const String& x, const char* m = FILE_READ) { return File(fopen(p(x).c_str(), m)); }
  bool exists(const String& x) { struct stat st; return stat(p(x).c_str(), &st) == 0; }
  bool mkdir(const String& x) { return ::mkdir(p(x).c_str(), 0755) == 0; }
  bool remove(const String& x) { return ::remove(p(x).c_str()) == 0; }
  bool rename(const String& a, const String& b) { return ::rename(p(a).c_str(), p(b).c_str()) == 0; }
};
extern SDStub SD;
