// web_ui.h — Interface web (une seule page, servie par l'ESP32).
#pragma once
#include <Arduino.h>

static const char WEB_UI[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Compteur UP</title>
<style>
:root{--ok:#0a7d34;--bad:#c62828;--warn:#e08600;--bg:#f4f5f7;--card:#fff;--line:#d9dce1;--ink:#16181d;--mute:#666}
*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;background:var(--bg);color:var(--ink);font-size:15px}
header{position:sticky;top:0;z-index:5;background:#1d2733;color:#fff;padding:8px 12px;display:flex;gap:10px;align-items:center;flex-wrap:wrap}
header b{font-size:17px}.pill{padding:2px 9px;border-radius:12px;font-size:13px;background:#455}.pill.session{background:#1565c0}.pill.mesure{background:#6a1b9a}
nav{display:flex;overflow-x:auto;background:#fff;border-bottom:1px solid var(--line)}nav a{padding:10px 13px;color:#333;text-decoration:none;white-space:nowrap;border-bottom:3px solid transparent}
nav a.on{border-color:#1565c0;color:#1565c0;font-weight:600}
main{max-width:900px;margin:0 auto;padding:10px}.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px;margin-bottom:10px}
h2{font-size:16px;margin:0 0 8px}button{font:inherit;border:0;border-radius:8px;padding:10px 14px;background:#e3e7ec;cursor:pointer}
button.pri{background:#1565c0;color:#fff}button.danger{background:var(--bad);color:#fff}button:disabled{opacity:.45}
.big{width:100%;font-size:24px;font-weight:700;padding:20px;background:#9aa4ae;color:#fff;margin:6px 0}.big.go{background:var(--ok);animation:pulse 1s infinite}
@keyframes pulse{50%{filter:brightness(1.25)}}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.grow{flex:1}
.tunnel{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px}.slot{border:2px solid var(--line);border-radius:8px;padding:8px;min-height:120px;position:relative}
.slot h3{margin:0 0 4px;font-size:12px;color:var(--mute);font-weight:600;text-transform:uppercase}.slot .n{font-size:20px;font-weight:700}.slot.w{border-color:#c0392b}
.bar{height:10px;background:#e5e7ea;border-radius:5px;overflow:hidden;margin:4px 0}.bar i{display:block;height:100%;background:var(--warn)}.bar i.done{background:var(--ok)}
.mute{color:var(--mute);font-size:13px}.alarm{background:#fdecea;border:1px solid #f5c2c0;color:#8a1c1c;border-radius:8px;padding:8px;margin-bottom:6px}
.alarm.info{background:#fff6e0;border-color:#f1d9a6;color:#6b4d00}
table{border-collapse:collapse;width:100%}td,th{padding:5px 6px;border-bottom:1px solid var(--line);text-align:left;font-size:14px}
.num{text-align:right}.ok{color:var(--ok);font-weight:600}.nok{color:var(--bad);font-weight:600}
label{display:block;margin:8px 0 2px;font-size:13px;color:#333}input,textarea,select{font:inherit;width:100%;padding:8px;border:1px solid #bbb;border-radius:6px}
input[type=checkbox]{width:auto}.grid2{display:grid;grid-template-columns:1fr 1fr;gap:0 10px}
#toast{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:#222;color:#fff;padding:10px 16px;border-radius:8px;display:none;z-index:9;max-width:90%}
#toast.err{background:var(--bad)}.hide{display:none}
</style></head><body>
<header><b>Compteur UP</b><span id="mode" class="pill">…</span><span id="clock" class="mute" style="color:#cfd8dc"></span><span class="grow"></span><span id="net" class="mute" style="color:#cfd8dc"></span></header>
<nav id="nav"></nav>
<main>
<section id="t-bord">
 <div id="alarms"></div>
 <div class="card" id="runCard">
  <div class="row"><h2 class="grow" id="runTitle">Repos</h2><span id="runInfo" class="mute"></span></div>
  <div class="tunnel" id="tunnel"></div>
  <button class="big" id="bShift" onclick="act('/api/shift')">DÉCALAGE</button>
  <div class="row"><span id="eta" class="grow mute"></span>
   <button id="bUndo" onclick="act('/api/undo',{},'Annuler le dernier décalage ?')">Annuler</button>
   <button id="bEnd" onclick="act('/api/endentries',{},'Plus aucune caisse à entrer ?')">Fin des entrées</button>
   <button id="bRem" onclick="act('/api/removelast',{},'Retirer la dernière caisse entrée (caisse vide, appui de trop) ?')">Retirer la dernière caisse</button>
   <button id="bAck" onclick="act('/api/ack')">Acquitter</button></div>
 </div>
 <div class="card" id="outCard"><h2>Dernières caisses sorties</h2><table id="outs"></table></div>
 <div class="card"><h2>Sondes</h2><table id="probes"></table></div>
 <div class="card"><h2>Journal</h2><div id="log" class="mute"></div></div>
</section>

<section id="t-session" class="hide">
 <div class="card" id="sessStart"><h2>Démarrer un lot</h2>
  <label>N° de lot *</label><input id="sLot">
  <label>Produit</label><input id="sProd">
  <label>Opérateur</label><input id="sOp">
  <div class="grid2"><div><label>T° des bouteilles au départ (°C)</label><input id="sT0" type="number" step="0.1"></div>
  <div><label>Cible (UP au cœur, bain seul)</label><input id="sTarget" type="number" step="0.1"></div></div>
  <p class="mute">Mettre la bouteille témoin dans la <b>1re caisse</b>, entrer la caisse en place 1 puis appuyer sur DÉCALAGE.</p>
  <button class="pri" onclick="startSession()">Démarrer la session</button></div>
 <div class="card hide" id="sessRun"><h2>Clôturer le lot</h2>
  <p id="sessState" class="mute"></p>
  <label>Commentaires / actions correctives (figurent sur le relevé)</label><textarea id="sComment" rows="3"></textarea>
  <div class="row" style="margin-top:8px"><button class="pri" onclick="closeSession(false)">Clôturer</button>
  <button class="danger" onclick="closeSession(true)">Forcer la clôture</button></div></div>
 <div class="card hide" id="lastRep"></div>
</section>

<section id="t-mesure" class="hide">
 <div class="card" id="mesStart"><h2>Mesure d'essai</h2>
  <p class="mute">Le témoin mesure les UP réelles. Appui 1 : témoin en place 1 · 2 : place 2 · 3 : place 3 · 4 : sortie du bain. La mesure continue à l'air jusqu'à l'arrêt (UP bain + air).</p>
  <label>Notes (cadence, réglage du bain…)</label><input id="mNotes">
  <button class="pri" style="margin-top:8px" onclick="act('/api/mesure/start',{notes:$('#mNotes').value})">Démarrer la mesure</button></div>
 <div class="card hide" id="mesRun"><h2>Mesure en cours</h2><table id="mesTable"></table>
  <label>Commentaire</label><input id="mComment">
  <button class="pri" style="margin-top:8px" onclick="act('/api/mesure/stop',{comment:$('#mComment').value},'Arrêter la mesure ?')">Arrêter la mesure</button></div>
 <div class="card hide" id="lastMes"></div>
 <div class="card hide" id="fitCard"></div>
</section>

<section id="t-hist" class="hide"><div class="card"><h2>Historique</h2><button onclick="loadHist()">Actualiser</button><table id="hist" style="margin-top:8px"></table></div></section>

<section id="t-reglages" class="hide">
 <div class="card"><h2>Heure</h2><p class="mute" id="clockTxt"></p>
  <button onclick="setTime()">Régler sur l'heure du téléphone</button></div>
 <div class="card"><h2>Accès administrateur</h2><div class="row"><input id="pass" type="password" placeholder="Mot de passe" class="grow" style="width:auto">
  <button class="pri" onclick="loadSettings()">Ouvrir</button></div></div>
 <div id="setForm" class="hide"></div>
</section>

<section id="t-etal" class="hide">
 <div class="card"><h2>Étalonnage des sondes</h2>
 <p class="mute">Plonger les sondes à vérifier et un thermomètre de référence étalonné ensemble, au plus près, dans l'eau du bain à ~65 °C (zone brassée). Attendre 3 minutes de stabilité, saisir la valeur de référence et cocher les sondes à corriger. Correction = référence − valeur brute (refusée au-delà de ±3 °C).</p>
 <table id="calTable"></table>
 <label>Température de référence (°C)</label><input id="calRef" type="number" step="0.01">
 <div class="row" style="margin-top:8px"><button class="pri" onclick="calib()">Calculer et enregistrer</button><button onclick="calReset()">Remettre à zéro</button></div>
 <p class="mute" id="calDate"></p></div>
</section>
</main>
<div id="toast"></div>
<script>
const $=s=>document.querySelector(s);
const TABS=[['bord','Tableau de bord'],['session','Session'],['mesure','Mesure'],['hist','Historique'],['reglages','Réglages'],['etal','Étalonnage']];
let st={},tab='bord',pass='',setLoaded=null;
function nav(){$('#nav').innerHTML=TABS.map(t=>`<a href="#" class="${t[0]==tab?'on':''}" onclick="go('${t[0]}');return false">${t[1]}</a>`).join('')}
function go(t){tab=t;nav();TABS.forEach(x=>$('#t-'+x[0]).classList.toggle('hide',x[0]!=t));if(t=='hist')loadHist();render()}
function toast(m,err){const e=$('#toast');e.textContent=m;e.className=err?'err':'';e.style.display='block';clearTimeout(e._t);e._t=setTimeout(()=>e.style.display='none',err?6000:1800)}
async function api(p,b){const o={method:b===undefined?'GET':'POST',headers:{'Content-Type':'application/json','X-Pass':pass}};if(b!==undefined)o.body=JSON.stringify(b);
 try{const r=await fetch(p,o);let j={};try{j=await r.json()}catch(e){}if(!r.ok&&!j.error)j.error='Erreur '+r.status;return j}catch(e){return{error:'Appareil injoignable'}}}
async function act(p,b,c){if(c&&!confirm(c))return;const j=await api(p,b||{});if(j.error)toast(j.error,true);else toast('OK');refresh();return j}
const f=(v,d=1)=>v===undefined||v===null||isNaN(v)?'–':Number(v).toFixed(d);
const mmss=s=>s<0||s===undefined?'–':Math.floor(s/60)+':'+String(Math.floor(s%60)).padStart(2,'0');
const esc=s=>String(s??'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
async function refresh(){const j=await api('/api/status');if(j.error){$('#mode').textContent='hors ligne';return}st=j;render()}
function render(){
 if(!st.mode)return;
 $('#mode').textContent=st.mode;$('#mode').className='pill '+st.mode;$('#clock').textContent=st.heure;$('#net').textContent=st.ip+(st.sd?'':' · SD absente');
 $('#alarms').innerHTML=(st.alarms||[]).map(a=>`<div class="alarm ${a.sounding?'':'info'}">⚠ ${esc(a.msg)} <span class="mute">(${a.since})</span></div>`).join('')+(st.silenced?'<div class="mute">Buzzer coupé temporairement</div>':'');
 const r=st.run,ses=st.mode=='session',mes=st.mode=='mesure';
 $('#runTitle').textContent=ses?'Lot '+r.lot:(mes?'Mesure d\'essai':'Repos');
 $('#runInfo').textContent=ses?`cible ${f(r.target)} UP · cadence prévue ${r.eqS>0?mmss(r.eqS):'–'} · ${r.nCrates} caisses · ${r.nNonConformes} NC`:(mes?esc(r.notes):'');
 const W=s=>{const p=st.probes[s];return p.ok?f(p.v,1)+' °C':'–'};
 let tun='';
 for(const s of [2,1,0]){
  const name=s==2?'Place 3 · sortie':(s==0?'Place 1 · entrée':'Place 2');
  let inner='';
  if(ses){const x=r.slots[s];if(x.id){const pc=Math.min(100,100*x.up/r.target);
   inner=`<div class="n">C${x.id}${x.witness?' 🌡':''}</div><div>${f(x.up,2)} / ${f(r.target)} UP</div><div class="bar"><i class="${pc>=100?'done':''}" style="width:${pc}%"></i></div><div class="mute">cœur ${f(x.tc)} °C · ${mmss(x.since)}</div>`}else inner='<div class="mute">vide</div>';}
  else if(mes&&r.pos==s){inner=`<div class="n">Témoin 🌡</div><div>${f(r.upBath,2)} UP</div><div class="mute">${W(0)}</div>`}
  tun+=`<div class="slot ${(ses&&r.slots[s].witness)||(mes&&r.pos==s)?'w':''}"><h3>${name}</h3><div class="mute">eau ${W(s+1)}</div>${inner}</div>`}
 $('#tunnel').innerHTML=tun;
 const b=$('#bShift');b.classList.toggle('go',!!st.ready);b.disabled=!(ses||mes);b.textContent=st.ready?'DÉCALER':'DÉCALAGE';
 $('#eta').textContent=ses?(r.finished?'Toutes les caisses sont sorties : clôturer (onglet Session)':(!r.started?'Témoin dans la caisse 1, puis appuyer':(st.ready?'Décaler maintenant':'Décaler dans ~'+mmss(st.eta)))):(mes?(r.pos>=0?'Décalage nominal dans '+mmss(st.eta):(r.pos==-2?'Appuyer quand le témoin entre en place 1':'Témoin sorti : arrêter la mesure (onglet Mesure)')):'');
 $('#bUndo').classList.toggle('hide',!(r&&r.canUndo));$('#bEnd').classList.toggle('hide',!(ses&&r.started&&!r.entriesEnded));$('#bRem').classList.toggle('hide',!(ses&&r.nCrates>1&&!r.finished));$('#bAck').classList.toggle('hide',!(st.alarms||[]).length&&!st.ready);
 $('#outCard').classList.toggle('hide',!ses);
 if(ses)$('#outs').innerHTML='<tr><th>Caisse</th><th>Sortie</th><th class="num">UP</th><th></th><th>Statut</th></tr>'+(r.exited||[]).map(c=>`<tr><td>C${c.id}</td><td>${c.sortie}</td><td class="num">${f(c.up,2)}</td><td class="mute">${c.measured?'mesurées':'estimées'}</td><td class="${c.conforme?'ok':'nok'}">${c.conforme?'conforme':'NON CONFORME'}</td></tr>`).join('');
 $('#probes').innerHTML=st.probes.map(p=>`<tr><td>${p.name}</td><td class="num ${p.ok?'':'nok'}">${p.ok?f(p.v,2)+' °C':'absente'}</td></tr>`).join('');
 $('#log').innerHTML=(st.log||[]).map(esc).join('<br>');
 // session
 $('#sessStart').classList.toggle('hide',st.mode!='repos');$('#sessRun').classList.toggle('hide',!ses);
 if(st.mode=='repos'&&!$('#sLot').dataset.init){$('#sLot').dataset.init=1;fillDefaults()}
 if(st.mode!='repos')delete $('#sLot').dataset.init;
 if(ses)$('#sessState').textContent=r.finished?'Toutes les caisses sont sorties.':`${r.nOut} caisse(s) sortie(s) sur ${r.nCrates}. Il reste des caisses au bain : « Forcer » les rendra non conformes.`;
 const lc=st.lastClosed||'';
 $('#lastRep').classList.toggle('hide',!lc.startsWith('/sessions'));$('#lastRep').innerHTML=links(lc,'Dernier lot clôturé');
 $('#mesStart').classList.toggle('hide',st.mode!='repos');$('#mesRun').classList.toggle('hide',!mes);
 if(mes)$('#mesTable').innerHTML=`<tr><td>Position du témoin</td><td>${r.pos>=0?'place '+(r.pos+1):(r.pos==-2?'pas encore entré':'sorti du bain')}</td></tr><tr><td>Température témoin</td><td>${W(0)}</td></tr><tr><td>T° à l'entrée</td><td>${f(r.tw0,2)} °C</td></tr><tr><td>Temps place 1 / 2 / 3</td><td>${r.tPos.map(mmss).join(' / ')}</td></tr><tr><td><b>UP bain</b></td><td><b>${f(r.upBath,2)}</b></td></tr><tr><td>UP bain + air</td><td>${f(r.upTotal,2)}</td></tr><tr><td>Prévision du modèle (bain)</td><td>${f(r.upPred,2)}</td></tr><tr><td>Auxiliaire bain / total</td><td>${f(r.upAuxBath,2)} / ${f(r.upAuxTotal,2)}</td></tr>`;
 $('#lastMes').classList.toggle('hide',!lc.startsWith('/mesures'));$('#lastMes').innerHTML=links(lc,'Dernière mesure')+(lc.startsWith('/mesures')?`<button onclick="fit('${lc}')" style="margin-top:6px">Calibrer le modèle sur cette mesure</button>`:'');
 // réglages / étalonnage
 $('#clockTxt').textContent=st.clockOk?'Heure de l\'appareil : '+st.heure:'Heure non réglée : la régler avant toute session.';
 $('#calTable').innerHTML='<tr><th></th><th>Sonde</th><th class="num">Brute</th><th class="num">Correction</th><th class="num">Corrigée</th></tr>'+st.probes.map((p,i)=>`<tr><td><input type="checkbox" class="calsel" value="${i}" ${document.querySelector('.calsel[value="'+i+'"]:checked')?'checked':''}></td><td>${p.name}</td><td class="num">${p.ok?f(p.raw,2):'–'}</td><td class="num">${f(p.offset,2)}</td><td class="num">${p.ok?f(p.v,2):'–'}</td></tr>`).join('');
}
function links(d,title){if(!d)return'';const u=p=>'/f?p='+encodeURIComponent(d+'/'+p);const s=d.startsWith('/sessions');
 return `<h2>${title}</h2><div class="row"><a href="${u('releve.html')}" target="_blank"><button class="pri">Relevé (imprimer / PDF)</button></a><a href="${u('data.csv')}"><button>data.csv</button></a>${s?`<a href="${u('caisses.csv')}"><button>caisses.csv</button></a>`:''}<a href="${u('events.csv')}"><button>events.csv</button></a></div>`}
function fillDefaults(){const d=st.def||{};$('#sProd').value=d.produit||'';$('#sT0').value=d.t0??6;$('#sTarget').value=d.target??''}
async function startSession(){const b={lot:$('#sLot').value.trim(),produit:$('#sProd').value,operateur:$('#sOp').value,t0:parseFloat($('#sT0').value),target:parseFloat($('#sTarget').value)};
 if(!b.lot)return toast('N° de lot obligatoire',true);if(isNaN(b.t0)||isNaN(b.target))return toast('T° de départ et cible obligatoires',true);
 if(!confirm(`Démarrer le lot ${b.lot} : cible ${b.target} UP, bouteilles à ${b.t0} °C ?`))return;
 const j=await act('/api/session/start',b);if(j&&!j.error){$('#sLot').value='';go('bord')}}
async function closeSession(force){const j=await act('/api/session/close',{comment:$('#sComment').value,force},force?'Forcer la clôture ? Les caisses encore au bain seront NON CONFORMES.':'Clôturer le lot ?');if(j&&!j.error)$('#sComment').value=''}
async function loadHist(){const j=await api('/api/history');if(j.error)return toast(j.error,true);const it=(j.items||[]).sort((a,b)=>a.id<b.id?1:-1);
 $('#hist').innerHTML='<tr><th>Date</th><th>Type</th><th>Lot / notes</th><th>Résultat</th><th></th></tr>'+it.map(x=>{const u=p=>'/f?p='+encodeURIComponent(x.dir+'/'+p);const ses=x.type=='session';
 const res=ses?`<span class="${x.statut=='CONFORME'?'ok':(x.statut=='en cours'?'':'nok')}">${esc(x.statut)}</span> ${x.caisses??''} c. · min ${f(x.upMin,1)}`:`${f(x.upBain,2)} UP bain`;
 return `<tr><td>${esc(x.debut||x.id)}</td><td>${ses?'lot':'mesure'}</td><td>${esc(ses?x.lot:x.notes)}</td><td>${res}</td><td><a href="${u('releve.html')}" target="_blank">relevé</a> · <a href="${u('data.csv')}">csv</a>${ses?'':` · <a href="#" onclick="fit('${x.dir}');return false">calibrer</a>`}</td></tr>`}).join('')}
async function fit(dir){toast('Calcul en cours…');const j=await api('/api/fit',{dir});go('mesure');const c=$('#fitCard');c.classList.remove('hide');
 if(j.k1===undefined){c.innerHTML='<h2>Étalonnage du modèle</h2><p class="nok">'+esc(j.error)+'</p>';return}
 c.innerHTML=`<h2>Étalonnage du modèle</h2><table><tr><td>Mesure</td><td>${esc(dir)}</td></tr><tr><td>Points au bain</td><td>${j.n} (pas ${f(j.dt,1)} s)</td></tr><tr><td>k1 / k2 proposés</td><td>${j.k1.toFixed(5)} / ${j.k2.toFixed(5)} /s</td></tr><tr><td>Écart moyen modèle / témoin</td><td>${f(j.rmse,2)} °C</td></tr><tr><td>UP mesurées (bain)</td><td>${f(j.upMeas,2)}</td></tr><tr><td>UP prévues par le nouveau modèle</td><td>${f(j.upPred,2)}</td></tr><tr><td>UP prévues par le modèle actuel</td><td>${f(j.upCurrent,2)}</td></tr></table>${j.error?'<p class="nok">'+esc(j.error)+'</p>':''}
 <p class="mute">Appliquer seulement si l'écart moyen est faible (&lt; 0,5 °C) et les UP prévues proches des mesurées. Mot de passe administrateur requis (onglet Réglages).</p><button class="pri" onclick="applyModel(${j.k1},${j.k2},'${esc(dir.split('/').pop())}')">Appliquer ce modèle</button>`}
async function applyModel(k1,k2,src){if(!pass)return toast('Ouvrir d\'abord l\'accès administrateur (Réglages)',true);await act('/api/model',{k1,k2,source:'étalonné sur la mesure '+src},'Remplacer le modèle actuel ?')}
async function setTime(){await act('/api/time',{epoch:Math.floor(Date.now()/1000)})}
const FIELDS=[['Réseau',[['wifiSsid','Wi-Fi : nom du réseau'],['wifiPass','Wi-Fi : mot de passe','password'],['adminPass','Mot de passe administrateur (4 car. min.)','password']]],
 ['Notifications ntfy',[['ntfyServer','Serveur'],['ntfyTopic','Sujet (long et difficile à deviner)'],['ntfyAlarms','Alarmes','checkbox'],['ntfyShift','Chaque décalage','checkbox'],['ntfyEnd','Fin de lot','checkbox']]],
 ['Calcul des UP',[['target','Cible par défaut (UP)','number'],['upMax','Alerte au-delà de (UP)','number'],['z','z (°C)','number'],['tRef','T° de référence (°C)','number'],['tCount','Seuil de comptage (°C)','number'],['nominalS','Décalage nominal – mode mesure / secours (s)','number'],['rAlarm','Alerte si mesure/modèle <','number']]],
 ['Bain',[['bathMin','Alarme si eau sous (°C)','number'],['bathMinDelayS','pendant plus de (s)','number']]],
 ['Modèle (bouteille virtuelle)',[['k1','k1 (/s)','number'],['k2','k2 (/s)','number'],['modelSource','Origine'],['modelDate','Date']]]];
async function loadSettings(){pass=$('#pass').value;const d=await api('/api/settings');if(d.error)return toast(d.error,true);setLoaded=d;
 $('#setForm').innerHTML=FIELDS.map(g=>`<div class="card"><h2>${g[0]}</h2>`+g[1].map(([k,l,t])=>t=='checkbox'?`<label><input type="checkbox" id="c_${k}" ${d[k]?'checked':''}> ${l}</label>`:`<label>${l}</label><input id="c_${k}" type="${t||'text'}" step="any" value="${esc(d[k])}">`).join('')+'</div>').join('')+
 `<div class="card row"><button class="pri" onclick="saveSettings()">Enregistrer</button><button onclick="act('/api/test/ntfy')">Tester ntfy</button><button onclick="act('/api/test/buzzer')">Tester le buzzer</button><button onclick="act('/api/reboot',{},'Redémarrer l\\'appareil ?')">Redémarrer</button></div>`;
 $('#setForm').classList.remove('hide');$('#calDate').textContent='Dernier étalonnage : '+(d.calDate||'jamais');toast('Réglages chargés')}
async function saveSettings(){const b={};FIELDS.forEach(g=>g[1].forEach(([k,l,t])=>{const e=$('#c_'+k);b[k]=t=='checkbox'?e.checked:(t=='number'?parseFloat(e.value):e.value)}));
 const j=await act('/api/settings',b);if(j&&j.reboot)toast('Wi-Fi modifié : redémarrer l\'appareil',false)}
async function calib(){if(!pass)return toast('Ouvrir d\'abord l\'accès administrateur (Réglages)',true);const sel=[...document.querySelectorAll('.calsel:checked')].map(e=>+e.value);
 const ref=parseFloat($('#calRef').value);if(isNaN(ref)||!sel.length)return toast('Saisir la référence et cocher des sondes',true);await act('/api/calib',{ref,sel},'Enregistrer les corrections ?')}
async function calReset(){if(!pass)return toast('Ouvrir d\'abord l\'accès administrateur (Réglages)',true);await act('/api/calib',{reset:true},'Remettre toutes les corrections à zéro ?')}
nav();refresh();setInterval(refresh,2000);
</script></body></html>)HTML";
