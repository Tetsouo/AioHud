// Base des jug pets BST — extraite de l'addon AioHUD (data/tables/bst_petinfo.lua +
// BST_JUG_PETS_DATABASE.md), types de move croises avec bst_moves.lua (assets bst/rdy_<type>.png).
// strong/weak derives de l'eco via la vraie roue d'ecosysteme FFXI (AioHUD CORR).

const CORR = {
  Aquan: ['Amorph', 'Bird'], Amorph: ['Bird', 'Aquan'], Bird: ['Aquan', 'Amorph'],
  Beast: ['Lizard', 'Plantoid'], Lizard: ['Vermin', 'Beast'], Vermin: ['Plantoid', 'Lizard'],
  Plantoid: ['Beast', 'Vermin'],
};

// [name, eco, role, adj, pro, con, moves]  ;  move = [type, name, cost]
const RAW = [
  ['Pondering Peter', 'Beast', 'Utility', '', 'Wild Carrot: party AoE heal', 'Modest stats, no offense',
    [['slashing', 'Foot Kick', 1], ['earth', 'Dust Cloud', 1], ['piercing', 'Whirl Claws', 1], ['buff', 'Wild Carrot', 2]]],
  ['Aged Angus', 'Aquan', 'Tank', 'ATK-10 DEF+20', 'Shared crab defense buffs', 'Low attack',
    [['water', 'Bubble Shower', 1], ['buff', 'Bubble Curtain', 3], ['slashing', 'Big Scissors', 1], ['buff', 'Scissor Guard', 2], ['buff', 'Metallic Body', 1]]],
  ['Warlike Patrick', 'Lizard', 'DPS', 'ATK+30 DEF-20', 'Cheap DPS, Tail Blow stun', 'Fragile defense',
    [['blunt', 'Tail Blow', 1], ['fire', 'Fireball', 1], ['blunt', 'Blockhead', 1], ['blunt', 'Brain Crush', 1], ['debuff', 'Infrasonics', 2], ['buff', 'Secretion', 1]]],
  ['Bouncing Bertha', 'Vermin', 'DPS/SC', 'ATK+20', 'Solid DPS, strong SC opener', 'Few moves, no utility',
    [['slashing', 'Sensilla Blades', 1], ['slashing', 'Tegmina Buffet', 2]]],
  ['Rhyming Shizuna', 'Beast', 'CC', 'ATK+10 DEF+30', 'Sheep Song sleep AoE; Rage ATK+60', 'Rage cuts DEF -50%',
    [['blunt', 'Lamb Chop', 1], ['buff', 'Rage', 2], ['blunt', 'Sheep Charge', 1], ['debuff', 'Sheep Song', 2]]],
  ['Swooping Zhivago', 'Bird', 'Utility', 'ATK-10', 'Dispel + Amnesia (Pentapeck)', 'Low attack',
    [['wind', 'Molting Plumage', 1], ['piercing', 'Swooping Frenzy', 2], ['piercing', 'Pentapeck', 3]]],
  ['Amiable Roche', 'Aquan', 'DPS', 'ATK+40 DEF-10', 'Huge ATK+40; Intimidate slow', 'Low HP, few attacks',
    [['debuff', 'Intimidate', 2], ['blunt', 'Recoil Dive', 1], ['buff', 'Water Wall', 3]]],
  ['Herald Henry', 'Aquan', 'Tank', 'ATK-10 DEF+20', 'Full crab def kit (plasm)', 'Low attack',
    [['water', 'Bubble Shower', 1], ['buff', 'Bubble Curtain', 3], ['slashing', 'Big Scissors', 1], ['buff', 'Scissor Guard', 2], ['buff', 'Metallic Body', 1]]],
  ['Brainy Waluis', 'Plantoid', 'Debuff', '', 'Huge debuff kit (para/poison/silence)', 'Costly moves, mid DPS',
    [['blunt', 'Frogkick', 1], ['debuff', 'Spore', 1], ['dark', 'Queasyshroom', 2], ['dark', 'Numbshroom', 2], ['dark', 'Shakeshroom', 2], ['dark', 'Silence Gas', 3], ['dark', 'Dark Spore', 3]]],
  ['Suspicious Alice', 'Lizard', 'Utility', '', 'Numbing Noise stun; Geist Wall dispel', 'Low TP per hit',
    [['slashing', 'Nimble Snap', 1], ['blunt', 'Cyclotail', 1], ['debuff', 'Geist Wall', 1], ['debuff', 'Numbing Noise', 1], ['debuff', 'Toxic Spit', 2]]],
  ['Headbreaker Ken', 'Vermin', 'Magic', 'MAB+30', 'Venom: Water magic burst', 'Low HP, small kit',
    [['magic', 'Cursed Sphere', 1], ['water', 'Venom', 1], ['blunt', 'Somersault', 1]]],
  ['Alluring Honey', 'Plantoid', 'Tank', 'DEF+30', 'Anti-magic tank; 3 AoE MB debuffs', 'Mid physical damage',
    [['blunt', 'Tickling Tendrils', 1], ['earth', 'Stink Bomb', 2], ['water', 'Nectarous Deluge', 2], ['water', 'Nepenthic Plunge', 3]]],
  ['Vivacious Vickie', 'Beast', 'Support', 'ATK+10 DEF-20', 'Zealous Snort: Haste+25 shared', 'Only 2 moves, low def',
    [['blunt', 'Sweeping Gouge', 1], ['buff', 'Zealous Snort', 3]]],
  ['Anklebiter Jedd', 'Vermin', 'DPS', 'ATK+30', 'High DPS; Filamented Hold Slow AoE', 'Low HP',
    [['blunt', 'Double Claw', 1], ['blunt', 'Grapple', 1], ['blunt', 'Spinning Top', 1], ['debuff', 'Filamented Hold', 2]]],
  ['Hurler Percival', 'Vermin', 'Tank', 'ATK+20', 'Fast TP (95/hit); Spoil STR-20', 'Utility, no burst',
    [['blunt', 'Power Attack', 1], ['debuff', 'Hi-Freq Field', 2], ['piercing', 'Rhino Attack', 1], ['buff', 'Rhino Guard', 1], ['debuff', 'Spoil', 1]]],
  ['Blackbeard Randy', 'Beast', 'DPS', 'ATK+60 DEF-10', 'Top DPS; Roar para AoE; Dispel', 'Low HP, fragile',
    [['debuff', 'Roar', 2], ['piercing', 'Razor Fang', 1], ['slashing', 'Claw Cyclone', 1], ['slashing', 'Crossthrash', 2], ['debuff', 'Predatory Glare', 2]]],
  ['Fleet Reinhard', 'Lizard', 'DPS/SC', 'ATK+30 DEF-10', 'Good DPS; rare Darkness opener', 'Low HP, fragile',
    [['blunt', 'Scythe Tail', 1], ['slashing', 'Ripper Fang', 1], ['slashing', 'Chomp Rush', 3]]],
  ['Choral Leera', 'Bird', 'DPS', '', 'Huge ACC; NIN shadows; 4-hit flurry', 'One move, no utility',
    [['piercing', 'Pecking Flurry', 1]]],
  ['Gussy Hachirobe', 'Vermin', 'DPS', 'ATK+30 DEF-10', 'Sickle Slash crit; Spider Web Slow', 'Fragile defense',
    [['blunt', 'Sickle Slash', 1], ['water', 'Acid Spray', 1], ['debuff', 'Spider Web', 2]]],
  ['Cursed Annabelle', 'Vermin', 'Debuff', 'DEF+30', 'Sandpit Bind MB; AoE debuffs', 'Mid damage',
    [['slashing', 'Mandibular Bite', 1], ['magic', 'Sandblast', 2], ['magic', 'Sandpit', 1], ['debuff', 'Venom Spray', 2]]],
  ['Submerged Iyo', 'Bird', 'SC/Tank', '', 'Fast Ready charges; magic-tanky', 'Low attack',
    [['blunt', 'Wing Slap', 2], ['blunt', 'Beak Lunge', 1]]],
  ['Threestar Lynn', 'Vermin', 'TH farm', 'DEF-10', 'TH+1; huge EVA; ACC/ATK debuffs', 'Very low HP/ATK',
    [['blunt', 'Sudden Lunge', 1], ['slashing', 'Spiral Spin', 1], ['debuff', 'Noisome Powder', 2]]],
  ['Generous Arthur', 'Amorph', 'Debuff', 'ATK-20 DEF+30', 'Corrosive Ooze ATK/DEF-33; tanky', 'Low ATK, costly moves',
    [['water', 'Purulent Ooze', 2], ['water', 'Corrosive Ooze', 3]]],
  ['Brave Hero Glenn', 'Aquan', 'None', 'ATK-30 DEF-30', 'Collection / meme pet', 'NO ready moves, awful stats',
    []],
  ['Sharpwit Hermes', 'Plantoid', 'Debuff', '', 'Blunt damage; varied debuffs', 'Very low TP per hit',
    [['blunt', 'Head Butt', 1], ['piercing', 'Wild Oats', 1], ['slashing', 'Leaf Dagger', 1], ['debuff', 'Scream', 1]]],
  ['Fluffy Bredo', 'Amorph', 'Hybrid', 'ATK+30 MAB+40', 'Hybrid DPS+MAB; Pestilent Plume', 'No skillchain',
    [['water', 'Foul Waters', 2], ['dark', 'Pestilent Plume', 2]]],
  ['Left-Handed Yoko', 'Vermin', 'Evasion', 'ATK-30', 'Monster EVA tank; drain+Plague', 'Very low attack',
    [['magic', 'Infected Leech', 1], ['dark', 'Gloom Spray', 2]]],
  ['Stalwart Angelina', 'Vermin', 'TH farm', 'DEF-20', 'TH+1 + crit; highest EVA', 'Low HP and defense',
    [['slashing', 'Disembowel', 1], ['blunt', 'Extirpating Salvo', 2]]],
  ['Sweet Caroline', 'Plantoid', 'Debuff', 'DEF-20', 'Hermes moves, buyable', 'Lowest TP/hit; Hermes better',
    [['blunt', 'Head Butt', 1], ['piercing', 'Wild Oats', 1], ['slashing', 'Leaf Dagger', 1], ['debuff', 'Scream', 1]]],
  ['Jovial Edwin', 'Aquan', 'Tank', 'DEF+40', 'Top DEF; full shared def kit', 'Modest attack',
    [['buff', 'Bubble Curtain', 3], ['buff', 'Scissor Guard', 2], ['buff', 'Metallic Body', 1], ['water', 'Venom Shower', 2], ['slashing', 'Mega Scissors', 2]]],
  ['Energized Sefina', 'Vermin', 'Hybrid', 'ATK+20 DEF+40', 'DPS+tank hybrid; fast TP (95/hit)', 'Nothing notable',
    [['blunt', 'Power Attack', 1], ['debuff', 'Hi-Freq Field', 2], ['piercing', 'Rhino Attack', 1], ['buff', 'Rhino Guard', 1], ['debuff', 'Spoil', 1], ['piercing', 'Rhinowrecker', 2]]],
  ['Vivacious Gaston', 'Beast', 'Support', 'MAB+30', 'Frenzied Rage ATK+25 (9min, 1chg)', 'No skillchain',
    [['debuff', 'Chaotic Eye', 1], ['debuff', 'Blaster', 2], ['lightning', 'Charged Whisker', 2], ['buff', 'Frenzied Rage', 1]]],
  ['Daring Roland', 'Bird', 'TH/DPS', 'ATK+30 DEF-10', 'Fantod x4 burst; TH+1; versatile', 'None - top all-rounder',
    [['blunt', 'Back Heel', 1], ['debuff', 'Jettatura', 3], ['slashing', 'Choke Breath', 1], ['buff', 'Fantod', 2], ['blunt', 'Hoof Volley', 3], ['debuff', 'Nihility Song', 1]]],
  ['Sultry Patrice', 'Amorph', 'Tank', 'ATK+30 DA+10', 'Ultimate phys tank; solid DPS', 'No magic resist, low HP',
    [['blunt', 'Fluid Toss', 1], ['blunt', 'Fluid Spread', 2], ['magic', 'Digest', 1]]],
];

