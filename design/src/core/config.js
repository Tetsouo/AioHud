/**
 * Configuration centrale de la maquette AioHUD.
 * Tout ce qui est "constante de projet" vit ici — un seul endroit a editer.
 */

// Racine des vrais assets du jeu (servis depuis la racine Windower par serve.mjs).
export const ASSETS = '/addons/AioHUD/assets';

/** Construit une URL d'asset : asset('buffIcons/40.png'). */
export const asset = (path) => `${ASSETS}/${path}`;

// Symboles d'ELEMENTS FFXI (design/icons/elements, servi depuis la racine Windower). Insensible a la casse.
const ELEM_FILE = {
  fire: '_0007_Fire', ice: '_0006_Ice', wind: '_0003_Wind', earth: '_0002_Earth',
  lightning: '_0000_Thunder', thunder: '_0000_Thunder', water: '_0001_Water', light: '_0005_Light', dark: '_0004_Dark',
};
/** URL de l'icone d'un element : elemIcon('Water') / elemIcon('dark'). null si inconnu. */
export const elemIcon = (name) => {
  const f = name && ELEM_FILE[String(name).toLowerCase()];
  return f ? `/plugins/_aiohud_re/design/icons/elements/${f}.png` : null;
};

// Types de move BST NON elementaires (design/icons/wstypes) : physiques + generiques.
const TYPE_FILE = {
  piercing: '_0000_Piercing', slashing: '_0001_Slashing', debuff: '_0002_Debuff',
  magic: '_0003_Magic', buff: '_0004_Buff', blunt: '_0005_Blunt',
};
/** Icone d'un type de ready move BST : element (Out4) sinon physique/generique (Out5). null si inconnu. */
export const moveTypeIcon = (type) => {
  const e = elemIcon(type);
  if (e) return e;
  const f = type && TYPE_FILE[String(type).toLowerCase()];
  return f ? `/plugins/_aiohud_re/design/icons/wstypes/${f}.png` : null;
};

// Fichiers de donnees runtime (relatifs a la page) — auto-sauves par le serveur dev.
export const ZONES_URL = 'zones.json';
export const PANELS_URL = 'panels.json';

// Cles localStorage. Incremente le suffixe pour invalider un cache devenu obsolete.
export const STORAGE = {
  panels: 'aiohud.panels.v1',
  zones: 'aiohud.zones.v3',
};

// Reglages d'interaction (mode edition).
export const SNAP_PX = 9; // distance (px) du magnetisme entre box
