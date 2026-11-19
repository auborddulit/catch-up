// archive.cpp
#include "archive.h"
#include <SD.h>
#include <vector>
#include "storage.h"
#include "clock.h"
#include "settings.h"
#include "notify.h"
#include "bg.h"
#ifdef ARDUINO
#include <WiFi.h>
#endif

static const char* PENDING_PATH = "/a_sauvegarder.txt";
static const char* FILES[] = {"releve.html", "caisses.csv", "events.csv", "data.csv", "meta.json"};

// ---------------------------------------------------------------------------
// ZIP (méthode « stocké », sans compression) — format PKWARE APPNOTE
// ---------------------------------------------------------------------------
static uint32_t crcTable[256];
static void crcInit() {
  if (crcTable[1]) return;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    crcTable[i] = c;
  }
}

struct Entry { String name, path; uint32_t crc = 0, size = 0, offset = 0; };

static void le16(uint8_t* p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static void le32(uint8_t* p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

String zipName(const String& dir) {
  int k = dir.lastIndexOf('/');
  return dir.substring(k + 1) + ".zip";
}

size_t zipFolder(const String& dir, ZipSink sink, bool dryRun) {
  crcInit();
  String base = zipName(dir);
  base = base.substring(0, base.length() - 4) + "/";
  std::vector<Entry> es;
  static uint8_t buf[1024];
  // passe 1 : tailles et CRC
  for (const char* f : FILES) {
    String p = dir + "/" + f;
    File in = SD.open(p, FILE_READ);
    if (!in) continue;
    Entry e;
    e.name = base + f;
    e.path = p;
    uint32_t c = 0xFFFFFFFFu;
    size_t n;
    while ((n = in.read(buf, sizeof buf)) > 0) {
      for (size_t i = 0; i < n; i++) c = crcTable[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
      e.size += n;
      backgroundTick();
    }
    in.close();
    e.crc = c ^ 0xFFFFFFFFu;
    es.push_back(e);
  }
  if (es.empty()) return 0;
  // date DOS (heure locale)
  time_t t = nowEpoch();
  struct tm tm;
  localtime_r(&t, &tm);
  uint16_t dtime = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2);
  uint16_t ddate = ((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday;
  size_t total = 0, cdSize = 0;
  for (auto& e : es) { e.offset = total; total += 30 + e.name.length() + e.size; cdSize += 46 + e.name.length(); }
  size_t cdOffset = total;
  total += cdSize + 22;
  if (dryRun) return total;

  uint8_t h[46];
  // passe 2 : en-têtes locaux + données
  for (auto& e : es) {
    memset(h, 0, 30);
    le32(h, 0x04034b50); le16(h + 4, 20); le16(h + 6, 0x0800);  // noms en UTF-8
    le16(h + 8, 0); le16(h + 10, dtime); le16(h + 12, ddate);
    le32(h + 14, e.crc); le32(h + 18, e.size); le32(h + 22, e.size);
    le16(h + 26, e.name.length()); le16(h + 28, 0);
    if (!sink(h, 30) || !sink((const uint8_t*)e.name.c_str(), e.name.length())) return 0;
    File in = SD.open(e.path, FILE_READ);
    if (!in) return 0;
    size_t left = e.size, n;
    while (left && (n = in.read(buf, left < sizeof buf ? left : sizeof buf)) > 0) {
      if (!sink(buf, n)) { in.close(); return 0; }
      left -= n;
      backgroundTick();     // le bouton DÉCALAGE et les mesures restent actifs
    }
    in.close();
    if (left) return 0;   // fichier modifié entre les deux passes
  }
  // répertoire central
  for (auto& e : es) {
    memset(h, 0, 46);
    le32(h, 0x02014b50); le16(h + 4, 20); le16(h + 6, 20); le16(h + 8, 0x0800);
    le16(h + 10, 0); le16(h + 12, dtime); le16(h + 14, ddate);
    le32(h + 16, e.crc); le32(h + 20, e.size); le32(h + 24, e.size);
    le16(h + 28, e.name.length());
    le32(h + 42, e.offset);
    if (!sink(h, 46) || !sink((const uint8_t*)e.name.c_str(), e.name.length())) return 0;
  }
  memset(h, 0, 22);
  le32(h, 0x06054b50);
  le16(h + 8, es.size()); le16(h + 10, es.size());
  le32(h + 12, cdSize); le32(h + 16, cdOffset);
  if (!sink(h, 22)) return 0;
  return total;
}

// ---------------------------------------------------------------------------
// Lots à sauvegarder : « dossier|lot|clôture|dernier rappel » par ligne
// ---------------------------------------------------------------------------
struct Pend { String dir, lot; uint32_t closed = 0, reminded = 0; };

static std::vector<Pend> readPending() {
  std::vector<Pend> v;
  File f = SD.open(PENDING_PATH, FILE_READ);
  if (!f) return v;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    l.trim();
    int a = l.indexOf('|'), b = l.indexOf('|', a + 1), c = l.indexOf('|', b + 1);
    if (a < 0 || b < 0 || c < 0) continue;
    Pend p;
    p.dir = l.substring(0, a);
    p.lot = l.substring(a + 1, b);
    p.closed = (uint32_t)l.substring(b + 1, c).toInt();
    p.reminded = (uint32_t)l.substring(c + 1).toInt();
    v.push_back(p);
  }
  f.close();
  return v;
}

static void writePending(const std::vector<Pend>& v) {
  String s;
  for (auto& p : v) s += p.dir + "|" + p.lot + "|" + String(p.closed) + "|" + String(p.reminded) + "\n";
  writeText(PENDING_PATH, s);
}

String deviceUrl() {
#ifdef ARDUINO
  IPAddress ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP();
  return String("http://") + ip.toString();
#else
  return String("http://192.168.1.50");
#endif
}

static void sendLinks(const String& dir, const String& title, const String& body, uint8_t prio,
                      const char* tags) {
  String url = deviceUrl();
  NotifyAction acts[2] = {{"Télécharger", url + "/zip?d=" + dir}, {"Commenter", url + "/?lot=" + dir}};
  notify(title, body, prio, tags, acts, 2);
}

void archivePending(const String& dir, const String& lot, bool conforme, bool updated) {
  if (!sdOk()) return;
  std::vector<Pend> v = readPending();
  for (size_t i = 0; i < v.size(); i++) if (v[i].dir == dir) { v.erase(v.begin() + i); break; }
  Pend p;
  p.dir = dir;
  p.lot = lot;
  p.closed = p.reminded = nowEpoch();
  v.push_back(p);
  writePending(v);
  if (!settings.ntfyEnd) return;
  if (updated)
    sendLinks(dir, String("Lot ") + lot + " – relevé mis à jour",
              "Un commentaire a été ajouté. Télécharger la nouvelle sauvegarde (Wi-Fi de l'atelier).", 3, "pencil");
  else
    sendLinks(dir, String("Lot ") + lot + (conforme ? " CONFORME" : " NON CONFORME"),
              "Relevé prêt. Ajouter un commentaire si besoin, puis télécharger la sauvegarde .zip (Wi-Fi de l'atelier).",
              conforme ? 3 : 5, conforme ? "white_check_mark" : "x");
}

void archiveDownloaded(const String& dir) {
  std::vector<Pend> v = readPending();
  for (size_t i = 0; i < v.size(); i++)
    if (v[i].dir == dir) { v.erase(v.begin() + i); writePending(v); return; }
}

bool archiveIsPending(const String& dir) {
  for (auto& p : readPending()) if (p.dir == dir) return true;
  return false;
}

void archiveList(JsonArray out) {
  for (auto& p : readPending()) {
    JsonObject o = out.add<JsonObject>();
    o["dir"] = p.dir;
    o["lot"] = p.lot;
    o["cloture"] = fmtDateTime(p.closed);
  }
}

void archiveLoop() {
  static uint32_t lastCheck = 0;
  if (lastCheck && millis() - lastCheck < 10UL * 60 * 1000) return;
  lastCheck = millis() ? millis() : 1;
  if (!clockValid() || !sdOk()) return;
  uint32_t now = nowEpoch();
  std::vector<Pend> v = readPending();
  bool changed = false;
  for (auto& p : v) {
    if (now - p.reminded < 24UL * 3600) continue;
    p.reminded = now;
    changed = true;
    if (settings.ntfyEnd && settings.ntfyReminder)
      sendLinks(p.dir, String("Lot ") + p.lot + " pas encore sauvegardé",
                String("Clôturé le ") + fmtDateTime(p.closed).substring(0, 16) +
                    ". Télécharger la sauvegarde depuis le Wi-Fi de l'atelier.", 4, "floppy_disk");
  }
  if (changed) writePending(v);
}
