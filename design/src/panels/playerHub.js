import { definePanel } from '../app/registry.js';
import { bar, statIcon, jobIcon } from '../components/ui.js';
import { asset } from '../core/config.js';

// Set PLD reel de Tetsouo (sets.idle de son GearSwap), noms resolus via res/items.lua.
// Main/Sub ne sont pas dans le set idle (definis ailleurs) -> p:true (a confirmer).
const GEAR = [
  { slot: 'Main', id: 21621, name: 'Naegling', p: true },
  { slot: 'Sub', id: 28648, name: 'Priwen', p: true },
  { slot: 'Range', id: 0, name: 'vide' },
  { slot: 'Ammo', id: 22279, name: 'Staunch Tathlum +1' },
  { slot: 'Head', id: 23426, name: 'Chev. Armet +3' },
  { slot: 'Neck', id: 25455, name: 'Kgt. Beads +2' },
  { slot: 'Ear 1', id: 26112, name: 'Tuisto Earring' },
  { slot: 'Ear 2', id: 27549, name: 'Odnowa Earring +1' },
  { slot: 'Body', id: 23861, name: 'Adamantite Armor' },
  { slot: 'Hands', id: 23560, name: 'Chev. Gauntlets +3' },
  { slot: 'Ring 1', id: 10773, name: 'Fortified Ring' },
  { slot: 'Ring 2', id: 10769, name: 'Gelatinous Ring +1' },
  { slot: 'Back', id: 26252, name: "Rudianos's Mantle" },
  { slot: 'Waist', id: 28437, name: 'Flume Belt +1' },
  { slot: 'Legs', id: 23627, name: 'Chev. Cuisses +3' },
  { slot: 'Feet', id: 23694, name: 'Chev. Sabatons +3' },
];
// Disposition = fenetre Equipement FFXI (ordre naturel des slots) :
//   Main Sub Range Ammo / Head Neck Ear1 Ear2 / Body Hands Ring1 Ring2 / Back Waist Legs Feet
const eqCell = (g) => g.id
  ? `<div class="eqcell" title="${g.slot}: ${g.name}${g.p ? ' (placeholder)' : ''}"><img src="${asset(`gearicons/${g.id}.bmp`)}"></div>`
  : `<div class="eqcell" title="${g.slot}: vide"></div>`;

// blocs de contenu (montres selon les toggles de config)
const tpBar = '<div class="bar tp ready"><i style="width:61%"></i><span class="notch" style="left:33.3%"></span><span class="notch" style="left:66.6%"></span><span class="bt">TP</span><span class="float" style="left:61%">1850</span></div>';
const selfBlock = '<div class="castself"><i></i><span class="fc"></span><span class="ct">Cure IV</span><span class="cv">0.8s</span></div>'
  + '<div class="selfres"><span class="ws">Savage Blade</span> → 3 hits <span class="dmg">12,847</span></div>';
// 12 buffs joueur (memes icones/taille que les debuffs de la cible). Mix avec/sans timer.
const BUF_LIST = [
  [40, ''], [41, ''], [42, '4:58'], [43, '1:12'], [44, '3:20'], [33, '2:40'],
  [39, '0:45'], [37, ''], [38, '5:00'], [36, '0:18'], [35, '1:30'], [251, '28:00'],
  [45, '0:50'], [46, ''],
];
const buffsTray = () => `<div class="tray">${BUF_LIST.map(([id, t]) =>
  statIcon(`buffIcons/${id}.png`, t ? { timer: t } : {})).join('')}</div>`;

