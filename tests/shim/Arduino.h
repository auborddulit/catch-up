// Arduino.h — émulation minimale pour compiler la logique du firmware sur PC (tests).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>
#include <string>
#include <algorithm>
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#define PROGMEM
#define FPSTR(p) (p)
#define HIGH 1
#define LOW 0
// strlcpy existe dans la glibc >= 2.38 ; sinon la définir.
uint32_t millis();
inline void delay(uint32_t) {}

class String {
 public:
  std::string s;
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& x) : s(x) {}
  String(char c) : s(1, c) {}
  String(int v) : s(std::to_string(v)) {}
  String(unsigned v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  String(long long v) : s(std::to_string(v)) {}
  String(unsigned long long v) : s(std::to_string(v)) {}
  String(float v, unsigned char d = 2) { fmt(v, d); }
  String(double v, unsigned char d = 2) { fmt(v, d); }
  String& operator=(const char* c) { s = c ? c : ""; return *this; }
  const char* c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }
  bool isEmpty() const { return s.empty(); }
  void reserve(size_t n) { s.reserve(n); }
  bool concat(const char* c) { s += c; return true; }
  String substring(size_t a) const { return a >= s.size() ? String() : String(s.substr(a)); }
  String substring(size_t a, size_t b) const { if (b > s.size()) b = s.size(); if (a >= b) return String(); return String(s.substr(a, b - a)); }
  int indexOf(char c, size_t from = 0) const { auto p = s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String& x, size_t from = 0) const { auto p = s.find(x.s, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const char* x) const { return indexOf(String(x)); }
  int lastIndexOf(char c) const { auto p = s.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c, int from) const { if (from < 0) return -1; auto p = s.rfind(c, from); return p == std::string::npos ? -1 : (int)p; }
  bool startsWith(const String& x) const { return s.compare(0, x.s.size(), x.s) == 0; }
  bool endsWith(const String& x) const { return s.size() >= x.s.size() && s.compare(s.size() - x.s.size(), x.s.size(), x.s) == 0; }
  void replace(const String& a, const String& b) { if (a.s.empty()) return; size_t p = 0; while ((p = s.find(a.s, p)) != std::string::npos) { s.replace(p, a.s.size(), b.s); p += b.s.size(); } }
  void remove(size_t i) { if (i < s.size()) s.erase(i); }
  void trim() { size_t a = s.find_first_not_of(" \t\r\n"); if (a == std::string::npos) { s.clear(); return; } size_t b = s.find_last_not_of(" \t\r\n"); s = s.substr(a, b - a + 1); }
  float toFloat() const { return (float)atof(s.c_str()); }
  long toInt() const { return atol(s.c_str()); }
  char operator[](size_t i) const { return s[i]; }
  char& operator[](size_t i) { return s[i]; }
  String& operator+=(const String& x) { s += x.s; return *this; }
  String& operator+=(const char* x) { s += x; return *this; }
  String& operator+=(char c) { s += c; return *this; }
  bool operator==(const String& x) const { return s == x.s; }
  bool operator==(const char* x) const { return s == x; }
  bool operator!=(const String& x) const { return s != x.s; }
 private:
  void fmt(double v, int d) { char b[64]; snprintf(b, sizeof b, "%.*f", d, v); s = b; }
};
inline String operator+(const String& a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const char* a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const String& a, const char* b) { String r(a); r += b; return r; }
inline String operator+(const String& a, char b) { String r(a); r += b; return r; }
inline String operator+(const String& a, int b) { return a + String(b); }
inline String operator+(const String& a, unsigned b) { return a + String(b); }
inline String operator+(const String& a, long b) { return a + String(b); }
inline String operator+(const String& a, unsigned long b) { return a + String(b); }

struct SerialStub { void println(const String& s) { (void)s; } void println(const char*) {} void begin(int) {} };
extern SerialStub Serial;
