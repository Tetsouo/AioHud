import { definePanel } from '../app/registry.js';
import { timeRamp } from '../core/ramp.js';

// Zone Tracker — box INDEPENDANTE. Les 5 trackers de zone d'AioHUD, formates comme les
// PREVIEWS du mode Edit (sample_rows / demo_rows). Choix de la zone = DOUBLE-CLIC sur la box
// en Mode edition. En-tete oriente \cs(235,150,90).
// Sources : modules/omen.lua (sample_rows), modules/nyzulhelper.lua (demo_rows),
//           modules/sheolhelper.lua (demo_rows -> famille "Tiger" en Sheol B, resistances.lua).

const head = (t) => `<div class="zt-head">${t}</div>`;
const sub = (t, c) => `<div class="zt-sub"${c ? ` style="color:${c}"` : ''}>${t}</div>`;
const goals = (rows) => `<div class="zt-goals">${rows.map(([t, s]) => `<div class="zt-goal ${s || ''}">${t}</div>`).join('')}</div>`;
const GREEN = 'rgb(0,255,0)', RED = 'rgb(255,0,0)';

const timeBar = (text, pct) =>
  `<div class="zt-timebar"><i style="width:${Math.round(pct * 100)}%;background:${timeRamp(pct)}"></i><span>${text}</span></div>`;

// Abyssea : 7 lumieres en mini-barres verticales + compteur. CHAQUE lumiere a son propre cap :
// [abrev, nom, couleur, cap]. La barre se remplit en valeur / cap de cette lumiere.
const LIGHTS = [
  ['Pe', 'Pearlescent', 'rgb(235,235,245)', 230],
  ['Az', 'Azure', 'rgb(80,150,255)', 255],
  ['Ru', 'Ruby', 'rgb(230,60,60)', 255],
  ['Am', 'Amber', 'rgb(230,180,60)', 255],
  ['Go', 'Golden', 'rgb(255,215,100)', 200],
  ['Si', 'Silvery', 'rgb(180,190,200)', 200],
  ['Eb', 'Ebon', 'rgb(140,120,170)', 200],
];
const lights = (counts) => `<div class="zt-lights">${LIGHTS.map(([ab, name, c, max], i) => {
  const v = counts[i];
  return `<div class="zt-light" title="${name} (${v}/${max})"><span class="ztl-n" style="color:${c}">${ab}</span>` +
    `<div class="ztl-bar"><i style="height:${Math.round(Math.min(1, v / max) * 100)}%;background:${c}"></i></div>` +
    `<span class="ztl-v">${v}</span></div>`;
}).join('')}</div>`;

// Dynamis : 5 Key Items (vert = obtenu, rouge = manquant)
const KI = ['Crimson', 'Azure', 'Amber', 'Alabaster', 'Obsidian'];
const kiList = (got) => `<div class="zt-ki">${KI.map((n, i) =>
  `<div class="zt-kirow ${got[i] ? 'good' : 'bad'}">${got[i] ? '✓' : '✗'} ${n}</div>`).join('')}</div>`;

// Sheol (Tiger en Sheol B) : 3 armes (neutres) + 8 elements (carre colore + % colore par resistance)
const WEAPONS = [['Slashing', 100, 'var(--ink)'], ['Piercing', 100, 'var(--ink)'], ['Blunt', 100, 'var(--ink)']];
const ELES = [
  ['Fire', 'rgb(255,90,70)', 125, GREEN], ['Ice', 'rgb(140,225,255)', 88, RED],
  ['Wind', 'rgb(140,230,150)', 100, 'var(--ink)'], ['Earth', 'rgb(205,165,95)', 100, 'var(--ink)'],
  ['Thunder', 'rgb(200,140,245)', 125, GREEN], ['Water', 'rgb(90,160,255)', 100, 'var(--ink)'],
  ['Light', 'rgb(245,245,245)', 100, 'var(--ink)'], ['Dark', 'rgb(170,120,200)', 100, 'var(--ink)'],
];
const weapRow = ([n, v, c]) => `<span class="wn">${n}</span><span class="wv" style="color:${c}">${v}%</span>`;
const eleCell = ([n, sq, v, vc]) => `<div class="zt-ele" title="${n}"><i style="background:${sq}"></i><span style="color:${vc}">${v}%</span></div>`;

const VARIANTS = {
  omen: () => head('Omen') + sub('Defeat all enemies') + sub('Omens: 5  Bonus 04:32') + goals([
    ['1: WS Damage [15000/15000]', 'good'],
    ['2: Kills [8/8]', 'good'],
    ['3: MB Damage [12000/15000]'],
    ['4: Critical Hits [40/60]'],
    ['5: Melee Round [9500/12000]'],
    ['6: Abilities [3/10]'],
    ['7: Magic Bursts [2/5]', 'bad'],
    ['8: Spells [1/8]', 'bad'],
    ['9: Skillchains [2/3]'],
    ['10: All WS [4/6]'],
  ]),
  dynamis: () => head('Dynamis') + timeBar('58:30', 0.97) + kiList([true, true, false, false, true]),
  abyssea: () => head('Abyssea') + timeBar('18:42', 0.62) + lights([180, 240, 90, 200, 150, 60, 30]),
  nyzul: () => head('Nyzul Isle') + sub('Floor: 42') + sub('Time:  00:48', 'rgb(255,70,70)') + goals([
    ['Objective: <span style="color:rgb(255,250,120)">Defeat all enemies</span>'],
    ['Restriction: <span style="color:rgb(255,165,0)">No magic</span>'],
    ['Completed: 7'],
    ['Reward Rate: 110%'],
    ['Tokens: 1540'],
  ]),
  odyssey: () => head('Tiger') + sub('Segments: 1234') +
    `<div class="zt-res"><div class="zt-weap">${WEAPONS.map(weapRow).join('')}</div>` +
    `<div class="zt-eles">${ELES.map(eleCell).join('')}</div></div>` +
    `<div class="zt-joke" style="color:${GREEN}">Cruel Joke</div>`,
};

definePanel({
  id: 'zone',
  title: 'Zone Tracker',
  type: 'ZoneTracker',
  css: new URL('./zoneTracker.css', import.meta.url).href,
  pos: { x: 14.5, y: 33 }, // pas de w -> la box s'adapte a son contenu
  // double-clic sur la box en Mode edition -> choisir la zone
  config: [{
    key: 'variant', label: 'Zone', type: 'select', default: 'omen',
    options: [
      { value: 'omen', label: 'Omen' },
      { value: 'dynamis', label: 'Dynamis' },
      { value: 'abyssea', label: 'Abyssea' },
      { value: 'nyzul', label: 'Nyzul' },
      { value: 'odyssey', label: 'Odyssey' },
    ],
  }],
  render: (cfg = {}) => (VARIANTS[cfg.variant] || VARIANTS.omen)(),
});
