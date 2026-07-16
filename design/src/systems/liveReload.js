/**
 * Live-reload dev. Recharge la page quand un fichier source change.
 *  1. canal prioritaire : SSE (EventSource) pousse par serve.mjs (instantane)
 *  2. repli : polling d'un endpoint de version (mtime max)
 * Inerte sur file:// (les ES modules exigent http de toute facon).
 */
export function initLiveReload() {
  if (location.protocol === 'file:') return;

  // 1) Server-Sent Events
  try {
    const es = new EventSource('__livereload');
    es.onmessage = (e) => { if (e.data === 'reload') location.reload(); };
    es.onerror = () => { es.close(); pollVersion(); };
    return;
  } catch {
    pollVersion();
  }
}

// 2) Repli : interroge __version (mtime max des fichiers) toutes les 700 ms.
function pollVersion() {
  let last = null;
  const tick = async () => {
    try {
      const v = await (await fetch('__version', { cache: 'no-store' })).text();
      if (last !== null && v !== last) { location.reload(); return; }
      last = v;
    } catch { /* serveur non joignable : on reessaie */ }
    setTimeout(tick, 700);
  };
  tick();
}
