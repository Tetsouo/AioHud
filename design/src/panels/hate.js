import { definePanel } from '../app/registry.js';
import { icon } from '../core/icons.js';
import { clamp, num } from '../core/util.js';
import { hpRamp } from '../core/ramp.js';

// Hate list — calque fidele d'AioHUD (boxes/hatelist_box.lua + modules/hatelist.lua).
// Ligne : [distance] [barre fiole : nom (gauche) + HP% (droite), caps dores] >> Cible(joueur vise).
// La ligne que TU cibles est encadree en or (rouge si claim adverse). Donnees = sample officiel.
// Nombre d'aggro affiches reglable par DOUBLE-CLIC sur la box (1 a 12).

// pool de mobs en aggro (les 3 premiers = sample officiel, dont la cible). Le reglage en affiche N.
const POOL = [
  { name: 'Hpemde',           hpp: 14, dist: 22.4, pcname: 'Tetsouo', target: false },
  { name: 'Cunning Sammael',  hpp: 47, dist: 11.8, pcname: null,      target: false },
  { name: 'Apex Bat',         hpp: 88, dist: 5.2,  pcname: null,      target: true },
  { name: 'Gigas Bonze',      hpp: 63, dist: 14.1, pcname: 'Kaories', target: false },
  { name: 'Hippogryph',       hpp: 31, dist: 9.7,  pcname: null,      target: false },
  { name: 'Wespe',            hpp: 95, dist: 18.3, pcname: null,      target: false, red: true },
  { name: 'Buccaboo',         hpp: 52, dist: 7.4,  pcname: null,      target: false },
  { name: 'Wild Karakul',     hpp: 76, dist: 25.0, pcname: null,      target: false },
  { name: 'Snoll',            hpp: 40, dist: 13.6, pcname: 'Brakuk',  target: false },
  { name: 'Bugbear Strongman',hpp: 21, dist: 8.9,  pcname: null,      target: false },
  { name: 'Goblin Tinkerer',  hpp: 67, dist: 16.2, pcname: null,      target: false },
  { name: 'Peiste',           hpp: 58, dist: 19.5, pcname: null,      target: false },
  { name: 'Greater Colibri',  hpp: 83, dist: 21.7, pcname: null,      target: false },
  { name: 'Tonberry Stalker', hpp: 9,  dist: 12.3, pcname: 'Selene',  target: false },
  { name: 'Marolith',         hpp: 71, dist: 27.4, pcname: null,      target: false },
];

const row = (m) => {
  const frame = m.target ? (m.red ? 'target red' : 'target') : '';
  // cible affichee a sa largeur naturelle (barre fixe -> deja uniforme) ; la box epouse le contenu
  const tgt = m.pcname ? `<span class="harrow">${icon('dblarrow', 11)}</span><span class="htgt">${m.pcname}</span>` : '';
  const nm = m.name.length > 10 ? m.name.slice(0, 10).trimEnd() + '…' : m.name; // tronque a 10 car. avec …
  return /* html */ `
    <div class="haterow ${frame}">
      <span class="hd">${m.dist.toFixed(1)}</span>
      <div class="hbar">
        <i style="width:${m.hpp}%;background:${hpRamp(m.hpp / 100)}"></i>
        <span class="hname">${nm}</span>
        <span class="hpct">${m.hpp}%</span>
      </div>
      <div class="htarget">${tgt}</div>
    </div>`;
};

definePanel({
  id: 'hate',
  title: 'Hate',
  type: 'HateList',
  css: new URL('./hate.css', import.meta.url).href,
  pos: { x: 22.8, y: 32.2 },   // pas de largeur fixe -> la box epouse son contenu (aucune marge a droite)
  growDown: true,              // ancrage par le HAUT : le haut reste fixe, les lignes du bas depop
  // double-clic sur la box -> choisir combien de mobs en aggro afficher (1 a 12)
  config: [{ key: 'count', label: "Nb d'aggro", type: 'number', min: 1, max: POOL.length, default: 3 }],
  render: (cfg = {}) => POOL.slice(0, clamp(num(cfg.count, 3), 1, POOL.length)).map(row).join(''),
});
