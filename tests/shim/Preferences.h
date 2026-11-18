#pragma once
#include "Arduino.h"
struct Preferences { void begin(const char*, bool) {} void end() {} String getString(const char*, const char* d) { return d; } void putString(const char*, const String&) {} };
