/**
 * Persistance : localStorage (cache navigateur) + ecriture fichier via le serveur dev.
 * Toutes les operations sont defensives (jamais throw cote appelant).
 */

export function loadLocal(key) {
  try { return JSON.parse(localStorage.getItem(key) || 'null'); }
  catch { return null; }
}

export function saveLocal(key, value) {
  try { localStorage.setItem(key, JSON.stringify(value)); } catch { /* quota / private mode */ }
}

export function removeLocal(key) {
  try { localStorage.removeItem(key); } catch { /* noop */ }
}

/** GET JSON sans cache. Throw si !ok (a catcher par l'appelant). */
export async function fetchJSON(url) {
  const res = await fetch(url, { cache: 'no-store' });
  if (!res.ok) throw new Error(`${url} -> ${res.status}`);
  return res.json();
}

/** GET texte brut sans cache. */
export async function fetchText(url) {
  const res = await fetch(url, { cache: 'no-store' });
  return res.text();
}

/**
 * Ecrit un objet JSON sur le disque via PUT (auto-save).
 * Sans serveur capable d'ecrire, l'erreur est silencieuse (fallback localStorage + export).
 */
export function putJSON(url, obj) {
  try {
    return fetch(url, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(obj, null, 2) + '\n',
    });
  } catch { return Promise.resolve(); }
}