definePanel({
  id: 'hub',
  title: 'Player',
  type: 'PlayerHub',
  css: new URL('./playerHub.css', import.meta.url).href,
  pos: { x: 36.8, y: 74.0 }, // largeur AUTO (max-content) -> la box retrecit si l'equipement est masque
  // reglages (double-clic en Mode edition) : position de l'equipement + on/off de chaque section.
  config: [
    {
      key: 'equip', label: 'Équipement', type: 'select', default: 'right',
      options: [{ value: 'right', label: 'À droite' }, { value: 'left', label: 'À gauche' }, { value: 'off', label: 'Masqué' }],
    },
    {
      key: 'bars', label: 'Barres HP/MP/TP', type: 'select', default: 'stacked',
      options: [{ value: 'stacked', label: 'Empilées' }, { value: 'row', label: 'Côte à côte' }, { value: 'vertical', label: 'Verticales' }],
    },
    { type: 'section', label: 'Identité' },
    { key: 'name', label: 'Nom', type: 'toggle', default: '1' },
    { key: 'job', label: 'Job', type: 'toggle', default: '1' },
    { key: 'engaged', label: 'Engaged', type: 'toggle', default: '1' },
    { type: 'section', label: 'Sections affichées' },
    { key: 'hp', label: 'HP', type: 'toggle', default: '1' },
    { key: 'mp', label: 'MP', type: 'toggle', default: '1' },
    { key: 'tp', label: 'TP', type: 'toggle', default: '1' },
    { key: 'cast', label: 'Self Cast', type: 'toggle', default: '1' },
    { key: 'gil', label: 'Gil', type: 'toggle', default: '1' },
    { key: 'speed', label: 'Speed', type: 'toggle', default: '1' },
    { key: 'buffs', label: 'Buffs', type: 'toggle', default: '1' },
  ],
  render: (cfg = {}) => {
    const on = (k) => String(cfg[k] ?? '1') !== '0';
    const equip = ['right', 'left', 'off'].includes(cfg.equip) ? cfg.equip : 'right';
    const gil = on('gil'), spd = on('speed');
    const gilrow = (gil || spd)
      ? `<div class="gilrow">${gil ? `<img src="${asset('gil.png')}"><b>1,250,400</b>` : ''}${spd ? '<span class="spd">Speed <b>+0%</b></span>' : ''}</div>`
      : '';
    const idHtml = (on('job') || on('name') || on('engaged'))
      ? `<div class="idrow">${on('job') ? jobIcon('pld') : ''}${on('name') ? '<span class="nm">Tetsouo</span>' : ''}`
        + `${on('job') ? '<span class="jb">PLD/WAR · Lv.99</span>' : ''}${on('engaged') ? '<span class="st eng">Engaged</span>' : ''}</div>`
      : '';
    // vitals HP/MP/TP : 3 dispositions (la largeur de la box s'adapte via la classe bars-* sur hub-main).
    const barsMode = ['stacked', 'row', 'vertical'].includes(cfg.bars) ? cfg.bars : 'stacked';
    let vitals;
    if (barsMode === 'vertical') {
      const vb = (k, pct, label, value) => on(k)
        ? `<div class="vbar ${k}${k === 'tp' ? ' ready' : ''}"><span class="vbv">${value}</span>`
          + `<div class="vbtrack"><i style="height:${pct}%"></i>${k === 'tp' ? '<span class="vnotch" style="bottom:33.3%"></span><span class="vnotch" style="bottom:66.6%"></span>' : ''}</div>`
          + `<span class="vbl">${label}</span></div>`
        : '';
      const cells = vb('hp', 100, 'HP', '1188') + vb('mp', 42, 'MP', '412') + vb('tp', 61, 'TP', '1850');
      vitals = cells ? `<div class="vbars">${cells}</div>` : '';
    } else if (barsMode === 'row') {
      // barres horizontales COTE A COTE (valeurs compactes pour tenir dans 3 colonnes)
      const tpRow = '<div class="bar tp ready"><i style="width:61%"></i><span class="notch" style="left:33.3%"></span><span class="notch" style="left:66.6%"></span><span class="bt">TP</span><span class="bv">1850</span></div>';
      const cells = (on('hp') ? bar('hp', 100, { label: 'HP', value: '1188' }) : '')
        + (on('mp') ? bar('mp', 42, { label: 'MP', value: '412' }) : '')
        + (on('tp') ? tpRow : '');
      vitals = cells ? `<div class="hbars">${cells}</div>` : '';
    } else {
      const cells = (on('hp') ? bar('hp', 100, { label: 'HP', value: '1188 / 1188' }) : '')
        + (on('mp') ? bar('mp', 42, { label: 'MP', value: '412 / 980' }) : '')
        + (on('tp') ? tpBar : '');
      vitals = cells ? `<div class="sbars">${cells}</div>` : '';
    }
    const main = /* html */ `
      <div class="hub-main bars-${barsMode}">
        ${idHtml}
        ${vitals}
        ${on('cast') ? selfBlock : ''}
        ${gilrow}
      </div>`;
    const gear = equip === 'off' ? ''
      : `<div class="hub-gear"><div class="eqgrid">${GEAR.map(eqCell).join('')}</div></div>`;
    const inner = equip === 'left' ? gear + main : main + gear;
    // buffs = rangee PLEINE LARGEUR sous le haut (stats + equipement) -> toute la largeur de la box.
    const buffrow = on('buffs') ? `<div class="hub-buffrow">${buffsTray()}</div>` : '';
    return `<div class="hub-wrap eq-${equip}"><div class="hub-top">${inner}</div>${buffrow}</div>`;
  },
});
