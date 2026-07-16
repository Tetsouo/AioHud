/**
 * Deplacement des box en Mode edition :
 *   - magnetisme (snap) sur les box voisines
 *   - anti-chevauchement entre box ET avec les zones interdites (avec exception `allow`)
 *   - persistance auto des positions (panels.json + localStorage)
 */
import { clamp, round1, overlaps, snap, snapGrid, snapCenterX, rectOf } from '../core/geometry.js';
import { loadLocal, saveLocal, putJSON } from '../core/store.js';
import { STORAGE, PANELS_URL, SNAP_PX } from '../core/config.js';
import { zoneRects } from './zonesSystem.js';

let panels = [];
let drag = null; // { p, dx, dy, lx, ly, others }

const GRID = 20;            // pas de la grille (px)
const CENTER_PX = 7;        // tolerance de snap au centre horizontal
const GRID_KEY = 'aiohud.grid.v1';
let gridOn = !!loadLocal(GRID_KEY);
let gridEl = null, guideEl = null; // overlay grille + guide de centrage

export function isGridOn() { return gridOn; }
export function toggleGrid() {
  gridOn = !gridOn;
  saveLocal(GRID_KEY, gridOn);
  document.body.classList.toggle('grid', gridOn);
  return gridOn;
}

const editing = () => document.body.classList.contains('editing');
// box qui forment un JOINT entre elles (gap 10px + barre) : party + les 2 alliances. Les autres snappent flush.
const JOINT = ['party', 'alliance1', 'alliance2'];

/* ---- collisions -------------------------------------------------------- */
// Aire de chevauchement (px^2) avec les zones interdites (hors zones qui autorisent cette box).
// Sert de "cliquet" : un deplacement n'est permis que s'il ne FAIT PAS croitre cette aire.
// -> une box deja dans une zone peut en SORTIR (aire decroit), mais aucune box ne peut y RENTRER.
function zonePenalty(x, y, w, h, id) {
  let pen = 0;
  for (const z of zoneRects()) {
    if (id && z.allow && z.allow.indexOf(id) >= 0) continue;
    const ix = Math.max(0, Math.min(x + w, z.right) - Math.max(x, z.left));
    const iy = Math.max(0, Math.min(y + h, z.bottom) - Math.max(y, z.top));
    pen += ix * iy;
  }
  return pen;
}
// autre box (rects figes au debut du drag) : chevauchement INTERDIT (bloc dur)
function hitsPanel(x, y, w, h) {
  if (!drag) return false;
  return drag.others.some((o) => overlaps(x, y, w, h, o));
}

/* ---- helpers ----------------------------------------------------------- */
function pin(p) {
  const r = p.getBoundingClientRect();
  p.style.left = Math.round(r.left) + 'px';
  p.style.top = Math.round(r.top) + 'px';
  p.style.right = 'auto';
  p.style.bottom = 'auto';
  p.style.transform = 'none';
}
function otherRects(self) {
  return panels.filter((p) => p !== self && p.offsetParent !== null).map(rectOf);
}

/* Ancre la box par le(s) BORD(S) reellement SNAPPE(S) — a une autre box, une zone interdite, ou le bord
   d'ecran — en basculant le positionnement CSS sur ce bord. Effet : quand le contenu change de taille
   (changement de pet, item plus court...), la box grandit/retrecit a l'OPPOSE du bord snappe, donc elle
   RESTE collee a ce qui l'a snappee (ex. EmpyPop colle au-dessus d'alliance 2 -> retrecit par le haut,
   reste colle). A defaut de bord snappe sur un axe, on retombe sur le quadrant ecran. */
function anchorPanel(p) {
  const r = p.getBoundingClientRect();
  if (!r.width) return;
  const TOL = 12; // un peu > le gap de snap box-a-box (10px)
  const ovX = (t) => Math.min(r.right, t.right) - Math.max(r.left, t.left) > 4;
  const ovY = (t) => Math.min(r.bottom, t.bottom) - Math.max(r.top, t.top) > 4;
  const targets = [...otherRects(p), ...zoneRects().filter((z) => !(p.id && z.allow.indexOf(p.id) >= 0))];
  let aR = Math.abs(r.right - innerWidth) <= TOL, aL = r.left <= TOL;     // bords d'ecran
  let aB = Math.abs(r.bottom - innerHeight) <= TOL, aT = r.top <= TOL;
  for (const t of targets) {                                              // bords de box/zones
    if (ovX(t) && Math.abs(r.bottom - t.top) <= TOL) aB = true;          // mon bas colle a un haut
    if (ovX(t) && Math.abs(r.top - t.bottom) <= TOL) aT = true;          // mon haut colle a un bas
    if (ovY(t) && Math.abs(r.right - t.left) <= TOL) aR = true;          // ma droite colle a une gauche
    if (ovY(t) && Math.abs(r.left - t.right) <= TOL) aL = true;          // ma gauche colle a une droite
  }
  const right = aR && !aL ? true : aL && !aR ? false : (r.left + r.width / 2 > innerWidth / 2);
  // growDown -> toujours ancree par le HAUT (le haut reste fixe, le bas change), quelle que soit sa position.
  const bottom = p.dataset.growDown ? false : (aB && !aT ? true : aT && !aB ? false : (r.top + r.height / 2 > innerHeight / 2));
  if (right) { p.style.right = round1((innerWidth - r.right) / innerWidth * 100) + '%'; p.style.left = 'auto'; }
  else { p.style.left = round1(r.left / innerWidth * 100) + '%'; p.style.right = 'auto'; }
  if (bottom) { p.style.bottom = round1((innerHeight - r.bottom) / innerHeight * 100) + '%'; p.style.top = 'auto'; }
  else { p.style.top = round1(r.top / innerHeight * 100) + '%'; p.style.bottom = 'auto'; }
  p.style.transform = 'none';
}

