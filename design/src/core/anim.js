/**
 * Boucle d'animation des box de démo. Tourne à ~`fps` images/s (ou `everyMs` si fourni) et se MET
 * EN PAUSE automatiquement pendant le Mode édition (body.editing) — chaque box évite ainsi de
 * dupliquer cette garde. Un seul timer par appel.
 */

/**
 * @param {(frame:number) => void} fn   appelée à chaque tick (reçoit le numéro de frame, 0,1,2…)
 * @param {{ fps?:number, everyMs?:number }} [opts]  cadence (fps par défaut = 25)
 * @returns {() => void} stop  arrête la boucle
 */
export function loop(fn, { fps = 25, everyMs } = {}) {
  const ms = everyMs ?? Math.round(1000 / fps);
  let frame = 0;
  const id = setInterval(() => {
    if (document.body.classList.contains('editing')) return; // pause en édition
    fn(frame++);
  }, ms);
  return () => clearInterval(id);
}
