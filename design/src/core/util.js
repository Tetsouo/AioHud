/** Petits utilitaires purs, sans DOM ni dépendance. */

/** Borne `v` dans l'intervalle [lo, hi]. */
export const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

/** Parse `v` en nombre fini ; retourne le défaut `d` si invalide (NaN, undefined…). */
export const num = (v, d) => { const n = +v; return Number.isFinite(n) ? n : d; };