/* ---- persistance ------------------------------------------------------- */
const pctL = (r) => round1(r.left / innerWidth * 100) + '%';
const pctT = (r) => round1(r.top / innerHeight * 100) + '%';
// Capture l'ANCRAGE REEL de la box (le bord par lequel elle est positionnee : left/right + top/bottom,
// avec sa valeur %). C'est ce que anchorPanel a pose -> on le restitue tel quel pour que la box reste
// EXACTEMENT au meme endroit ET grandisse du meme cote (donc ne deborde jamais sur sa voisine).
function anchorOf(p) {
  const s = p.style, r = p.getBoundingClientRect(), o = {};
  if (s.right && s.right !== 'auto') o.r = s.right;
  else o.l = (s.left && s.left !== 'auto') ? s.left : pctL(r);
  if (s.bottom && s.bottom !== 'auto') o.b = s.bottom;
  else o.t = (s.top && s.top !== 'auto') ? s.top : pctT(r);
  return o;
}
function snapshot() {
  const out = {};
  for (const p of panels) {
    const moved = p.dataset.moved || (p.style.left && p.style.left !== 'auto') || (p.style.right && p.style.right !== 'auto');
    if (moved) out[p.id] = anchorOf(p);
  }
  return out;
}
function persist() {
  const data = snapshot();
  saveLocal(STORAGE.panels, data);
  putJSON(PANELS_URL, {
    _README: 'Positions des box HUD (%). Auto-genere depuis la page (Mode edition).',
    panels: data,
  });
}
function applyPos(map) {
  if (!map) return;
  for (const p of panels) {
    let q = map[p.id];
    if (!q) continue;
    // retro-compat ancien format {x,y} (left%/top% en nombres)
    if ('x' in q || 'y' in q) q = { l: q.x + '%', t: q.y + '%' };
    if ('r' in q) { p.style.right = q.r; p.style.left = 'auto'; }
    else if ('l' in q) { p.style.left = q.l; p.style.right = 'auto'; }
    if ('b' in q) { p.style.bottom = q.b; p.style.top = 'auto'; }
    else if ('t' in q) { p.style.top = q.t; p.style.bottom = 'auto'; }
    p.style.transform = 'none';
    p.dataset.moved = '1';
  }
}
async function restore() {
  const saved = loadLocal(STORAGE.panels);
  if (saved) { applyPos(saved); return; }
  try {
    const res = await fetch(PANELS_URL, { cache: 'no-store' });
    if (res.ok) applyPos((await res.json()).panels);
  } catch { /* fichier absent : positions par defaut */ }
}

