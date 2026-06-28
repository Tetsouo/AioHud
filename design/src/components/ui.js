/**
 * Briques d'UI reutilisables (retournent du HTML string).
 * Mutualise les motifs repetes dans les box : barres et icones de statut.
 */
import { asset } from '../core/config.js';

/**
 * Barre HP/MP/TP (placeholder — remplacee par les fioles natives C++).
 * @param {'hp'|'mp'|'tp'|''} kind
 * @param {number} pct  remplissage 0-100
 * @param {object} [o]  { label, value, cls, inner, barStyle, fillStyle }
 */
export function bar(kind, pct, o = {}) {
  const { label = '', value = '', cls = '', inner = '', barStyle = '', fillStyle = '' } = o;
  const classes = ['bar', kind, cls].filter(Boolean).join(' ');
  return `<div class="${classes}" style="${barStyle}">`
    + `${inner}<i style="width:${pct}%;${fillStyle}"></i>`
    + (label ? `<span class="bt">${label}</span>` : '')
    + (value !== '' ? `<span class="bv">${value}</span>` : '')
    + `</div>`;
}

/**
 * Icone de statut (buff/debuff) avec timer optionnel.
 * @param {string} icon  chemin relatif sous assets (ex: 'buffIcons/40.png')
 * @param {object} [o]   { mine:boolean, timer:string, short:boolean }
 */
export function statIcon(icon, o = {}) {
  const { mine = false, timer = '', short = false } = o;
  return `<div class="statwrap${mine ? ' mine' : ''}">`
    + `<img src="${asset(icon)}">`
    + (timer ? `<span class="tm${short ? ' short' : ''}">${timer}</span>` : '')
    + `</div>`;
}

/** Icone de job : jobIcon('pld') -> <img class="job" ...>. */
export const jobIcon = (job) => `<img class="job" src="${asset(`jobIcons/${job}.png`)}">`;
