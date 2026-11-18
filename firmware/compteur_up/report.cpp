// report.cpp
#include "report.h"
#include <vector>
#include <SD.h>
#include "config.h"
#include "settings.h"
#include "clock.h"
#include "storage.h"

// ---------------------------------------------------------------------------
// Outils
// ---------------------------------------------------------------------------
static String f(float v, int d = 1) {
  if (std::isnan(v)) return String("–");
  return String(v, d);
}

static String dur(float s) {
  int m = (int)(s / 60), r = (int)s % 60;
  char b[16];
  snprintf(b, sizeof b, "%d:%02d", m, r);
  return String(b);
}

static int splitCsv(const String& line, String* out, int maxf) {
  int n = 0, start = 0;
  for (int i = 0; i <= (int)line.length() && n < maxf; i++) {
    if (i == (int)line.length() || line[i] == ',') {
      out[n++] = line.substring(start, i);
      start = i + 1;
    }
  }
  return n;
}

static float num(const String& s) {
  String t = s;
  t.trim();
  if (!t.length()) return NAN;
  return t.toFloat();
}

// Lecture de data.csv : statistiques et points sous-échantillonnés
struct Scan {
  int rows = 0;
  float mn[5], mx[5], sum[5];
  int cnt[5];
  std::vector<float> t, y[5];   // 0 témoin, 1-3 bain, 4 aux
};

static void scanData(const String& path, Scan& sc, int maxPts = 360) {
  for (int k = 0; k < 5; k++) { sc.mn[k] = 1e9; sc.mx[k] = -1e9; sc.sum[k] = 0; sc.cnt[k] = 0; }
  File fl = SD.open(path, FILE_READ);
  if (!fl) return;
  fl.readStringUntil('\n');   // en-tête
  while (fl.available()) { fl.readStringUntil('\n'); sc.rows++; }
  fl.close();
  int step = sc.rows / maxPts + 1;
  fl = SD.open(path, FILE_READ);
  fl.readStringUntil('\n');
  String fld[8];
  int i = 0;
  while (fl.available()) {
    String line = fl.readStringUntil('\n');
    line.trim();
    if (splitCsv(line, fld, 8) < 7) { i++; continue; }
    float t = num(fld[1]);
    float v[5] = {num(fld[2]), num(fld[3]), num(fld[4]), num(fld[5]), num(fld[6])};
    for (int k = 0; k < 5; k++) {
      if (std::isnan(v[k])) continue;
      sc.mn[k] = fminf(sc.mn[k], v[k]);
      sc.mx[k] = fmaxf(sc.mx[k], v[k]);
      sc.sum[k] += v[k];
      sc.cnt[k]++;
    }
    if (i % step == 0) {
      sc.t.push_back(t / 60.0f);
      for (int k = 0; k < 5; k++) sc.y[k].push_back(v[k]);
    }
    i++;
  }
  fl.close();
}

