import { definePanel } from '../app/registry.js';
import { clamp, num } from '../core/util.js';
import { member, memberFields } from './partyCore.js';

// Alliances — XivParty listes index 1 et 2 (box SEPAREES, empilees AU-DESSUS de la party).
// 0 a 6 membres chacune. Le jeu ne transmet ni buffs ni actions pour l'alliance -> HP/MP/TP seuls.
// Le 1er membre de chaque liste porte la couronne de chef d'alliance. Reglages : double-clic.

// 12 membres d'alliance (2 x 6). alliance:true -> rendu sans buffs/cast.
const ALLY = [
  ['Tifa', 'SAM', 'WAR', 'dd'], ['Tidus', 'GEO', 'WHM', 'support'], ['Barret', 'DRK', 'SAM', 'dd'],
  ['Rinoa', 'SCH', 'RDM', 'healer'], ['Wakka', 'MNK', 'WAR', 'dd'], ['Zell', 'RNG', 'NIN', 'dd'],
  ['Vincent', 'BRD', 'WHM', 'support'], ['Quistis', 'PLD', 'RUN', 'tank'], ['Kimahri', 'SMN', 'SCH', 'support'],
  ['Yuffie', 'DRG', 'SAM', 'dd'], ['Irvine', 'BLU', 'NIN', 'dd'], ['Jecht', 'RUN', 'BLU', 'tank'],
].map(([name, job, , role], i) => ({
  name, job, role, alliance: true, // PAS de sub job : inconnu pour l'alliance (paquet 0x0DD = party only)
  maxHp: 1080 + i * 35, maxMp: 700 + i * 45,
  hp: 55 + (i * 13) % 45, mp: 35 + (i * 17) % 60, tp: ((i + 1) * 410) % 3001,
}));

const cssUrl = new URL('./party.css', import.meta.url).href; // reutilise le style .pm (dedupe par href)

function defineAlliance(id, title, members, pos) {
  members[0].leader = true; // chef d'alliance (couronne dediee)
  definePanel({
    id, title, type: 'AllianceList', css: cssUrl, pos,
    config: [
      { key: 'count', label: 'Membres (0-6)', type: 'number', min: 0, max: 6, default: members.length },
      ...members.flatMap(memberFields),
    ],
    render: (cfg = {}) =>
      members.slice(0, clamp(num(cfg.count, members.length), 0, 6)).map((m, i) => member(m, i, cfg)).join(''),
  });
}

// XivParty : alliance 1 au-dessus, alliance 2 au milieu, ta party en bas (colonne de droite).
// w: 250 -> meme largeur que la party (a effectif egal, meme hauteur aussi).
defineAlliance('alliance1', 'Alliance 1', ALLY.slice(0, 6), { x: 66, y: 24, w: 315 });
defineAlliance('alliance2', 'Alliance 2', ALLY.slice(6, 12), { x: 66, y: 41, w: 315 });
