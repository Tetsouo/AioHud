/**
 * Icones SVG inline (Material Icons, viewBox 24x24, fill=currentColor -> herite la couleur du texte).
 * REGLE PROJET : jamais d'emoji dans l'UI -> on passe toujours par icon('nom').
 */
const PATHS = {
  // engrenage (reglages)
  settings: 'M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z',
  // interdiction (zones jeu)
  block: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zM4 12c0-4.42 3.58-8 8-8 1.85 0 3.55.63 4.9 1.69L5.69 16.9C4.63 15.55 4 13.85 4 12zm8 8c-1.85 0-3.55-.63-4.9-1.69L18.31 7.1C19.37 8.45 20 10.15 20 12c0 4.42-3.58 8-8 8z',
  // copier
  copy: 'M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z',
  // ajouter
  add: 'M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z',
  // recharger (fichier)
  refresh: 'M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z',
  // deplacer (4 fleches)
  move: 'M10 9h4V6h3l-5-5-5 5h3v3zm-1 1H6V7l-5 5 5 5v-3h3v-4zm14 2l-5-5v3h-3v4h3v3l5-5zm-9 3h-4v3H8l5 5 5-5h-3v-3z',
  // fleche d'arborescence (sous-element)
  subarrow: 'M19 15l-6 6-1.42-1.42L15.17 16H4V4h2v10h9.17l-3.59-3.58L13 9l6 6z',
  // double chevron ">>" (cible visee par un mob dans la hate list)
  dblarrow: 'M15.5 5H11l5 7-5 7h4.5l5-7-5-7zM8.5 5H4l5 7-5 7h4.5l5-7-5-7z',
  // fleche droite "->" (skillchain : etape suivante / propriete)
  arrow: 'M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z',
  // etincelle UNIQUE (sort en cours d'incantation) — etoile 4 branches, centree, remplit le cadre
  cast: 'M12 3c.82 6.38 2.62 8.18 9 9-6.38.82-8.18 2.62-9 9-.82-6.38-2.62-8.18-9-9 6.38-.82 8.18-2.62 9-9z',
  // triangle d'alerte (TP move ennemi dangereux en preparation)
  warn: 'M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z',
  // liste (menu d'affichage des box)
  list: 'M3 13h2v-2H3v2zm0 4h2v-2H3v2zm0-8h2V7H3v2zm4 4h14v-2H7v2zm0 4h14v-2H7v2zM7 7v2h14V7H7z',
  // personnage (selecteur de job)
  user: 'M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z',
  // grille (snap sur grille)
  grid: 'M3 3v8h8V3H3zm6 6H5V5h4v4zm-6 4v8h8v-8H3zm6 6H5v-4h4v4zm4-16v8h8V3h-8zm6 6h-4V5h4v4zm-6 4v8h8v-8h-8zm6 6h-4v-4h4v4z',
  // telechargement / export
  download: 'M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z',
  // croix (KO) — conserve au cas ou
  close: 'M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z',
  // couronne (alliance leader) — viewBox serre sur le tracé
  crown: { vb: '2 3 20 18', d: 'M2 6l4.5 4.2L12 3l5.5 7.2L22 6l-2.3 11H4.3L2 6zM4 18.6h16V21H4z' },
  // etoile (party leader)
  star: { vb: '2 2 20 19', d: 'M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z' },
  // signe dollar (quarter master / loot) — glyphe etroit -> viewBox serre pour coller le $ (pas de vide)
  dollar: { vb: '6.9 2.5 10.5 18.5', d: 'M13.3 10.9c-2.27-.59-3-1.2-3-2.15 0-1.09 1.01-1.85 2.7-1.85 1.78 0 2.44.85 2.5 2.1h2.21c-.07-1.72-1.12-3.3-3.21-3.81V3h-3v2.16c-1.94.42-3.5 1.68-3.5 3.61 0 2.31 1.91 3.46 4.7 4.13 2.5.6 3 1.48 3 2.41 0 .69-.49 1.79-2.7 1.79-2.06 0-2.87-.92-2.98-2.1h-2.2c.12 2.19 1.76 3.42 3.68 3.83V21h3v-2.15c1.95-.37 3.5-1.5 3.5-3.55 0-2.84-2.43-3.81-4.7-4.4z' },
  // tete de mort (KO) — yeux/nez evides via fill-rule evenodd
  skull: { evenodd: true, d: 'M12 2c-4.42 0-8 3.36-8 7.5 0 2.31 1.1 4.38 2.84 5.74.41.32.66.81.66 1.33V18c0 .55.45 1 1 1h.5v1.5c0 .28.22.5.5.5s.5-.22.5-.5V19h1v1.5c0 .28.22.5.5.5s.5-.22.5-.5V19h1v1.5c0 .28.22.5.5.5s.5-.22.5-.5V19h.5c.55 0 1-.45 1-1v-1.43c0-.52.25-1.01.66-1.33C18.9 13.88 20 11.81 20 9.5 20 5.36 16.42 2 12 2zM9 11.5a1.75 1.75 0 110-3.5 1.75 1.75 0 010 3.5zm6 0a1.75 1.75 0 110-3.5 1.75 1.75 0 010 3.5zm-3 1l1 2h-2z' },
};

/** Retourne le markup SVG d'une icone. icon('settings', 14). */
export function icon(name, size = 14) {
  const p = PATHS[name];
  const d = typeof p === 'string' ? p : (p && p.d) || '';
  const rule = (p && p.evenodd) ? ' fill-rule="evenodd" clip-rule="evenodd"' : '';
  // viewBox serre (vb) -> dimensionne par la HAUTEUR, largeur intrinseque = au plus pres du glyphe.
  const vb = (p && p.vb) || '0 0 24 24';
  const dims = (p && p.vb) ? `height="${size}"` : `width="${size}" height="${size}"`;
  return `<svg class="ic" viewBox="${vb}" ${dims} fill="currentColor" aria-hidden="true">`
    + `<path${rule} d="${d}"/></svg>`;
}
