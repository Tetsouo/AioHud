/**
 * Reglages par box (variantes, options). Persistance : localStorage + boxconfig.json (PUT).
 * Une box declare son schema via definePanel({ config: [...] }) ; la valeur courante est
 * resolue ici (valeur stockee, sinon defaut du champ).
 */
import { loadLocal, saveLocal, putJSON } from './store.js';

const KEY = 'aiohud.boxconfig.v1';
const URL = 'boxconfig.json';

let data = loadLocal(KEY); // null si jamais configure

/** Charge boxconfig.json au boot SI aucun cache local (sinon le cache prime). */
export async function loadConfigFile() {
  if (data) return;
  try {
    const res = await fetch(URL, { cache: 'no-store' });
    if (res.ok) data = (await res.json()).config || {};
  } catch { /* fichier absent */ }
  if (!data) data = {};
}

export function getConfig(id) { return (data && data[id]) || {}; }

export function setConfig(id, patch) {
  data = data || {};
  data[id] = { ...data[id], ...patch };
  saveLocal(KEY, data);
  putJSON(URL, { _README: 'Reglages par box (variantes, options). Auto-sauve depuis la page.', config: data });
}

/** Valeurs effectives d'une box : champ stocke, sinon defaut declare. */
export function resolveConfig(def) {
  const stored = getConfig(def.id);
  const out = {};
  for (const f of def.config || []) out[f.key] = (f.key in stored) ? stored[f.key] : f.default;
  return out;
}
