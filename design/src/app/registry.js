/**
 * Registre des box. Chaque module `panels/<box>.js` appelle `definePanel(...)` au chargement
 * (effet de bord) ; l'App récupère la liste via `panels()` pour les monter, et l'export l'utilise
 * comme source de métadonnées.
 *
 * ─── Contrat d'une définition de box (`PanelDef`) ───────────────────────────────────────────────
 * @typedef {Object} PanelDef
 * @property {string}   id         identifiant unique (clé de persistance + id DOM + clé d'export).
 * @property {string}   title      titre lisible (HTML autorisé, ex. icône). Sert au libellé d'édition.
 * @property {string}   type       nom de la classe widget NATIVE (ex. 'TargetBar') — consommé par l'export.
 * @property {string}   css        URL du CSS de la box : `new URL('./x.css', import.meta.url).href`.
 * @property {{x:number, y:number, w?:number}} pos  position par défaut : x/y en % d'écran, w en px (sinon auto).
 * @property {(cfg:Object) => string} render  HTML du corps `.body` ; reçoit toujours la config résolue.
 * @property {Array}    [config]   schéma de réglages (double-clic en Mode édition). Champs :
 *                                 `{ key, label, type:'number'|'select'|'toggle'|'section', default, options?, min?, max?, step? }`.
 * @property {(id:string)=>void} [onChange]  callback après changement de config (ex. visibilité d'alliances).
 * @property {string[]} [jobs]     gating : box affichée seulement si main OU sub ∈ jobs (ex. ['SCH']).
 * @property {boolean}  [bare]     true = pas de chrome (ni cadre ni titre) ; box image nue (ex. grimoire).
 * @property {boolean}  [growDown] true = ancrage HAUT forcé (le haut reste fixe, le contenu varie par le bas).
 * @property {number}   [z]        ordre de superposition (optionnel ; défaut = ordre de registre).
 *
 * ─── Gabarit canonique d'un fichier `panels/<box>.js` ───────────────────────────────────────────
 *   1. imports (registry, core/*, components/*, data locale)
 *   2. constantes data en UPPER_SNAKE + helpers de rendu
 *   3. `definePanel({ ... })`
 *   4. (optionnel) bloc d'animation démo via `loop()` de `core/anim.js` (pause auto en édition)
 */
const _panels = [];

/** Enregistre une box. @param {PanelDef} def @returns {PanelDef} */
export function definePanel(def) {
  _panels.push(def);
  return def;
}

/** @returns {PanelDef[]} copie de la liste des box enregistrées (ordre de chargement). */
export function panels() {
  return _panels.slice();
}
