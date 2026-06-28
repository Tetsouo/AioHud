import { definePanel } from '../app/registry.js';
import { getConfig } from '../core/config-store.js';
import { clamp, num } from '../core/util.js';
import { loop } from '../core/anim.js';
import { member, memberFields } from './partyCore.js';

// Affiche/masque les box d'alliance selon le reglage "En alliance" de la party (0 / 1 / 2).
function applyAllianceVisibility() {
  const n = num(getConfig('party').alliances, 2);
  const a1 = document.getElementById('alliance1');
  const a2 = document.getElementById('alliance2');
  if (a1) a1.style.display = n >= 1 ? '' : 'none';
  if (a2) a2.style.display = n >= 2 ? '' : 'none';
}

// Party — TA party (XivParty liste index 0) : 1 a 6 membres, infos completes
// (HP/MP/TP + buffs + actions). Les alliances sont des box separees (alliance.js), au-dessus.
// Nombre de membres + reglages par joueur : double-clic sur la box.

const PARTY = [
  { name: 'Cloud', job: 'PLD', sub: 'WAR', role: 'tank', leader: true, allianceLeader: true, qm: true, self: true, maxHp: 1188, maxMp: 980, hp: 100, mp: 42, tp: 3000, buffs: 3, cast: 'Flash' },
  { name: 'Yuna', job: 'RDM', sub: 'WHM', role: 'healer', maxHp: 1252, maxMp: 1220, hp: 70, mp: 73, tp: 1000, buffs: 2, cast: 'Haste II' },
  { name: 'Squall', job: 'WAR', sub: 'SAM', role: 'dd', maxHp: 1444, maxMp: 100, hp: 18, mp: 0, tp: 2000, buffs: 1 },
  { name: 'Aerith', job: 'WHM', sub: 'SCH', role: 'healer', maxHp: 1244, maxMp: 1166, hp: 45, mp: 60, tp: 300, buffs: 2, cast: 'Cure VI' },
  { name: 'Lulu', job: 'BLM', sub: 'RDM', role: 'dd', maxHp: 1256, maxMp: 1648, hp: 0, mp: 0, tp: 0, buffs: 0 },
  { name: 'Rikku', job: 'COR', sub: 'RNG', role: 'support', maxHp: 1253, maxMp: 1050, hp: 95, mp: 40, tp: 600, buffs: 0, cast: 'Chaos Roll' },
];

const cfgFields = [
  { key: 'alliances', label: 'En alliance', type: 'select', default: '2',
    options: [{ value: '0', label: 'Solo (party seule)' }, { value: '1', label: '1 alliance' }, { value: '2', label: '2 alliances' }] },
  { key: 'count', label: 'Membres (1-6)', type: 'number', min: 1, max: 6, default: 6 },
  ...PARTY.flatMap(memberFields),
];

definePanel({
  id: 'party',
  title: 'Party',
  type: 'PartyList',
  css: new URL('./party.css', import.meta.url).href,
  pos: { x: 66, y: 58, w: 315 }, // largeur calee sur le pire cas : nom 15 car. + marqueurs + badge + jauges
  config: cfgFields,
  onChange: applyAllianceVisibility, // double-clic party -> solo / 1 / 2 alliances : masque ou affiche les box
  render: (cfg = {}) => PARTY.slice(0, clamp(num(cfg.count, 6), 1, 6)).map((m, i) => member(m, i, cfg)).join(''),
});

// ----- DEMO : la SELECTION (cible) defile 1er -> dernier -> 1er, avec un pulse a chaque changement -----
let _sel = 0, _dir = 1;
function moveSelection() {
  const root = document.getElementById('party');
  if (!root) return;
  const rows = root.querySelectorAll('.pm');
  if (!rows.length) return;
  rows.forEach((r) => r.classList.remove('target'));
  if (_sel > rows.length - 1) _sel = rows.length - 1;
  rows[_sel].classList.add('target'); // rejoue le pulse de selection
  if (_sel + _dir < 0 || _sel + _dir > rows.length - 1) _dir = -_dir; // ping-pong
  _sel += _dir;
}
if (typeof document !== 'undefined') {
  moveSelection(); loop(moveSelection, { everyMs: 1400 }); // pause auto en Mode edition
  // applique l'etat "En alliance" au demarrage (attend que les box d'alliance existent)
  const tryApply = () => { if (document.getElementById('alliance1')) applyAllianceVisibility(); else requestAnimationFrame(tryApply); };
  requestAnimationFrame(tryApply);
}