// Graphique SVG (aucun script : s'imprime tel quel)
static void svgChart(File& o, const Scan& sc, const int* series, int ns, float y0, float y1,
                     const char* title) {
  static const char* COL[5] = {"#c0392b", "#1f77b4", "#2ca02c", "#9467bd", "#7f7f7f"};
  static const char* NAME[5] = {"Témoin", "Bain place 1", "Bain place 2", "Bain place 3", "Aux"};
  const int W = 720, H = 230, L = 46, R = 12, T = 26, B = 30;
  float x1 = sc.t.empty() ? 1 : sc.t.back();
  if (x1 <= 0) x1 = 1;
  auto X = [&](float t) { return L + (W - L - R) * t / x1; };
  auto Y = [&](float v) { return T + (H - T - B) * (1 - (v - y0) / (y1 - y0)); };
  o.printf("<svg viewBox='0 0 %d %d' class='chart' xmlns='http://www.w3.org/2000/svg'>", W, H);
  o.printf("<text x='%d' y='16' class='ct'>%s</text>", L, title);
  float span = y1 - y0, dy = span > 30 ? 10 : (span > 12 ? 5 : (span > 5 ? 2 : 1));
  for (float v = ceilf(y0 / dy) * dy; v <= y1 + 0.001f; v += dy) {
    o.printf("<line x1='%d' x2='%d' y1='%.1f' y2='%.1f' class='g'/>", L, W - R, Y(v), Y(v));
    if (v > y0 + 0.001f) o.printf("<text x='%d' y='%.1f' class='ax' text-anchor='end'>%g</text>", L - 4, Y(v) + 4, v);
  }
  float dx = x1 > 180 ? 60 : (x1 > 60 ? 15 : (x1 > 20 ? 5 : 1));
  for (float t = 0; t <= x1 + 0.001f; t += dx) {
    o.printf("<line x1='%.1f' x2='%.1f' y1='%d' y2='%d' class='g'/>", X(t), X(t), T, H - B);
    o.printf("<text x='%.1f' y='%d' class='ax' text-anchor='middle'>%g</text>", X(t), H - B + 14, t);
  }
  o.printf("<text x='%d' y='%d' class='ax' text-anchor='end'>min</text>", W - R, H - 2);
  for (int s = 0; s < ns; s++) {
    int k = series[s];
    bool open = false;
    for (size_t i = 0; i < sc.t.size(); i++) {
      float v = sc.y[k][i];
      if (std::isnan(v)) { if (open) { o.print("'/>"); open = false; } continue; }
      v = fminf(fmaxf(v, y0), y1);
      if (!open) { o.printf("<polyline fill='none' stroke='%s' stroke-width='1.6' points='", COL[k]); open = true; }
      o.printf("%.1f,%.1f ", X(sc.t[i]), Y(v));
    }
    if (open) o.print("'/>");
    o.printf("<rect x='%d' y='6' width='10' height='10' fill='%s'/><text x='%d' y='15' class='ax'>%s</text>",
             W - R - 110 * (ns - s), COL[k], W - R - 110 * (ns - s) + 14, NAME[k]);
  }
  o.print("</svg>");
}

