/**
 * Zones interdites = UI native du jeu qu'on ne peut pas bouger.
 * Editables dans la page (Mode "⛔ Zones jeu") : deplacer / redimensionner /
 * ajouter / renommer / supprimer, avec auto-save vers zones.json.
 *
 * `allow` (liste d'ids de box) = exception : ces box peuvent chevaucher CETTE zone.
 * C'est une regle de config -> toujours re-appliquee depuis le fichier (jamais perdue).
 */
import { clamp, round1 } from '../core/geometry.js';
import { loadLocal, saveLocal, removeLocal, fetchText, putJSON } from '../core/store.js';
import { STORAGE, ZONES_URL } from '../core/config.js';

let host = null;
let zones = [];      // modele : { label, x, y, w, h, allow[] } en % de l'ecran
let readme = '';
let dirty = false;   // true des qu'on edite dans la page (stoppe le poll fichier)
let fileLast = null; // dernier texte zones.json vu (detection de changement)
let busy = false;    // un drag/resize est en cours

const zoning = () => document.body.classList.contains('zoning');
const pctX = (px) => px / innerWidth * 100;
const pctY = (px) => px / innerHeight * 100;

/** Zones en px (+ allow) pour la detection de collision des box (dragSystem). */
export function zoneRects() {
  return zones.map((z) => ({
    left: z.x / 100 * innerWidth,
    top: z.y / 100 * innerHeight,
    right: (z.x + z.w) / 100 * innerWidth,
    bottom: (z.y + z.h) / 100 * innerHeight,
    allow: Array.isArray(z.allow) ? z.allow : [],
  }));
}

/** Zones en % (modele brut, copie) pour l'export du layout. */
export function zonesData() {
  return zones.map((z) => ({
    label: z.label, x: round1(z.x), y: round1(z.y), w: round1(z.w), h: round1(z.h),
    allow: Array.isArray(z.allow) ? z.allow.slice() : [],
  }));
}

/* ---- persistance ------------------------------------------------------- */
function buildJSON() {
  return {
    _README: readme,
    zones: zones.map((z) => {
      const o = { label: z.label, x: round1(z.x), y: round1(z.y), w: round1(z.w), h: round1(z.h) };
      if (Array.isArray(z.allow) && z.allow.length) o.allow = z.allow; // garde l'exception
      return o;
    }),
  };
}
function markDirty() {
  dirty = true;
  saveLocal(STORAGE.zones, { readme, zones });
  putJSON(ZONES_URL, buildJSON());
}

/* ---- rendu ------------------------------------------------------------- */
function updChip(z, chip) {
  chip.textContent = `x${round1(z.x)} y${round1(z.y)} · w${round1(z.w)} h${round1(z.h)}`;
}
function render() {
  host.innerHTML = '';
  zones.forEach((z, i) => {
    const d = document.createElement('div');
    d.className = 'zone';
    d.dataset.i = i;
    d.style.left = z.x + '%';
    d.style.top = z.y + '%';
    d.style.width = z.w + '%';
    d.style.height = z.h + '%';

    const lab = document.createElement('span');
    lab.className = 'zlabel';
    lab.textContent = z.label || '';
    lab.contentEditable = 'false';
    lab.addEventListener('dblclick', () => {
      if (!zoning()) return;
      lab.contentEditable = 'true';
      lab.focus();
      document.execCommand?.('selectAll', false, null);
    });
    lab.addEventListener('blur', () => {
      lab.contentEditable = 'false';
      z.label = lab.textContent.trim();
      markDirty();
    });
    lab.addEventListener('keydown', (e) => { if (e.key === 'Enter') { e.preventDefault(); lab.blur(); } });

    const del = document.createElement('div');
    del.className = 'zdel';
    del.textContent = '×';
    del.title = 'Supprimer';
    del.addEventListener('mousedown', (e) => {
      e.stopPropagation();
      e.preventDefault();
      zones.splice(i, 1);
      markDirty();
      render();
    });

    const res = document.createElement('div');
    res.className = 'zres';
    res.title = 'Redimensionner';
    const chip = document.createElement('div');
    chip.className = 'zchip';

    d.append(lab, del, res, chip);
    host.appendChild(d);
    d._chip = chip;
    updChip(z, chip);
  });
}

