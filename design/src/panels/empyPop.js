import { definePanel } from '../app/registry.js';
import { icon } from '../core/icons.js';
import { NMS, NM_LIST } from './empyPop.data.js';

// EmpyPop — calque d'AioHUD (modules/empypop.lua + boxes/empypop_box.lua, data Xurion).
// Pour le NM choisi : titre (vert "— READY!" si tout obtenu) + UNE sous-box par pre-requis
// global (fond vert si obtenu) montrant l'item (vert=obtenu / rouge=manquant) + "from <NM>"
// + la chaine de sous-pops indentee (les paliers). + ligne collectable (compteur).
// NM choisi par DOUBLE-CLIC sur la box en Mode edition.

// etat "obtenu" pseudo-aleatoire mais STABLE par item (demo) -> montre les sous-box vertes/rouges
const owned = (s) => { let h = 0; for (let k = 0; k < s.length; k++) h = (h * 31 + s.charCodeAt(k)) >>> 0; return h % 3 === 0; };
// un pre-requis est COMPLET quand son item ET toute sa chaine de sous-pops sont obtenus
const allOwned = (p) => owned(p.name) && p.sub.every(allOwned);

// UNE ligne par item : [fleche si sous-pop] Item(vert/rouge) [KI]  NM (POSITION en blanc).
// La fleche montre que ce drop sert a pop l'item du dessus. Hierarchie par indentation.
function popHtml(p, depth) {
  const cls = owned(p.name) ? 'have' : 'need';
  const arrow = depth > 0 ? `<span class="ep-arrow">${icon('subarrow', 12)}</span>` : '';
  const ki = p.ki ? '<span class="ep-ki">KI</span>' : '';
  let from = '';
  if (p.from) {
    const m = p.from.match(/\s*\(([^)]+)\)\s*$/);          // extrait la position (H-8) en fin
    const nm = m ? p.from.slice(0, m.index).trim() : p.from;
    from = `<span class="ep-from">${nm}</span>${m ? `<span class="ep-pos">${m[1]}</span>` : ''}`;
  }
  return `<div class="ep-line ${cls}" style="padding-left:${depth * 11}px">${arrow}<span class="ep-name">${p.name}</span>${ki}${from}</div>`
    + p.sub.map((s) => popHtml(s, depth + 1)).join('');
}

function render(cfg) {
  const nm = NMS[cfg.nm] || NMS.itzpapalotl;
  const allDone = nm.pops.length > 0 && nm.pops.every(allOwned);
  const title = `<div class="ep-title${allDone ? ' done' : ''}">${nm.name}${allDone ? ' — READY!' : ''}</div>`;
  const groups = nm.pops.map((p) => `<div class="ep-box${allOwned(p) ? ' ok' : ''}">${popHtml(p, 0)}</div>`).join('');
  const coll = nm.coll ? `<div class="ep-coll">${nm.coll.name}: <b>0</b>/${nm.coll.count}</div>` : '';
  return title + groups + coll;
}

definePanel({
  id: 'empypop',
  title: 'EmpyPop',
  type: 'EmpyPop',
  css: new URL('./empyPop.css', import.meta.url).href,
  pos: { x: 14.5, y: 4 }, // pas de w -> auto-largeur
  // double-clic sur la box en Mode edition -> choisir le NM track (27 NM)
  config: [{ key: 'nm', label: 'NM', type: 'select', default: 'itzpapalotl', options: NM_LIST.map((n) => ({ value: n.key, label: n.label })) }],
  render: (cfg = {}) => render(cfg),
});