/* ---- drag -------------------------------------------------------------- */
function onDown(e) {
  if (!editing()) return;
  const p = e.target.closest('.panel');
  if (!p) return;
  e.preventDefault();
  pin(p);
  p.classList.add('dragging');
  const r = p.getBoundingClientRect();
  // lx/ly = derniere position appliquee ; fx/fy = derniere position SANS recouvrement de box
  // cibles de snap des zones : les zones INTERDITES a cette box (on peut se coller a leur bord, flush)
  const zoneTargets = zoneRects().filter((z) => !(p.id && z.allow.indexOf(p.id) >= 0));
  // Le GAP de 10px (+ joint) ne concerne QUE party<->alliances entre elles. Les autres box
  // (target, focus, hate...) snappent NORMALEMENT, bord a bord (gap 0).
  const selfJoint = JOINT.indexOf(p.id) >= 0;
  const boxTargets = panels.filter((q) => q !== p && q.offsetParent !== null)
    .map((q) => ({ ...rectOf(q), gap: (selfJoint && JOINT.indexOf(q.id) >= 0) ? 10 : 0 }));
  drag = { p, dx: e.clientX - r.left, dy: e.clientY - r.top, lx: r.left, ly: r.top, fx: r.left, fy: r.top, others: boxTargets, zones: zoneTargets };
}
function onMove(e) {
  if (!drag) return;
  const w = drag.p.offsetWidth, h = drag.p.offsetHeight, id = drag.p.id;
  let x = clamp(e.clientX - drag.dx, 0, innerWidth - w);
  let y = clamp(e.clientY - drag.dy, 0, innerHeight - h);

  // magnetisme : grille (si activee) SINON box voisines + bords des zones interdites
  const s = gridOn ? snapGrid(x, y, GRID, SNAP_PX) : snap(x, y, w, h, [...drag.others, ...drag.zones], SNAP_PX);
  x = clamp(s.x, 0, innerWidth - w);
  y = clamp(s.y, 0, innerHeight - h);
  // centrage horizontal sur l'interface (toujours dispo) + guide visuel quand ca accroche
  const c = snapCenterX(x, w, innerWidth / 2, CENTER_PX);
  x = clamp(c.x, 0, innerWidth - w);
  if (guideEl) guideEl.style.display = c.snapped ? 'block' : 'none';

  // zones interdites : cliquet par aire (sortir = OK, rentrer/s'enfoncer = refuse).
  // Les AUTRES box ne sont PAS testees ici -> on les traverse librement pendant le drag.
  const refPen = zonePenalty(drag.lx, drag.ly, w, h, id);
  const okZone = (nx, ny) => zonePenalty(nx, ny, w, h, id) <= refPen + 0.5;
  if (!okZone(x, y)) {
    if (okZone(x, drag.ly)) y = drag.ly;
    else if (okZone(drag.lx, y)) x = drag.lx;
    else { x = drag.lx; y = drag.ly; }
  }

  drag.lx = x; drag.ly = y; drag.p.dataset.moved = '1';
  if (!hitsPanel(x, y, w, h)) { drag.fx = x; drag.fy = y; } // memorise la derniere position libre
  drag.p.style.left = Math.round(x) + 'px';
  drag.p.style.top = Math.round(y) + 'px';
  drag.p._chip.innerHTML = chipHtml(drag.p, x, y);
  placeChip(drag.p);
  updateSnapSeparators(); // separateur live pendant le drag
}

// contenu de la pastille flottante : nom + coords
function chipHtml(p, x, y) {
  return `${p.dataset.label}  ${Math.round(x)},${Math.round(y)}`;
}

// Pose la pastille (coords) ET le badge ⚙ dans le coin de la box OPPOSE au bord d'ecran
// le plus proche (box collee a droite -> coin gauche ; box en haut -> coin bas) -> jamais coupes.
function placeChip(p) {
  const r = p.getBoundingClientRect();
  const toLeft = (r.left + r.width / 2) > window.innerWidth / 2; // box plutot a droite -> coin gauche
  const toTop = (r.top + r.height / 2) > window.innerHeight / 2; // box plutot en bas -> coin haut
  const corner = (elm, off) => {
    if (!elm) return;
    elm.style.left = toLeft ? off + 'px' : 'auto';
    elm.style.right = toLeft ? 'auto' : off + 'px';
    elm.style.top = toTop ? '-18px' : 'auto';
    elm.style.bottom = toTop ? 'auto' : '-18px';
  };
  corner(p._gear, 0);                       // l'engrenage tout au coin
  corner(p._chip, p._gear ? 28 : 0);        // les coords decalees a cote de l'engrenage
}

// Separateur de SNAP : si une box est posee juste SOUS une autre (bords colles + chevauchement
// horizontal), on marque la box du dessous .snap-top -> une jolie ligne lumineuse a la jointure (CSS).
function updateSnapSeparators() {
  for (const p of panels) p.classList.remove('snap-top', 'snap-bot');
  const rects = panels.map((p) => p.getBoundingClientRect());
  for (let i = 0; i < panels.length; i++) {
    for (let j = 0; j < panels.length; j++) {
      if (i === j) continue;
      // le JOINT n'existe qu'entre party/alliances -> on n'evalue l'adjacence que pour ces box-la
      if (JOINT.indexOf(panels[i].id) < 0 || JOINT.indexOf(panels[j].id) < 0) continue;
      const a = rects[i], b = rects[j];
      const gap = b.top - a.bottom;                                 // ecart vertical (B sous A) ; ~10px = collee avec le joint
      const adjacent = gap >= -2 && gap <= 14;
      const overlapX = Math.min(a.right, b.right) - Math.max(a.left, b.left);
      if (adjacent && overlapX > Math.min(a.width, b.width) * 0.5) {
        panels[j].classList.add('snap-top'); // B est collee SOUS A
        panels[i].classList.add('snap-bot'); // A a une box collee dessous
      }
    }
  }
}

