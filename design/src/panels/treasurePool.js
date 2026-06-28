import { definePanel } from '../app/registry.js';
import { asset } from '../core/config.js';
import { clamp, num } from '../core/util.js';

// Treasure Pool — calque fidele d'AioHUD (boxes/treasurepool_box.lua + modules/treasurepool.lua).
// Icone coffre a gauche (centree verticalement) + colonnes alignees :
// idx (droite) / nom / temps restant (good vert / alert orange / bad rouge) / gagnant:lot (cyan).
// Donnees = le sample Help d'AioHUD.
const ITEMS = [
  { idx: '1', name: 'Heavy Metal Plate', time: '09:05', tier: 'good', loot: null },
  { idx: '2', name: 'Riftcinder', time: '08:10', tier: 'alert', loot: 'Tetsouo: 900' },
  { idx: '3', name: 'Beitetsu', time: '07:15', tier: 'bad', loot: null },
  { idx: '4', name: 'Riftborn Stone', time: '06:20', tier: 'good', loot: 'Tetsouo: 800' },
  { idx: '5', name: 'Pixie Hairpin +1', time: '05:25', tier: 'alert', loot: null },
  { idx: '6', name: 'Ginganauts Pole', time: '04:30', tier: 'bad', loot: 'Tetsouo: 700' },
  { idx: '7', name: 'Crepuscular Knife', time: '03:35', tier: 'good', loot: null },
  { idx: '8', name: "Mariselle's Pole", time: '02:40', tier: 'alert', loot: 'Tetsouo: 600' },
  { idx: '9', name: 'Defiant Scarf', time: '01:45', tier: 'bad', loot: null },
  { idx: '10', name: 'Ababinili', time: '00:50', tier: 'good', loot: 'Tetsouo: 500' },
];

const cells = (m) =>
  `<span class="tp-idx">${m.idx}</span>` +
  `<span class="tp-name">${m.name}</span>` +
  `<span class="tp-time ${m.tier}">${m.time}</span>` +
  `<span class="tp-loot">${m.loot || ''}</span>`;

definePanel({
  id: 'pool',
  title: 'Treasure Pool',
  type: 'TreasurePool',
  css: new URL('./treasurePool.css', import.meta.url).href,
  pos: { x: 70.6, y: 36.8 }, // pas de w -> auto-largeur
  // double-clic sur la box en Mode edition -> nombre d'items affiches
  config: [{ key: 'count', label: 'Items', type: 'number', min: 1, max: ITEMS.length, default: ITEMS.length }],
  render: (cfg = {}) => {
    const n = clamp(num(cfg.count, ITEMS.length), 1, ITEMS.length);
    return /* html */ `
    <div class="tp">
      <img class="tp-coffer" src="${asset('Coffer.png')}">
      <div class="tp-rows">${ITEMS.slice(0, n).map(cells).join('')}</div>
    </div>`;
  },
});
