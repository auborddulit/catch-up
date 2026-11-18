// up_core.cpp — voir up_core.h
#include "up_core.h"

namespace up {

float effectiveBath(const Inputs& in, int pos, bool* fallback) {
  if (fallback) *fallback = false;
  if (pos >= 0 && pos < NPOS && in.bathOk[pos]) return in.bath[pos];
  float m = NAN;
  for (int i = 0; i < NPOS; i++)
    if (in.bathOk[i] && (std::isnan(m) || in.bath[i] < m)) m = in.bath[i];
  if (fallback && !std::isnan(m)) *fallback = true;
  return m;
}

// ===========================================================================
// Session
// ===========================================================================
void Session::begin(const Params& prm, float t0_, uint32_t now) {
  p = prm;
  t0 = t0_;
  tStart = now;
  lastShift = now;
  started = false;
  entriesEnded = false;
  rKnown = false;
  r = 1;
  witnessFaulted = false;
  nCrates = 0;
  for (int i = 0; i < NPOS; i++) slot[i] = -1;
  u = Undo();
  ev.clear();
}

int Session::newCrate(uint32_t now) {
  if (nCrates >= MAX_CRATES) return -1;
  int i = nCrates++;
  Crate& c = crates[i];
  c = Crate();
  c.id = (int16_t)(i + 1);
  c.pos = 0;
  c.witness = (i == 0);  // le témoin est toujours dans la 1re caisse
  c.tIn = now;
  c.b.init(t0);
  ev.push(Ev::CrateIn, c.id);
  return i;
}

float Session::targetFor(const Crate& c) const {
  if (c.witness && !c.witnessFault) return p.target;
  // UP estimées × rc >= cible  <=>  UP estimées >= cible / rc
  return p.target / rc();
}

float Session::upNow(const Crate& c) const {
  if (c.witness && !c.witnessFault) return c.upMeas;
  return c.upModel * rc();
}

void Session::step(uint32_t now, float dt, const Inputs& in) {
  if (dt <= 0) return;
  for (int s = 0; s < NPOS; s++) {
    int i = slot[s];
    if (i < 0) continue;
    Crate& c = crates[i];
    bool fb = false;
    float tw = effectiveBath(in, s, &fb);
    c.tPos[s] += dt;
    if (std::isnan(tw)) {
      // aucune sonde de bain valide : on gèle la bouteille virtuelle (aucune
      // UP comptée) et la caisse sera non conforme.
      if (!c.bathLost) ev.push(Ev::BathLost, c.id);
      c.bathLost = true;
    } else {
      if (tw < c.minBath) c.minBath = tw;
      float tcOld = c.b.tc;
      c.b.step(tw, dt, p.k1, p.k2);
      c.upModel += 0.5f * (puPerSecond(tcOld, p) + puPerSecond(c.b.tc, p)) * dt;
    }
    if (c.witness && !c.witnessFault) {
      if (in.witnessOk) {
        c.upMeas += puPerSecond(in.witness, p) * dt;
      } else {
        c.witnessBadS += dt;  // échantillon perdu : on ne compte rien (prudent)
        if (c.witnessBadS >= p.witnessFaultS) {
          c.witnessFault = true;
          witnessFaulted = true;
          ev.push(Ev::WitnessFault, c.id);
        }
      }
    }
    if (!c.overMaxLive && upNow(c) > p.upMax) {
      c.overMaxLive = true;
      ev.push(Ev::OverMax, c.id, upNow(c));
    }
  }
}

void Session::finalize(Crate& c, uint32_t now) {
  c.pos = -1;
  c.tOut = now;
  if (c.witness && !c.witnessFault) {
    c.measured = true;
    c.upFinal = c.upMeas;
    c.rcAtExit = 1;
    // comparaison témoin / modèle pour la suite du lot
    if (c.upModel > 0.05f) {
      r = c.upMeas / c.upModel;
      rKnown = true;
      ev.push(Ev::WitnessResult, c.id, c.upMeas, c.upModel);
      if (r < p.rAlarm) ev.push(Ev::RatioLow, c.id, r);
    }
  } else {
    c.measured = false;
    c.rcAtExit = rc();
    c.upFinal = c.upModel * c.rcAtExit;
  }
  c.early = c.upFinal < p.target;
  c.overMax = c.upFinal > p.upMax;
  c.conforme = !c.early && !c.bathLost;
  ev.push(Ev::CrateOut, c.id, c.upFinal, c.conforme ? 1.0f : 0.0f);
  if (c.early) ev.push(Ev::EarlyExit, c.id, c.upFinal);
}

int Session::shift(uint32_t now) {
  u.valid = true;
  u.t = now;
  u.lastShift = lastShift;
  u.started = started;
  for (int s = 0; s < NPOS; s++) u.slot[s] = slot[s];
  u.exited = -1;
  u.added = -1;

  int out = -1;
  if (slot[NPOS - 1] >= 0) {
    Crate& c = crates[slot[NPOS - 1]];
    finalize(c, now);
    out = c.id;
    u.exited = slot[NPOS - 1];
  }
  for (int s = NPOS - 1; s > 0; s--) {
    slot[s] = slot[s - 1];
    if (slot[s] >= 0) crates[slot[s]].pos = (int8_t)s;
  }
  slot[0] = entriesEnded ? -1 : newCrate(now);
  u.added = slot[0];
  started = true;
  lastShift = now;
  if (finished()) ev.push(Ev::Done);
  return out;
}

bool Session::undo(uint32_t now, uint32_t maxAgeS) {
  if (!u.valid || now - u.t > maxAgeS) return false;
  if (u.added >= 0 && u.added == nCrates - 1) nCrates--;  // retire la caisse ajoutée
  for (int s = 0; s < NPOS; s++) {
    slot[s] = u.slot[s];
    if (slot[s] >= 0) crates[slot[s]].pos = (int8_t)s;
  }
  if (u.exited >= 0) {
    Crate& c = crates[u.exited];
    c.pos = NPOS - 1;
    c.tOut = 0;
    c.upFinal = 0;
    c.early = c.overMax = c.conforme = c.measured = false;
    if (c.witness) { rKnown = false; r = 1; }
  }
  lastShift = u.lastShift;
  started = u.started;
  u.valid = false;
  ev.push(Ev::Undo);
  return true;
}

void Session::endEntries() {
  if (entriesEnded) return;
  entriesEnded = true;
  ev.push(Ev::EntriesEnded);
  // si la caisse en place 1 vient d'entrer par erreur, l'opérateur utilise « annuler »
  if (finished()) ev.push(Ev::Done);
}

bool Session::removeLast() {
  if (nCrates == 0) return false;
  int i = nCrates - 1;
  Crate& c = crates[i];
  if (c.pos < 0 || c.witness) return false;  // déjà sortie, ou caisse témoin
  slot[c.pos] = -1;
  nCrates--;
  u.valid = false;
  ev.push(Ev::Removed, i + 1);
  if (finished()) ev.push(Ev::Done);
  return true;
}

bool Session::finished() const {
  if (!entriesEnded) return false;
  for (int s = 0; s < NPOS; s++) if (slot[s] >= 0) return false;
  return true;
}

bool Session::ready(uint32_t now) const {
  if (!started) return false;
  int i = slot[NPOS - 1];
  if (i < 0) {
    // place de sortie vide (remplissage du tunnel) : décalage nominal
    bool any = false;
    for (int s = 0; s < NPOS; s++) if (slot[s] >= 0) any = true;
    return any && (float)(now - lastShift) >= p.nominalS;
  }
  const Crate& c = crates[i];
  return upNow(c) >= p.target;
}

float Session::etaSeconds(uint32_t now, const Inputs& in) const {
  if (!started) return -1;
  int i = slot[NPOS - 1];
  if (i < 0) {
    float e = p.nominalS - (float)(now - lastShift);
    return e < 0 ? 0 : e;
  }
  const Crate& c = crates[i];
  float need = p.target - upNow(c);
  if (need <= 0) return 0;
  float tw = effectiveBath(in, NPOS - 1);
  if (std::isnan(tw)) return -1;
  // projection : eau constante à sa valeur actuelle
  Bottle b = c.b;
  float gain = 0, factor = (c.witness && !c.witnessFault) ? 1.0f : rc();
  const float h = 5.0f;
  for (float t = 0; t < 3600; t += h) {
    float tcOld = b.tc;
    b.step(tw, h, p.k1, p.k2);
    gain += 0.5f * (puPerSecond(tcOld, p) + puPerSecond(b.tc, p)) * h * factor;
    if (gain >= need) return t + h;
  }
  return -1;
}

bool Session::lotConforme() const {
  if (witnessFaulted) return false;  // contrôle réel absent
  if (nCrates == 0) return false;
  for (int i = 0; i < nCrates; i++) {
    const Crate& c = crates[i];
    if (c.pos != -1) return false;  // caisse encore dans le bain
    if (!c.conforme) return false;
  }
  return true;
}

// ===========================================================================
// Mesure
// ===========================================================================
void Measure::begin(const Params& prm, uint32_t now) {
  p = prm;
  tStart = now;
  tIn = tOut = lastPress = 0;
  pos = -2;
  upBath = upTotal = upAuxBath = upAuxTotal = upPred = 0;
  t0 = NAN;
  for (int i = 0; i < NPOS; i++) tPos[i] = 0;
  ev.clear();
}

void Measure::press(uint32_t now) {
  lastPress = now;
  if (pos == -2) { pos = 0; tIn = now; t0 = NAN; }
  else if (pos >= 0 && pos < NPOS - 1) pos++;
  else if (pos == NPOS - 1) { pos = -1; tOut = now; }
  ev.push(Ev::MeasureMove, pos);
}

bool Measure::undo(uint32_t now, uint32_t maxAgeS) {
  if (lastPress == 0 || now - lastPress > maxAgeS) return false;
  if (pos == 0) { pos = -2; tIn = 0; }
  else if (pos > 0) pos--;
  else if (pos == -1) { pos = NPOS - 1; tOut = 0; }
  lastPress = 0;
  ev.push(Ev::Undo);
  return true;
}

void Measure::step(uint32_t now, float dt, const Inputs& in) {
  (void)now;
  if (dt <= 0) return;
  bool inBath = pos >= 0 && pos < NPOS;
  bool after = pos == -1;
  if (inBath && std::isnan(t0) && in.witnessOk) { t0 = in.witness; b.init(t0); }
  if (in.witnessOk && (inBath || after)) {
    float r = puPerSecond(in.witness, p) * dt;
    upTotal += r;
    if (inBath) upBath += r;
  }
  if (in.auxOk && (inBath || after)) {
    float r = puPerSecond(in.aux, p) * dt;
    upAuxTotal += r;
    if (inBath) upAuxBath += r;
  }
  if (inBath) {
    tPos[pos] += dt;
    float tw = effectiveBath(in, pos);
    if (!std::isnan(tw) && !std::isnan(t0)) {
      float tcOld = b.tc;
      b.step(tw, dt, p.k1, p.k2);
      upPred += 0.5f * (puPerSecond(tcOld, p) + puPerSecond(b.tc, p)) * dt;
    }
  }
}

// ===========================================================================
// Ajustement du modèle
// ===========================================================================
float simulateSSE(const float* tw, const float* tm, int n, float dt,
                  float k1, float k2, float* upPred, const Params& p) {
  Bottle b;
  b.init(tm[0]);
  float sse = 0, upv = 0;
  float a1 = expf(-k1 * dt), a2 = expf(-k2 * dt);
  for (int i = 1; i < n; i++) {
    float w = tw[i - 1];
    float tsOld = b.ts, tcOld = b.tc;
    b.ts = w + (b.ts - w) * a1;
    float tsAvg = 0.5f * (tsOld + b.ts);
    b.tc = tsAvg + (b.tc - tsAvg) * a2;
    float e = b.tc - tm[i];
    // on pèse davantage la zone qui produit des UP
    float wgt = tm[i] > p.tCount - 5 ? 4.0f : 1.0f;
    sse += wgt * e * e;
    upv += 0.5f * (puPerSecond(tcOld, p) + puPerSecond(b.tc, p)) * dt;
  }
  if (upPred) *upPred = upv;
  return sse;
}

FitResult fitModel(const float* tw, const float* tm, int n, float dt, const Params& p) {
  FitResult r;
  r.n = n;
  if (n < 30 || dt <= 0) return r;
  // grille logarithmique, k1 >= k2 (le modèle est symétrique en k1/k2)
  const int G = 32;
  const float lo = logf(1e-4f), hi = logf(0.1f);
  float best = INFINITY, bk1 = 0, bk2 = 0;
  for (int i = 0; i < G; i++) {
    float k1 = expf(lo + (hi - lo) * i / (G - 1));
    for (int j = 0; j <= i; j++) {
      float k2 = expf(lo + (hi - lo) * j / (G - 1));
      float s = simulateSSE(tw, tm, n, dt, k1, k2, nullptr, p);
      if (s < best) { best = s; bk1 = k1; bk2 = k2; }
    }
  }
  // affinage local (3 passes de grille resserrée)
  float span = (hi - lo) / (G - 1);
  for (int pass = 0; pass < 3; pass++) {
    float c1 = logf(bk1), c2 = logf(bk2);
    for (int i = -4; i <= 4; i++)
      for (int j = -4; j <= 4; j++) {
        float k1 = expf(c1 + span * i / 4), k2 = expf(c2 + span * j / 4);
        if (k2 > k1) continue;
        float s = simulateSSE(tw, tm, n, dt, k1, k2, nullptr, p);
        if (s < best) { best = s; bk1 = k1; bk2 = k2; }
      }
    span /= 4;
  }
  r.k1 = bk1;
  r.k2 = bk2;
  float upPred = 0;
  simulateSSE(tw, tm, n, dt, bk1, bk2, &upPred, p);
  // RMSE non pondéré
  {
    Bottle b; b.init(tm[0]);
    float a1 = expf(-bk1 * dt), a2 = expf(-bk2 * dt), s = 0, um = 0;
    for (int i = 1; i < n; i++) {
      float w = tw[i - 1], tsOld = b.ts;
      b.ts = w + (b.ts - w) * a1;
      float tsAvg = 0.5f * (tsOld + b.ts);
      b.tc = tsAvg + (b.tc - tsAvg) * a2;
      float e = b.tc - tm[i];
      s += e * e;
      um += 0.5f * (puPerSecond(tm[i - 1], p) + puPerSecond(tm[i], p)) * dt;
    }
    r.rmse = sqrtf(s / (n - 1));
    r.upMeas = um;
  }
  r.upPred = upPred;
  r.ok = r.rmse < 2.0f;
  return r;
}

}  // namespace up
