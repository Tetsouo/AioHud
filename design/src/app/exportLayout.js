/**
 * Construit le DESCRIPTOR de layout (source de vérité unique mockup → UI native C++).
 * Agrège l'état LIVE : métadonnées des box (registry) + position/ancre courante (DOM) +
 * config résolue + visibilité manuelle + zones interdites. Coordonnées en % + viewport de réf
 * (le natif convertit en px selon la résolution réelle). Schéma documenté dans docs/EXPORT.md.
 *
 * Placé dans app/ (et non core/) : il dépend de registry/systems, alors que core reste sans dépendance.
 */
import { panels } from './registry.js';
import { resolveConfig } from '../core/config-store.js';
import { isHidden } from '../core/visibility.js';
import { round1 } from '../core/geometry.js';
import { num } from '../core/util.js';
import { zonesData } from '../systems/zonesSystem.js';

const VERSION = 1;

// config TYPÉE selon le schéma de la box : number -> nombre, toggle -> booléen, select/autre -> string.
// (resolveConfig renvoie des valeurs brutes — string si réglée à la main, défaut sinon ; on uniformise.)
function typedConfig(def) {
  const cfg = resolveConfig(def);
  const out = {};
  for (const f of def.config || []) {
    if (f.type === 'section') continue;
    const v = cfg[f.key];
    if (f.type === 'number') out[f.key] = num(v, f.default);
    else if (f.type === 'toggle') out[f.key] = String(v ?? f.default ?? '1') !== '0';
    else out[f.key] = v;
  }
  return out;
}

// ancre (tl/tr/bl/br) + position % depuis le positionnement CSS courant (left/right + top/bottom inline).
function placement(def) {
  const elx = document.getElementById(def.id);
  if (!elx) return { anchor: 'tl', pos: { x: round1(def.pos?.x ?? 0), y: round1(def.pos?.y ?? 0) } };
  const s = elx.style;
  const right = !!(s.right && s.right !== 'auto');
  const bottom = !!(s.bottom && s.bottom !== 'auto');
  return {
    anchor: (bottom ? 'b' : 't') + (right ? 'r' : 'l'),
    pos: { x: round1(parseFloat(right ? s.right : s.left) || 0), y: round1(parseFloat(bottom ? s.bottom : s.top) || 0) },
  };
}

/** @returns {object} le descriptor de layout complet. */
export function buildLayout() {
  const widgets = panels().map((def, i) => ({
    id: def.id,
    type: def.type || def.id,
    ...placement(def),
    size: { w: def.pos?.w ?? null },          // null = largeur auto (contenu)
    z: def.z ?? i,                            // défaut = ordre de registre
    visible: !isHidden(def.id),               // masquage MANUEL (le gating job est porté par `jobs`)
    jobs: def.jobs || null,
    growDown: !!def.growDown,
    bare: !!def.bare,
    config: typedConfig(def),                 // valeurs effectives, TYPÉES selon le schéma de la box
  }));
  return {
    version: VERSION,
    viewport: { w: window.innerWidth, h: window.innerHeight },
    widgets,
    zones: zonesData(),
  };
}