/* ---- drag & resize ----------------------------------------------------- */
let act = null; // { z, el, mode, sx, sy, bx, by, bw, bh }
function onDown(e) {
  if (!zoning()) return;
  const el = e.target.closest('.zone');
  if (!el) return;
  if (e.target.classList.contains('zlabel') && el.querySelector('.zlabel').isContentEditable) return;
  const z = zones[+el.dataset.i];
  if (!z) return;
  e.preventDefault();
  const mode = e.target.classList.contains('zres') ? 'resize' : 'move';
  act = { z, el, mode, sx: e.clientX, sy: e.clientY, bx: z.x, by: z.y, bw: z.w, bh: z.h };
  busy = true;
}
function onMove(e) {
  if (!act) return;
  const z = act.z, dx = pctX(e.clientX - act.sx), dy = pctY(e.clientY - act.sy);
  if (act.mode === 'move') {
    z.x = clamp(act.bx + dx, 0, 100 - z.w);
    z.y = clamp(act.by + dy, 0, 100 - z.h);
    act.el.style.left = z.x + '%';
    act.el.style.top = z.y + '%';
  } else {
    z.w = clamp(act.bw + dx, 1, 100 - z.x);
    z.h = clamp(act.bh + dy, 1, 100 - z.y);
    act.el.style.width = z.w + '%';
    act.el.style.height = z.h + '%';
  }
  updChip(z, act.el._chip);
}
function onUp() {
  if (!act) return;
  const z = act.z;
  z.x = round1(z.x); z.y = round1(z.y); z.w = round1(z.w); z.h = round1(z.h);
  act = null;
  busy = false;
  markDirty();
}

/* ---- chargement fichier ------------------------------------------------ */
async function loadFile(force) {
  try {
    const txt = await fetchText(ZONES_URL);
    if (!force && txt === fileLast) return;
    fileLast = txt;
    const data = JSON.parse(txt);
    readme = data._README || readme;
    if (force || !dirty) {
      zones = (data.zones || []).map((z) => ({
        label: z.label || '', x: +z.x || 0, y: +z.y || 0, w: +z.w || 0, h: +z.h || 0,
        allow: Array.isArray(z.allow) ? z.allow : [],
      }));
      render();
    }
  } catch { /* JSON invalide en cours d'edition : garde le dernier rendu valide */ }
}
// re-applique les exceptions `allow` depuis le fichier (par label) meme si la geometrie vient du cache
async function refreshAllowFromFile() {
  try {
    const data = JSON.parse(await fetchText(ZONES_URL));
    const by = {};
    (data.zones || []).forEach((z) => { if (z.label) by[z.label] = Array.isArray(z.allow) ? z.allow : []; });
    let changed = false;
    zones.forEach((z) => {
      const a = by[z.label];
      if (a && JSON.stringify(a) !== JSON.stringify(z.allow)) { z.allow = a; changed = true; }
    });
    if (changed) render();
  } catch { /* noop */ }
}

/* ---- cablage des boutons + boot --------------------------------------- */
function wireToolbar() {
  document.getElementById('zoneAddBtn').addEventListener('click', () => {
    zones.push({ label: 'Nouvelle zone', x: 40, y: 42, w: 20, h: 16, allow: [] });
    markDirty();
    render();
  });
  document.getElementById('zoneExportBtn').addEventListener('click', () => {
    const txt = JSON.stringify(buildJSON(), null, 2) + '\n';
    navigator.clipboard?.writeText(txt);
    alert('zones.json copié dans le presse-papier — colle-le dans design/zones.json :\n\n' + txt);
  });
  document.getElementById('zoneReloadBtn').addEventListener('click', () => {
    removeLocal(STORAGE.zones);
    dirty = false;
    fileLast = null;
    loadFile(true);
  });
}

export function initZonesSystem() {
  host = document.getElementById('zones');
  host.addEventListener('mousedown', onDown);
  document.addEventListener('mousemove', onMove);
  document.addEventListener('mouseup', onUp);
  wireToolbar();

  const saved = loadLocal(STORAGE.zones);
  if (saved && Array.isArray(saved.zones)) {
    zones = saved.zones.map((z) => ({ allow: [], ...z }));
    readme = saved.readme || '';
    dirty = true;
    render();
    refreshAllowFromFile();
  } else {
    loadFile(true);
  }
  // sync live des editions a la main du fichier, tant qu'on n'a pas touche une zone
  setInterval(() => { if (!busy && !dirty) loadFile(); }, 700);
}
