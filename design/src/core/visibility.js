/**
 * Visibilite des box (afficher / masquer). Persistance : localStorage + visibility.json (PUT),
 * meme schema que les positions/config (le cache local prime, sinon le fichier).
 * Une box masquee a `display:none` -> elle sort aussi du drag/snap (offsetParent null).
 */
import { loadLocal, saveLocal, putJSON } from './store.js';
import { jobMatches } from './job.js';

const KEY = 'aiohud.visibility.v1';
const FILE = 'visibility.json';
let hidden = new Set(); // ids des box masquees A LA MAIN (menu Boxes)

/** Charge l'etat (cache local prioritaire, sinon le fichier). A appeler avant applyVisibility. */
export async function loadVisibility() {
  const local = loadLocal(KEY);
  if (local) { hidden = new Set(local); return; }
  try {
    const res = await fetch(FILE, { cache: 'no-store' });
    if (res.ok) hidden = new Set((await res.json()).hidden || []);
  } catch { /* fichier absent : tout visible */ }
}

export function isHidden(id) { return hidden.has(id); }

// visibilite EFFECTIVE : masquee a la main OU job non concordant (data-jobs pose par buildPanel).
function effHidden(p) { return hidden.has(p.id) || !jobMatches(p.dataset.jobs); }

function applyOne(id) {
  const p = document.getElementById(id);
  if (p) p.style.display = effHidden(p) ? 'none' : '';
}

/** Applique l'etat a toutes les box montees (boot, changement de job ou de masquage). */
export function applyVisibility() {
  document.querySelectorAll('.panel').forEach((p) => { p.style.display = effHidden(p) ? 'none' : ''; });
}

function persist() {
  const arr = [...hidden];
  saveLocal(KEY, arr);
  putJSON(FILE, { _README: 'Box masquees du HUD (barre edition -> Boxes).', hidden: arr });
}

/** Masque (h=true) ou affiche (h=false) une box, applique et persiste. */
export function setHidden(id, h) {
  if (h) hidden.add(id); else hidden.delete(id);
  applyOne(id);
  persist();
}
