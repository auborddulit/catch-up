// notify.cpp — Notifications téléphone via ntfy (https://ntfy.sh ou serveur perso).
// Envoi dans une tâche séparée pour ne jamais bloquer les mesures.
#include "notify.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "settings.h"

// Autorités racines Let's Encrypt (ISRG Root X1 et X2), utilisées par ntfy.sh.
static const char LE_ROOTS[] PROGMEM =
"-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
"CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
"R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
"MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
"ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
"EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
"+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
"ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
"AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
"zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
"tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
"/q4AaOeMSQ+2b1tbFfLn\n"
"-----END CERTIFICATE-----\n";

struct Msg {
  char title[64];
  char body[200];
  uint8_t prio;    // 1 (min) à 5 (urgent)
  char tags[32];
  uint8_t nAct;
  char actLabel[2][24];
  char actUrl[2][160];
};

static QueueHandle_t q;
static volatile int lastCode = 0;

static void sendOne(const Msg& m) {
  if (WiFi.status() != WL_CONNECTED || !settings.ntfyTopic[0]) return;
  // Publication JSON (accents OK) : POST sur la racine du serveur.
  String url = String(settings.ntfyServer);
  if (!url.endsWith("/")) url += "/";
  JsonDocument d;
  d["topic"] = settings.ntfyTopic;
  d["title"] = m.title;
  d["message"] = m.body;
  d["priority"] = m.prio;
  if (m.tags[0]) d["tags"].add(m.tags);
  for (int i = 0; i < m.nAct; i++) {
    JsonObject a = d["actions"].add<JsonObject>();
    a["action"] = "view";
    a["label"] = m.actLabel[i];
    a["url"] = m.actUrl[i];
  }
  if (m.nAct) d["click"] = m.actUrl[0];
  String payload;
  serializeJson(d, payload);
  HTTPClient http;
  WiFiClientSecure sec;
  WiFiClient plain;
  bool ok;
  if (url.startsWith("https://")) {
    sec.setCACert(LE_ROOTS);
    ok = http.begin(sec, url);
  } else {
    ok = http.begin(plain, url);
  }
  if (!ok) { lastCode = -1; return; }
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  lastCode = http.POST(payload);
  http.end();
}

static void task(void*) {
  Msg m;
  for (;;) {
    if (xQueueReceive(q, &m, portMAX_DELAY) == pdTRUE) sendOne(m);
  }
}

void notifyBegin() {
  q = xQueueCreate(12, sizeof(Msg));
  xTaskCreatePinnedToCore(task, "ntfy", 12288, nullptr, 1, nullptr, 0);
}

void notify(const String& title, const String& body, uint8_t prio, const char* tags,
            const NotifyAction* actions, int nActions) {
  if (!q || !settings.ntfyTopic[0]) return;
  Msg m;
  strlcpy(m.title, title.c_str(), sizeof m.title);
  strlcpy(m.body, body.c_str(), sizeof m.body);
  m.prio = prio;
  strlcpy(m.tags, tags ? tags : "", sizeof m.tags);
  m.nAct = 0;
  for (int i = 0; i < nActions && i < 2; i++) {
    strlcpy(m.actLabel[i], actions[i].label, sizeof m.actLabel[i]);
    strlcpy(m.actUrl[i], actions[i].url.c_str(), sizeof m.actUrl[i]);
    m.nAct++;
  }
  xQueueSend(q, &m, 0);   // file pleine : message perdu plutôt que bloquer
}

int notifyLastCode() { return lastCode; }