// Skillchain ouvert par chaque ready move (source : AioHUD data/tables/bst_moves.lua -> ready_move_sc).
// "X / Y" = double propriete (on affiche la principale = la 1re). Les moves absents n'ouvrent pas de SC.
const SC_BY_MOVE = {
  'Foot Kick': 'Reverberation', 'Whirl Claws': 'Impaction', 'Big Scissors': 'Scission', 'Tail Blow': 'Impaction',
  'Blockhead': 'Reverberation', 'Brain Crush': 'Liquefaction', 'Sensilla Blades': 'Scission', 'Tegmina Buffet': 'Distortion / Detonation',
  'Lamb Chop': 'Impaction', 'Sheep Charge': 'Reverberation', 'Swooping Frenzy': 'Fusion / Reverberation', 'Pentapeck': 'Light / Distortion',
  'Recoil Dive': 'Transfixion', 'Frogkick': 'Compression', 'Somersault': 'Compression', 'Nimble Snap': 'Impaction',
  'Cyclotail': 'Impaction', 'Tickling Tendrils': 'Impaction', 'Sweeping Gouge': 'Induration', 'Double Claw': 'Liquefaction',
  'Grapple': 'Reverberation', 'Spinning Top': 'Impaction', 'Power Attack': 'Reverberation', 'Rhino Attack': 'Detonation',
  'Razor Fang': 'Impaction', 'Claw Cyclone': 'Scission', 'Crossthrash': 'Distortion / Detonation', 'Scythe Tail': 'Liquefaction',
  'Ripper Fang': 'Induration', 'Chomp Rush': 'Darkness / Gravitation', 'Pecking Flurry': 'Transfixion', 'Sickle Slash': 'Transfixion',
  'Mandibular Bite': 'Detonation', 'Wing Slap': 'Gravitation / Liquefaction', 'Beak Lunge': 'Scission', 'Sudden Lunge': 'Impaction',
  'Spiral Spin': 'Scission', 'Head Butt': 'Detonation', 'Wild Oats': 'Transfixion', 'Leaf Dagger': 'Scission',
  'Disembowel': 'Impaction', 'Extirpating Salvo': 'Fusion / Impaction', 'Mega Scissors': 'Gravitation / Scission', 'Rhinowrecker': 'Fusion / Transfixion',
  'Back Heel': 'Reverberation', 'Hoof Volley': 'Light / Fragmentation', 'Fluid Toss': 'Reverberation', 'Fluid Spread': 'Fragmentation / Transfixion',
};
// element (couleur) de chaque propriete de skillchain
const SC_ELEMENT = {
  Transfixion: 'light', Compression: 'dark', Liquefaction: 'fire', Scission: 'earth', Reverberation: 'water',
  Induration: 'ice', Impaction: 'lightning', Detonation: 'wind', Gravitation: 'earth', Distortion: 'ice',
  Fusion: 'fire', Fragmentation: 'lightning', Light: 'light', Darkness: 'dark',
};

export const PETS = RAW.map(([n, eco, role, adj, pro, con, mv]) => ({
  n, eco, role, adj, pro, con,
  strong: (CORR[eco] || [])[0] || null,
  weak: (CORR[eco] || [])[1] || null,
  moves: mv.map(([type, name, cost]) => {
    const sc = SC_BY_MOVE[name];
    const primary = sc ? sc.split(' / ')[0] : null;     // propriete principale (1re) pour double SC
    return { type, name, cost, sc: sc ? { name: primary, el: SC_ELEMENT[primary] || 'light' } : null };
  }),
}));

export const DEFAULT_PET = 'Brainy Waluis';
