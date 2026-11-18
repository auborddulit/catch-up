// up_core.h — Cœur de calcul du compteur d'UP (C++ pur, sans dépendance Arduino).
//
// Contient :
//   - le calcul des unités de pasteurisation (UP) ;
//   - la « bouteille virtuelle » (modèle thermique à 2 nœuds) ;
//   - le moteur de session (suivi des caisses dans le tunnel à 3 places) ;
//   - le moteur de mesure (essais avec la bouteille témoin) ;
//   - l'ajustement du modèle sur une courbe mesurée.
//
// Ce fichier se compile aussi sur PC (voir tests/) pour vérifier les calculs.
//
// Licence MIT — voir LICENSE.
#pragma once
#include <cstdint>
#include <cmath>

namespace up {

constexpr int NPOS = 3;            // places dans le tunnel : 0 = entrée, 2 = sortie
constexpr int MAX_CRATES = 300;    // caisses max par session
constexpr int EVQ = 32;            // taille de la file d'événements

// ---------------------------------------------------------------------------
// Paramètres
// ---------------------------------------------------------------------------
struct Params {
  float z        = 6.94f;   // °C
  float tRef     = 60.0f;   // °C, 1 UP = 1 min à tRef
  float tCount   = 60.0f;   // °C, les UP ne s'accumulent qu'au-dessus de ce seuil
  float target   = 15.0f;   // UP cible (bain seul)
  float upMax    = 60.0f;   // UP au-delà desquelles on alerte (goût de cuit)
  float k1       = 0.0060f; // 1/s, échange eau -> paroi/périphérie
  float k2       = 0.0060f; // 1/s, périphérie -> cœur
  float nominalS = 400.0f;  // s, décalage nominal quand la place de sortie est vide
  float rAlarm   = 0.85f;   // alerte si UP mesurées / UP modèle < rAlarm
  float witnessFaultS = 30; // s cumulées de sonde témoin invalide avant défaut
};

// UP accumulées par seconde à la température T.
inline float puPerSecond(float T, const Params& p) {
  if (T < p.tCount) return 0.0f;
  return powf(10.0f, (T - p.tRef) / p.z) / 60.0f;
}

// ---------------------------------------------------------------------------
// Bouteille virtuelle : eau du bain -> nœud périphérique (ts) -> cœur (tc)
// Mise à jour exacte pour une température d'eau constante sur le pas dt.
// ---------------------------------------------------------------------------
struct Bottle {
  float ts = 0, tc = 0;
  void init(float t0) { ts = tc = t0; }
  void step(float tw, float dt, float k1, float k2) {
    // sous-pas pour rester précis si dt est grand (ex. après un blocage)
    int n = (int)ceilf(dt / 5.0f);
    if (n < 1) n = 1;
    float h = dt / n;
    float a1 = expf(-k1 * h), a2 = expf(-k2 * h);
    for (int i = 0; i < n; i++) {
      float tsOld = ts;
      ts = tw + (ts - tw) * a1;
      float tsAvg = 0.5f * (tsOld + ts);
      tc = tsAvg + (tc - tsAvg) * a2;
    }
  }
};

// ---------------------------------------------------------------------------
// Entrées d'un pas de calcul
// ---------------------------------------------------------------------------
struct Inputs {
  float bath[NPOS] = {NAN, NAN, NAN};
  bool  bathOk[NPOS] = {false, false, false};
  float witness = NAN;
  bool  witnessOk = false;
  float aux = NAN;
  bool  auxOk = false;
};

// Température d'eau retenue pour une place : la sonde de la place si elle est
// valide, sinon la plus froide des sondes valides (prudent). NAN si aucune.
float effectiveBath(const Inputs& in, int pos, bool* fallback = nullptr);

// ---------------------------------------------------------------------------
// Événements produits par les moteurs (consommés par le firmware : journal,
// buzzer, notifications).
// ---------------------------------------------------------------------------
enum class Ev : uint8_t {
  CrateIn, CrateOut, EarlyExit, OverMax, WitnessResult, RatioLow,
  WitnessFault, BathFallback, BathLost, Undo, EntriesEnded, Done, Removed,
  MeasureMove
};

struct Event {
  Ev type;
  int16_t crate;   // n° de caisse (ou place pour BathFallback)
  float value;     // UP, ratio, etc.
  float value2;
};

class EventQueue {
 public:
  void push(Ev t, int c = -1, float v = 0, float v2 = 0) {
    if (n_ >= EVQ) return;  // file pleine : on perd l'événement le plus récent
    q_[(h_ + n_) % EVQ] = Event{t, (int16_t)c, v, v2};
    n_++;
  }
  bool pop(Event& e) {
    if (!n_) return false;
    e = q_[h_]; h_ = (h_ + 1) % EVQ; n_--;
    return true;
  }
  void clear() { h_ = n_ = 0; }
 private:
  Event q_[EVQ];
  int h_ = 0, n_ = 0;
};

// ---------------------------------------------------------------------------
// Caisse
// ---------------------------------------------------------------------------
struct Crate {
  int16_t id = 0;          // n° de caisse (1, 2, ...)
  int8_t  pos = -2;        // -2 : pas encore entrée, 0..2 : place, -1 : sortie
  bool    witness = false; // contient la bouteille témoin
  bool    witnessFault = false;
  bool    bathLost = false;   // aucune sonde de bain valide pendant sa présence
  bool    overMaxLive = false;
  uint32_t tIn = 0, tOut = 0; // horodatage (s epoch)
  float   tPos[NPOS] = {0, 0, 0};  // secondes passées à chaque place
  float   minBath = 999;     // eau la plus froide vue
  Bottle  b;
  float   upModel = 0;       // UP estimées par la bouteille virtuelle
  float   upMeas = 0;        // UP mesurées (caisse témoin)
  float   witnessBadS = 0;
  // résultat à la sortie
  float   upFinal = 0;       // UP retenues (mesurées, ou estimées × correction)
  float   rcAtExit = 1;      // correction appliquée
  bool    measured = false;  // upFinal vient du témoin
  bool    early = false;     // sortie avant d'atteindre la cible
  bool    overMax = false;
  bool    conforme = false;
};

// ---------------------------------------------------------------------------
// Moteur de session
// ---------------------------------------------------------------------------
class Session {
 public:
  void begin(const Params& p, float t0, uint32_t now);
  // Intègre dt secondes avec les mesures « in ».
  void step(uint32_t now, float dt, const Inputs& in);
  // Appui sur DÉCALAGE. Renvoie le n° de la caisse sortie, ou -1.
  int  shift(uint32_t now);
  // Annule le dernier décalage s'il date de moins de maxAgeS secondes.
  bool undo(uint32_t now, uint32_t maxAgeS = 60);
  void endEntries();
  // Retire la dernière caisse entrée (appui de trop en fin de lot : caisse vide).
  bool removeLast();

