/** Maths de rectangles : clamp, overlap, magnetisme. Pures, testables, sans DOM. */

export const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
export const round1 = (v) => Math.round(v * 10) / 10;

/** Rectangle absolu (px) d'un element : {left,top,right,bottom,width,height}. */
export function rectOf(elt) {
  const r = elt.getBoundingClientRect();
  return { left: r.left, top: r.top, right: r.right, bottom: r.bottom, width: r.width, height: r.height };
}

/** La box (x,y,w,h) chevauche-t-elle le rectangle r ? (contact = autorise) */
export function overlaps(x, y, w, h, r) {
  return x < r.right && x + w > r.left && y < r.bottom && y + h > r.top;
}

/**
 * Magnetisme : colle les bords de la box (x,y,w,h) a ceux des rectangles voisins.
 * Retourne {x,y} ajuste. 4 collages par axe : adjacence (bord a bord) + alignement.
 */
export function snap(x, y, w, h, rects, dist) {
  let bx = null, db = dist, by = null, dy = dist;
  for (const o of rects) {
    const g = o.gap || 0;                                           // ecart laisse a l'adjacence (0 = bord a bord)
    const vGate = (y < o.bottom + dist) && (y + h > o.top - dist);   // ~cote a cote
    const hGate = (x < o.right + dist) && (x + w > o.left - dist);   // ~l'un sur l'autre
    if (vGate) {
      // X : adjacence gauche/droite (avec gap), puis alignement gauche/droite (sans gap)
      for (const [val, diff] of [[o.left - w - g, (x + w) - (o.left - g)], [o.right + g, x - (o.right + g)], [o.left, x - o.left], [o.right - w, (x + w) - o.right]]) {
        const d = Math.abs(diff); if (d < db) { db = d; bx = val; }
      }
    }
    if (hGate) {
      // Y : adjacence haut/bas (avec gap), puis alignement haut/bas (sans gap)
      for (const [val, diff] of [[o.top - h - g, (y + h) - (o.top - g)], [o.bottom + g, y - (o.bottom + g)], [o.top, y - o.top], [o.bottom - h, (y + h) - o.bottom]]) {
        const d = Math.abs(diff); if (d < dy) { dy = d; by = val; }
      }
    }
  }
  return { x: bx ?? x, y: by ?? y };
}

/** Snap des bords gauche/haut de la box sur la grille (pas `cell`) si dans `dist`. */
export function snapGrid(x, y, cell, dist) {
  const near = (v) => { const g = Math.round(v / cell) * cell; return Math.abs(g - v) <= dist ? g : v; };
  return { x: near(x), y: near(y) };
}

/** Snap le CENTRE horizontal de la box sur `cx` (centre ecran) si dans `dist`. -> {x, snapped}. */
export function snapCenterX(x, w, cx, dist) {
  const target = cx - w / 2;
  return Math.abs(target - x) <= dist ? { x: target, snapped: true } : { x, snapped: false };
}
