import { definePanel } from '../app/registry.js';
import { asset, moveTypeIcon } from '../core/config.js';
import { num } from '../core/util.js';
import { PETS, DEFAULT_PET } from './bstPetData.js';

// BST Pet — calque fidele de l'addon AioHUD (boxes/bsthud_box.lua) :
// nom (Impact) + famille + jauge distance Command/Out + bloc role/+pro/-con/Strong-Weak
// + fioles HP/TP + Ready Moves (n° + icone type + nom + pips de cout).
// DOUBLE-CLIC : choisir le PET (toute la base AioHUD) + afficher/masquer chaque section
// (toggles on/off : show_dist, show_role, show_pro, show_con, show_eco, show_moves).

const DIST_SCALE = 10; // echelle de la jauge de distance (0..10)

// etat "demo" (runtime, identique quel que soit le pet selectionne)
const STATE = { hpp: 78, tp: 1500, dist: 3.4, cmd: 5, charges: 2, next: 47 }; // next = recast (s) jusqu'a la prochaine charge

const pips = (cost, ok) =>
  `<span class="move-pips">${Array.from({ length: cost }, () => `<i class="${ok ? 'on' : ''}"></i>`).join('')}</span>`;

// charges = EMPREINTES DE PATTE (theme BST : l'animal charme qu'on commande). Patte pleine = un ordre
// "Ready" dispo ; la suivante se REMPLIT en or au rythme du recast (duree = secondes) -> pas de texte "47s".
const chargePips = (have, max, recast) =>
  `<span class="charge-cmd">${Array.from({ length: max }, (_, k) =>
    k < have ? '<span class="cc on"><i></i></span>'
      : (k === have ? `<span class="cc charging" style="--dur:${recast}s"><i></i></span>` : '<span class="cc"><i></i></span>')).join('')}</span>`;

// items d'une grille UNIQUE (.pet-moves) : la colonne du nom est fixe (= plus long move BST) -> pips alignes
const moveRow = (m, i) => `
  <span class="move-num">${i + 1}</span>
  <img class="move-icon" src="${moveTypeIcon(m.type) || asset(`bst/rdy_${m.type}.png`)}">
  <span class="move-name ${m.ok ? 'ok' : 'no'}">${m.name}</span>
  ${pips(m.cost, m.ok)}
  ${m.sc ? `<span class="move-sc ${m.sc.el}">${m.sc.name}</span>` : '<span class="move-sc"></span>'}`;

const distGauge = (d, cmd) => {
  const cmdPct = Math.min(100, cmd / DIST_SCALE * 100);
  const curPct = Math.min(100, d / DIST_SCALE * 100);
  const inCmd = d <= cmd;
  const outMid = (cmd + DIST_SCALE) / 2 / DIST_SCALE * 100;
  return `
    <div class="petdist">
      <div class="petdist-tube">
        <div class="petdist-zone" style="width:${cmdPct}%"></div>
        <div class="petdist-cursor ${inCmd ? 'in' : 'out'}" style="left:${curPct}%"></div>
        <div class="petdist-val ${inCmd ? 'in' : 'out'}">${d.toFixed(2)}</div>
      </div>
      <div class="petdist-labels">
        <span class="petdist-cmd" style="left:${cmdPct / 2}%">Command</span>
        <span class="petdist-out" style="left:${outMid}%">Out</span>
      </div>
    </div>`;
};

const petBar = (kind, pct, caption, capCls = '') =>
  `<div class="bar ${kind} petbar"><i style="width:${pct}%"></i><span class="petbar-cap ${capCls}">${caption}</span></div>`;

// toggles d'affichage (double-clic) = memes options que l'addon AioHUD BST
const SHOW = [
  { key: 'show_dist', label: 'Distance', def: 1 },
  { key: 'show_role', label: 'Rôle', def: 1 },
  { key: 'show_pro', label: 'Points forts (+)', def: 1 },
  { key: 'show_con', label: 'Points faibles (-)', def: 1 },
  { key: 'show_eco', label: 'Strong / Weak', def: 1 },
  { key: 'show_moves', label: 'Ready Moves', def: 1 },
];

definePanel({
  id: 'bst',
  title: 'Pet',
  type: 'BstPet',
  css: new URL('./bstPet.css', import.meta.url).href,
  pos: { x: 1.5, y: 33, w: 286 },
  jobs: ['BST'], // affichee seulement si BST en main ou sub
  config: [
    { key: 'pet', label: 'Pet', type: 'select', default: DEFAULT_PET, options: PETS.map((p) => p.n) },
    ...SHOW.map((s) => ({ key: s.key, label: s.label, type: 'toggle', default: String(s.def) })),
  ],
  render: (cfg = {}) => {
    const p = PETS.find((x) => x.n === (cfg.pet || DEFAULT_PET)) || PETS[0];
    const s = STATE;
    const on = (k) => num(cfg[k], 1) !== 0;
    const moves = p.moves.map((m) => ({ ...m, ok: m.cost <= s.charges }));
    return /* html */ `
      <div class="pet-name">${p.n}</div>
      <div class="pet-fam">${p.eco}</div>
      ${on('show_dist') ? distGauge(s.dist, s.cmd) : ''}
      ${on('show_role') ? `<div class="pet-role">${p.role}${p.adj ? `<span class="pet-adj">${p.adj}</span>` : ''}</div>` : ''}
      ${on('show_pro') ? `<div class="pet-pro">+ ${p.pro}</div>` : ''}
      ${on('show_con') ? `<div class="pet-con">- ${p.con}</div>` : ''}
      ${on('show_eco') && p.strong ? `<div class="pet-eco"><span class="lbl">Strong:</span> <span class="s">${p.strong}</span>&nbsp;&nbsp;<span class="lbl">Weak:</span> <span class="w">${p.weak}</span></div>` : ''}
      <div class="pet-sep"></div>
      ${petBar('hp', s.hpp, `HP  ${s.hpp}%`)}
      ${petBar('tp', s.tp / 3000 * 100, `TP  ${s.tp}`, s.tp >= 1000 ? 'hi' : '')}
      ${on('show_moves') ? `
        <div class="pet-sep"></div>
        <div class="pet-moves-title">Ready Moves</div>
        <div class="pet-charges">${chargePips(s.charges, 3, s.next)}</div>
        ${moves.length ? `<div class="pet-moves">${moves.map(moveRow).join('')}</div>` : '<div class="pet-nomoves">— aucun ready move —</div>'}` : ''}`;
  },
});