static const char CSS[] PROGMEM = R"CSS(
<style>
body{font-family:Arial,Helvetica,sans-serif;font-size:12px;color:#111;max-width:800px;margin:16px auto;padding:0 12px}
h1{font-size:19px;margin:0 0 4px}h2{font-size:14px;margin:18px 0 6px;border-bottom:1px solid #999;padding-bottom:2px}
table{border-collapse:collapse;width:100%}td,th{border:1px solid #bbb;padding:3px 5px;text-align:left;vertical-align:top}
th{background:#eee}.kv td:first-child{width:38%;color:#444}.num{text-align:right}
.ok{color:#0a6b2d;font-weight:bold}.nok{color:#b00020;font-weight:bold}
.verdict{font-size:22px;padding:8px 12px;border:3px solid;display:inline-block;margin:8px 0}
.verdict.ok{border-color:#0a6b2d}.verdict.nok{border-color:#b00020}
.chart{width:100%;height:auto;margin:6px 0}.chart .g{stroke:#e3e3e3}.chart .ax{font-size:10px;fill:#555}.chart .ct{font-size:12px;font-weight:bold}
.sign td{height:42px}.small{font-size:10px;color:#555}
@media print{body{margin:0}h2{break-after:avoid}tr{break-inside:avoid}.noprint{display:none}}
</style>)CSS";

static void head(File& o, const String& title) {
  o.print("<!DOCTYPE html><html lang='fr'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>");
  o.print(htmlEsc(title));
  o.print("</title>");
  o.print(FPSTR(CSS));
  o.print("</head><body><p class='noprint'><button onclick='window.print()'>Imprimer / PDF</button></p>");
}

static void kv(File& o, const char* k, const String& v) {
  o.print("<tr><td>"); o.print(k); o.print("</td><td>"); o.print(v); o.print("</td></tr>");
}

static void eventsTable(File& o, const String& dir) {
  o.print("<h2>Journal des événements</h2><table><tr><th>Heure</th><th>Type</th><th>Caisse</th><th>Message</th></tr>");
  File fl = SD.open(dir + "/events.csv", FILE_READ);
  if (fl) {
    fl.readStringUntil('\n');
    while (fl.available()) {
      String line = fl.readStringUntil('\n');
      line.trim();
      int a = line.indexOf(','), b = line.indexOf(',', a + 1), c = line.indexOf(',', b + 1);
      if (a < 0 || b < 0 || c < 0) continue;
      String msg = line.substring(c + 1);
      if (msg.startsWith("\"") && msg.endsWith("\"")) { msg = msg.substring(1, msg.length() - 1); msg.replace("\"\"", "\""); }
      o.print("<tr><td>"); o.print(line.substring(0, a).substring(11));
      o.print("</td><td>"); o.print(htmlEsc(line.substring(a + 1, b)));
      o.print("</td><td>"); o.print(line.substring(b + 1, c));
      o.print("</td><td>"); o.print(htmlEsc(msg)); o.print("</td></tr>");
    }
    fl.close();
  }
  o.print("</table>");
}

static void modelRows(File& o) {
  kv(o, "Méthode UP", String("1 UP = 1 min à ") + f(settings.tRef, 1) + " °C, z = " + f(settings.z, 2) +
                         " °C, comptage au-dessus de " + f(settings.tCount, 1) + " °C");
  kv(o, "Modèle (bouteille virtuelle)", String("k1 = ") + String(settings.k1, 5) + " /s, k2 = " +
                                            String(settings.k2, 5) + " /s — " + htmlEsc(settings.modelSource) +
                                            (settings.modelDate[0] ? String(" (") + settings.modelDate + ")" : String("")));
  String off;
  for (int i = 0; i < NPROBE; i++) off += (i ? " / " : "") + f(settings.offset[i], 2);
  kv(o, "Étalonnage des sondes", String(settings.calDate[0] ? settings.calDate : "non renseigné") +
                                     " — corrections (témoin / bain 1 / 2 / 3 / aux) : " + off + " °C");
}

// ---------------------------------------------------------------------------
// CSV des caisses
// ---------------------------------------------------------------------------
bool writeCratesCsv(const RunMeta& m, const up::Session& s) {
  File o = SD.open(String(m.dir) + "/caisses.csv", FILE_WRITE);
  if (!o) return false;
  o.print("caisse,entree,sortie,duree_place1_s,duree_place2_s,duree_place3_s,eau_min_C,temoin,"
          "up_mesurees,up_modele,correction,up_retenues,cible,statut,remarque\r\n");
  for (int i = 0; i < s.nCrates; i++) {
    const up::Crate& c = s.crates[i];
    String rem;
    if (c.pos != -1) rem = "encore au bain à la clôture";
    else if (c.bathLost) rem = "aucune sonde de bain valide";
    else if (c.early) rem = "sortie avant la cible";
    if (c.witnessFault) rem += (rem.length() ? " ; " : "") + String("témoin défaillant");
    if (c.overMax) rem += (rem.length() ? " ; " : "") + String("au-dessus du maximum");
    o.printf("%d,%s,%s,%.0f,%.0f,%.0f,%s,%s,%s,%.3f,%.3f,%s,%.2f,%s,%s\r\n", c.id,
             fmtDateTime(c.tIn).c_str(), c.tOut ? fmtDateTime(c.tOut).c_str() : "", c.tPos[0], c.tPos[1],
             c.tPos[2], c.minBath < 500 ? String(c.minBath, 2).c_str() : "", c.witness ? "oui" : "non",
             c.witness ? String(c.upMeas, 3).c_str() : "", c.upModel, c.rcAtExit,
             c.pos == -1 ? String(c.upFinal, 3).c_str() : "", s.p.target,
             c.pos == -1 && c.conforme ? "CONFORME" : "NON CONFORME", csvField(rem).c_str());
  }
  o.close();
  return true;
}

// ---------------------------------------------------------------------------
// Relevé de session
// ---------------------------------------------------------------------------
bool writeSessionReport(const RunMeta& m, const up::Session& s, const String& comment) {
  String dir = m.dir;
  Scan sc;
  scanData(dir + "/data.csv", sc);
  File o = SD.open(dir + "/releve.html", FILE_WRITE);
  if (!o) return false;
  bool conf = s.lotConforme();
  uint32_t tEnd = nowEpoch();
  head(o, String("Relevé de pasteurisation – lot ") + m.lot);
  o.print("<h1>Relevé de pasteurisation — lot "); o.print(htmlEsc(m.lot)); o.print("</h1>");
  o.print("<div class='verdict "); o.print(conf ? "ok'>LOT CONFORME" : "nok'>LOT NON CONFORME"); o.print("</div>");

  // motifs de non-conformité
  std::vector<String> why;
  int nc = 0, nout = 0;
  float mn = 1e9, mx = -1, mnInt = 1e9, mxInt = 0, sumInt = 0;
  int nInt = 0;
  for (int i = 0; i < s.nCrates; i++) {
    const up::Crate& c = s.crates[i];
    if (i > 0) {
      float d = (float)(c.tIn - s.crates[i - 1].tIn);
      mnInt = fminf(mnInt, d); mxInt = fmaxf(mxInt, d); sumInt += d; nInt++;
    }
    if (c.pos != -1) { why.push_back(String("Caisse ") + c.id + " encore au bain à la clôture"); nc++; continue; }
    nout++;
    mn = fminf(mn, c.upFinal); mx = fmaxf(mx, c.upFinal);
    if (!c.conforme) {
      nc++;
      why.push_back(String("Caisse ") + c.id + (c.bathLost ? " : aucune sonde de bain valide" :
                    String(" : ") + f(c.upFinal, 2) + " UP < cible " + f(s.p.target, 1)));
    }
  }
  if (s.witnessFaulted) why.push_back("Sonde témoin défaillante : contrôle réel absent pour ce lot");
  if (s.nCrates == 0) why.push_back("Aucune caisse traitée");
  if (!why.empty()) {
    o.print("<ul>");
    for (auto& w : why) { o.print("<li class='nok'>"); o.print(htmlEsc(w)); o.print("</li>"); }
    o.print("</ul>");
  }

  o.print("<h2>Lot</h2><table class='kv'>");
  kv(o, "N° de lot", htmlEsc(m.lot));
  kv(o, "Produit", htmlEsc(m.produit));
  kv(o, "Opérateur", htmlEsc(m.operateur));
  kv(o, "Début / fin", fmtDateTime(m.tStart) + " → " + fmtDateTime(tEnd) + " (" + dur((float)(tEnd - m.tStart)) + ")");
  kv(o, "Caisses traitées", String(s.nCrates) + " (" + nout + " sorties, " + nc + " non conformes)");
  kv(o, "Température de départ des bouteilles (saisie)", f(m.t0, 1) + " °C");
  kv(o, "Cible", f(s.p.target, 1) + " UP au cœur, bain seul (alerte au-delà de " + f(s.p.upMax, 0) + " UP)");
  kv(o, "UP retenues min / max", nout ? f(mn, 2) + " / " + f(mx, 2) : String("–"));
  if (nInt) kv(o, "Intervalle entre décalages min / moyen / max", dur(mnInt) + " / " + dur(sumInt / nInt) + " / " + dur(mxInt));
  o.print("</table>");

  o.print("<h2>Contrôle par la bouteille témoin</h2><table class='kv'>");
  if (s.nCrates > 0) {
    const up::Crate& c = s.crates[0];
    kv(o, "Caisse témoin", "caisse 1");
    kv(o, "UP mesurées au cœur (bain)", c.witnessFault ? String("<span class='nok'>sonde défaillante</span>") : f(c.upMeas, 2));
    kv(o, "UP prévues par le modèle", f(c.upModel, 2));
    kv(o, "Rapport mesure / modèle", s.rKnown ? f(s.r, 3) : String("–"));
    kv(o, "Correction appliquée aux caisses suivantes", s.rKnown && s.r < 1
         ? String("UP estimées × ") + f(s.rc(), 3) + " (modèle optimiste)"
         : String("aucune (modèle prudent ou exact)"));
  }
  modelRows(o);
  kv(o, "Appareil", String("compteur-up v") + FW_VERSION);
  o.print("</table>");

  o.print("<h2>Températures de l'eau</h2><table><tr><th>Sonde</th><th class='num'>Min</th><th class='num'>Moyenne</th><th class='num'>Max</th></tr>");
  static const char* NM[4] = {"Témoin", "Eau place 1 (entrée)", "Eau place 2", "Eau place 3 (sortie)"};
  for (int j = 1; j <= 3; j++) {
    o.print("<tr><td>"); o.print(NM[j]); o.print("</td>");
    if (sc.cnt[j]) o.printf("<td class='num'>%.2f</td><td class='num'>%.2f</td><td class='num'>%.2f</td></tr>",
                            sc.mn[j], sc.sum[j] / sc.cnt[j], sc.mx[j]);
    else o.print("<td colspan='3'>–</td></tr>");
  }
  o.print("</table>");
  {
    float lo = 1e9, hi = -1e9;
    for (int k = 1; k <= 3; k++) if (sc.cnt[k]) { lo = fminf(lo, sc.mn[k]); hi = fmaxf(hi, sc.mx[k]); }
    if (hi > lo - 1) {
      lo = floorf(lo - 0.5f); hi = ceilf(hi + 0.5f);
      int ser[3] = {1, 2, 3};
      svgChart(o, sc, ser, 3, lo, hi, "Eau du bain (°C)");
    }
    int ser2[4] = {0, 1, 2, 3};
    svgChart(o, sc, ser2, 4, 0, 80, "Témoin et bain (°C)");
  }

  o.print("<h2>Caisses</h2><table><tr><th>N°</th><th>Entrée</th><th>Sortie</th><th class='num'>Place 1</th>"
          "<th class='num'>Place 2</th><th class='num'>Place 3</th><th class='num'>Eau min</th>"
          "<th class='num'>UP</th><th>Origine</th><th>Statut</th></tr>");
  for (int i = 0; i < s.nCrates; i++) {
    const up::Crate& c = s.crates[i];
    o.printf("<tr><td>%d</td><td>%s</td><td>%s</td><td class='num'>%s</td><td class='num'>%s</td><td class='num'>%s</td><td class='num'>%s</td>",
             c.id, fmtTime(c.tIn).c_str(), c.tOut ? fmtTime(c.tOut).c_str() : "–", dur(c.tPos[0]).c_str(),
             dur(c.tPos[1]).c_str(), dur(c.tPos[2]).c_str(), c.minBath < 500 ? f(c.minBath, 1).c_str() : "–");
    bool ok = c.pos == -1 && c.conforme;
    o.printf("<td class='num'>%s</td><td>%s</td><td class='%s'>%s%s</td></tr>",
             c.pos == -1 ? f(c.upFinal, 2).c_str() : "–",
             c.measured ? "mesurées (témoin)" : (c.rcAtExit < 0.999f ? (String("estimées × ") + f(c.rcAtExit, 2)).c_str() : "estimées"),
             ok ? "ok" : "nok", ok ? "CONFORME" : "NON CONFORME", c.overMax ? " (≥ max)" : "");
  }
  o.print("</table><p class='small'>UP estimées : bouteille virtuelle calculée avec la température de l'eau à la place de la caisse. "
          "UP mesurées : sonde au cœur de la bouteille témoin.</p>");

  eventsTable(o, dir);

  o.print("<h2>Commentaires et actions correctives</h2><p>");
  o.print(comment.length() ? htmlEsc(comment) : String("Aucun."));
  o.print("</p><h2>Visa</h2><table class='sign'><tr><th>Vérifié par</th><th>Date</th><th>Signature</th></tr><tr><td></td><td></td><td></td></tr></table>");
  o.printf("<p class='small'>Relevé généré le %s par compteur-up v%s. Données brutes : data.csv, caisses.csv, events.csv (dossier %s).</p>",
           fmtDateTime(tEnd).c_str(), FW_VERSION, htmlEsc(dir).c_str());
  o.print("</body></html>");
  o.close();
  return true;
}

// ---------------------------------------------------------------------------
// Relevé de mesure (essai)
// ---------------------------------------------------------------------------
bool writeMesureReport(const RunMeta& m, const up::Measure& me, const String& comment) {
  String dir = m.dir;
  Scan sc;
  scanData(dir + "/data.csv", sc);
  File o = SD.open(dir + "/releve.html", FILE_WRITE);
  if (!o) return false;
  uint32_t tEnd = nowEpoch();
  head(o, String("Mesure ") + m.id);
  o.print("<h1>Mesure d'UP — essai "); o.print(htmlEsc(m.id)); o.print("</h1><table class='kv'>");
  kv(o, "Notes", htmlEsc(m.notes));
  kv(o, "Début / fin", fmtDateTime(m.tStart) + " → " + fmtDateTime(tEnd));
  kv(o, "Témoin au bain", me.tIn ? fmtTime(me.tIn) + " → " + (me.tOut ? fmtTime(me.tOut) : String("non sorti")) : String("jamais entré"));
  kv(o, "Temps par place (1 / 2 / 3)", dur(me.tPos[0]) + " / " + dur(me.tPos[1]) + " / " + dur(me.tPos[2]));
  kv(o, "Température du témoin à l'entrée", f(me.t0, 2) + " °C");
  kv(o, "<b>UP au cœur – bain seul</b>", "<b>" + f(me.upBath, 2) + "</b>");
  kv(o, "UP au cœur – bain + air", f(me.upTotal, 2));
  kv(o, "UP prévues par le modèle actuel (bain)", f(me.upPred, 2));
  if (sc.cnt[4]) kv(o, "Sonde auxiliaire : UP bain / bain + air", f(me.upAuxBath, 2) + " / " + f(me.upAuxTotal, 2));
  modelRows(o);
  kv(o, "Appareil", String("compteur-up v") + FW_VERSION);
  o.print("</table>");
  int ser[5] = {0, 1, 2, 3, 4};
  svgChart(o, sc, ser, sc.cnt[4] ? 5 : 4, 0, 80, "Témoin et bain (°C)");
  eventsTable(o, dir);
  o.print("<h2>Commentaires</h2><p>");
  o.print(comment.length() ? htmlEsc(comment) : String("Aucun."));
  o.printf("</p><p class='small'>Généré le %s par compteur-up v%s. Données brutes : data.csv.</p></body></html>",
           fmtDateTime(tEnd).c_str(), FW_VERSION);
  o.close();
  return true;
}

// ---------------------------------------------------------------------------
// Étalonnage du modèle sur une mesure
// ---------------------------------------------------------------------------
String fitFromDir(const String& dir, JsonObject out) {
  if (dir.indexOf("..") >= 0 || !dir.startsWith("/mesures/")) return "Dossier invalide";
  File fl = SD.open(dir + "/data.csv", FILE_READ);
  if (!fl) return "Fichier data.csv introuvable";
  fl.readStringUntil('\n');
  std::vector<float> tw, tm;
  tw.reserve(1500); tm.reserve(1500);
  String fld[9];
  float t0 = NAN, t1 = NAN, lastW[3] = {NAN, NAN, NAN}, lastM = NAN;
  while (fl.available() && tm.size() < 6000) {
    String line = fl.readStringUntil('\n');
    line.trim();
    if (splitCsv(line, fld, 9) < 8) continue;
    int place = fld[7].toInt();
    for (int k = 0; k < 3; k++) { float v = num(fld[3 + k]); if (!std::isnan(v)) lastW[k] = v; }
    float m = num(fld[2]);
    if (!std::isnan(m)) lastM = m;
    if (place < 1 || place > 3) continue;
    float w = lastW[place - 1];
    if (std::isnan(w)) {   // sonde de la place absente : la plus froide des autres
      for (int k = 0; k < 3; k++) if (!std::isnan(lastW[k]) && (std::isnan(w) || lastW[k] < w)) w = lastW[k];
    }
    if (std::isnan(w) || std::isnan(lastM)) continue;
    float t = num(fld[1]);
    if (std::isnan(t0)) t0 = t;
    t1 = t;
    tw.push_back(w);
    tm.push_back(lastM);
  }
  fl.close();
  if (tm.size() < 60) return "Pas assez de points avec le témoin au bain (faire au moins un passage complet)";
  float dt = (t1 - t0) / (tm.size() - 1);
  up::Params p = settings.params();
  up::FitResult r = up::fitModel(tw.data(), tm.data(), (int)tm.size(), dt, p);
  float upCur = 0;
  up::simulateSSE(tw.data(), tm.data(), (int)tm.size(), dt, settings.k1, settings.k2, &upCur, p);
  out["ok"] = r.ok;
  out["k1"] = r.k1;
  out["k2"] = r.k2;
  out["rmse"] = r.rmse;
  out["upMeas"] = r.upMeas;
  out["upPred"] = r.upPred;
  out["upCurrent"] = upCur;
  out["n"] = r.n;
  out["dt"] = dt;
  out["dir"] = dir;
  if (!r.ok) return "Ajustement imprécis (écart moyen > 2 °C) : vérifier la mesure";
  return "";
}