function onUp() {
  if (!drag) return;
  const p = drag.p, w = p.offsetWidth, h = p.offsetHeight;
  // refus du depot sur une autre box -> retour a la derniere position libre
  if (hitsPanel(drag.lx, drag.ly, w, h)) {
    p.style.left = Math.round(drag.fx) + 'px';
    p.style.top = Math.round(drag.fy) + 'px';
    p._chip.innerHTML = chipHtml(p, drag.fx, drag.fy);
  }
  anchorPanel(p);   // ancre par le coin le plus proche -> grandit a l'oppose au prochain changement de taille
  placeChip(p);
  p.classList.remove('dragging');
  drag = null;
  if (guideEl) guideEl.style.display = 'none';
  updateSnapSeparators();
  persist();
}

/* ---- API publique ------------------------------------------------------ */
export function initDragSystem() {
  panels = [...document.querySelectorAll('.panel')];
  // overlay grille + guide de centrage horizontal (visibilite geree en CSS via body.grid / inline)
  const scene = document.querySelector('.scene') || document.body;
  gridEl = document.createElement('div'); gridEl.id = 'gridOverlay'; scene.appendChild(gridEl);
  guideEl = document.createElement('div'); guideEl.id = 'centerGuide'; scene.appendChild(guideEl);
  if (gridOn) document.body.classList.add('grid');
  for (const p of panels) {
    const chip = document.createElement('div');
    chip.className = 'poschip';
    p.appendChild(chip);
    p._chip = chip;
    // badge ⚙ flottant (overlay) sur les box configurables -> ne touche jamais la box.
    // C'est un SVG : engrenage dessine au CENTRE du viewBox -> centrage garanti (pas de calage pixel).
    if (p.dataset.config) {
      const gear = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      gear.setAttribute('class', 'cfg-gear');
      gear.setAttribute('viewBox', '0 0 24 24');
      gear.innerHTML = '<path fill="#2a1d04" transform="translate(5 5) scale(.875)" d="M9.405 1.05c-.413-1.4-2.397-1.4-2.81 0l-.1.34a1.464 1.464 0 0 1-2.105.872l-.31-.17c-1.283-.698-2.686.705-1.987 1.987l.169.311c.446.82.023 1.841-.872 2.105l-.34.1c-1.4.413-1.4 2.397 0 2.81l.34.1a1.464 1.464 0 0 1 .872 2.105l-.17.31c-.698 1.283.705 2.686 1.987 1.987l.311-.169a1.464 1.464 0 0 1 2.105.872l.1.34c.413 1.4 2.397 1.4 2.81 0l.1-.34a1.464 1.464 0 0 1 2.105-.872l.31.17c1.283.698 2.686-.705 1.987-1.987l-.169-.311a1.464 1.464 0 0 1 .872-2.105l.34-.1c1.4-.413 1.4-2.397 0-2.81l-.34-.1a1.464 1.464 0 0 1-.872-2.105l.17-.31c.698-1.283-.705-2.686-1.987-1.987l-.311.169a1.464 1.464 0 0 1-2.105-.872l-.1-.34zM8 10.93a2.929 2.929 0 1 1 0-5.86 2.929 2.929 0 0 1 0 5.858z"/>';
      p.appendChild(gear);
      p._gear = gear;
    }
  }
  restore();
  // Le CSS des box est injecte en <link> async -> au 1er passage les hauteurs peuvent etre fausses.
  // Apres stabilisation de la mise en page : on re-detecte les joints ET on (re)ancre chaque box par
  // son coin (pour qu'elle grandisse a l'oppose). L'ancrage est fait sur 'load' + fallback (CSS charge).
  updateSnapSeparators();
  requestAnimationFrame(() => requestAnimationFrame(updateSnapSeparators));
  const settle = () => { for (const p of panels) anchorPanel(p); updateSnapSeparators(); };
  window.addEventListener('load', settle);
  setTimeout(settle, 450);
  document.addEventListener('mousedown', onDown);
  document.addEventListener('mousemove', onMove);
  document.addEventListener('mouseup', onUp);
}

/** Rafraichit les pastilles de coordonnees (appele a l'entree en Mode edition). */
export function refreshChips() {
  for (const p of panels) {
    const r = p.getBoundingClientRect();
    p._chip.innerHTML = chipHtml(p, r.left, r.top);
    placeChip(p);
  }
}

/** Copie un recap des positions dans le presse-papier. */
export function copyPositions() {
  const out = panels
    .map((p) => {
      const r = p.getBoundingClientRect();
      return `#${p.id}: ${Math.round(r.left)},${Math.round(r.top)}  (${Math.round(r.width)}x${Math.round(r.height)})`;
    })
    .join('\n');
  const txt = `// stage ${innerWidth}x${innerHeight}\n${out}`;
  navigator.clipboard?.writeText(txt);
  console.log(txt);
  alert('Positions copiées :\n\n' + txt);
}