  bool  ready(uint32_t now) const;          // faut-il décaler maintenant ?
  float etaSeconds(uint32_t now, const Inputs& in) const;  // -1 si inconnu/>1 h
  float upNow(const Crate& c) const;         // UP retenues en cours (pour affichage)
  float targetFor(const Crate& c) const;     // cible appliquée à cette caisse
  float rc() const { return rKnown ? fminf(1.0f, r) : 1.0f; }
  // Cadence d'équilibre : durée T telle qu'une bouteille partant de t0 et
  // passant T à chaque place (eau = moyenne lissée) reçoive la cible.
  // Renvoie -1 si la cible est inatteignable en 1 h par place.
  float equilibriumInterval(const float tw[NPOS]) const;
  // Décalage attendu quand la place de sortie est vide (remplissage du tunnel)
  float fillInterval() const { return eqS > 0 ? eqS : p.nominalS; }
  bool  finished() const;
  bool  lotConforme() const;

  Params p;
  float  t0 = 6;
  uint32_t tStart = 0, lastShift = 0;
  bool   started = false;      // premier appui fait
  bool   entriesEnded = false;
  bool   rKnown = false;
  float  r = 1;
  bool   witnessFaulted = false;
  float  bathAvg[NPOS] = {NAN, NAN, NAN};  // eau lissée (constante de temps 5 min)
  float  eqS = -1;                         // cadence d'équilibre courante (s)
  uint32_t eqAt = 0;
  int    nCrates = 0;
  int    slot[NPOS] = {-1, -1, -1};   // index dans crates[], -1 = vide
  Crate  crates[MAX_CRATES];
  EventQueue ev;

  // annulation
  struct Undo {
    bool valid = false;
    uint32_t t = 0, lastShift = 0;
    int slot[NPOS];
    int exited = -1, added = -1;
    bool started = false;
  } u;

 private:
  void finalize(Crate& c, uint32_t now);
  int  newCrate(uint32_t now);
};

// ---------------------------------------------------------------------------
// Moteur de mesure (essais) : le témoin avance d'une place à chaque appui.
// Appui 1 : entre en place 1 ; 2 : place 2 ; 3 : place 3 ; 4 : sort du bain.
// ---------------------------------------------------------------------------
class Measure {
 public:
  void begin(const Params& p, uint32_t now);
  void step(uint32_t now, float dt, const Inputs& in);
  void press(uint32_t now);
  bool undo(uint32_t now, uint32_t maxAgeS = 60);

  Params p;
  uint32_t tStart = 0, tIn = 0, tOut = 0, lastPress = 0;
  int8_t pos = -2;              // -2 avant entrée, 0..2 dans le bain, -1 sorti
  float upBath = 0, upTotal = 0;          // témoin
  float upAuxBath = 0, upAuxTotal = 0;    // sonde auxiliaire
  float upPred = 0;                       // prévision du modèle (bain)
  float t0 = NAN;                          // température du témoin à l'entrée
  float tPos[NPOS] = {0, 0, 0};
  Bottle b;
  EventQueue ev;
};

// ---------------------------------------------------------------------------
// Ajustement du modèle sur une courbe mesurée
// tw[i] : eau autour du témoin ; tm[i] : cœur mesuré ; pas constant dt.
// ---------------------------------------------------------------------------
struct FitResult {
  bool  ok = false;
  float k1 = 0, k2 = 0;
  float rmse = 0;        // °C
  float upMeas = 0, upPred = 0;
  int   n = 0;
};

float simulateSSE(const float* tw, const float* tm, int n, float dt,
                  float k1, float k2, float* upPred, const Params& p);
FitResult fitModel(const float* tw, const float* tm, int n, float dt, const Params& p);

}  // namespace up
