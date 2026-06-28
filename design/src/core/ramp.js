/**
 * Rampes de couleur vert (plein) → jaune → rouge (0), pour barres HP / temps restant.
 * `pct` en 0..1. Les variantes ne diffèrent que par la base/amplitude RGB :
 *  - sombre  : un texte blanc reste lisible par-dessus (ex : hate list)
 *  - claire  : barre seule, plus vive (ex : zone tracker)
 */

// facteurs (r, g) de la rampe à partir de pct 0..1.
const gyr = (pct) => {
  pct = Math.max(0, Math.min(1, pct));
  return pct > 0.5 ? { r: (1 - pct) * 2, g: 1 } : { r: 1, g: pct * 2 };
};

const rgbRamp = (pct, rBase, rAmp, gBase, gAmp, b) => {
  const { r, g } = gyr(pct);
  return `rgb(${Math.floor(rBase + r * rAmp)},${Math.floor(gBase + g * gAmp)},${b})`;
};

/** HP (barre avec texte blanc par-dessus) — teintes assombries. */
export const hpRamp = (pct) => rgbRamp(pct, 32, 122, 36, 120, 46);

/** Temps restant (barre seule) — teintes plus claires. */
export const timeRamp = (pct) => rgbRamp(pct, 45, 185, 45, 180, 55);

/** HP discret à 4 paliers (couleur de jauge), `p` en 0..100 — ex : party / alliance. */
export const hpStep = (p) => (p > 75 ? '#9bdc8c' : p > 50 ? '#f3f37c' : p > 25 ? '#f8ba80' : '#fc8182');
