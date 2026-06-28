import { definePanel } from '../app/registry.js';
import { elemIcon } from '../core/config.js';
import { icon } from '../core/icons.js';
import { loop } from '../core/anim.js';

// Vana'diel + RADAR dans la meme box. Le JOUR elementaire colore le nom du jour ET la lune.
const ELEM = {
  Fire: '#ff7a4a', Earth: '#d8b06a', Water: '#5aa6ff', Wind: '#74e08a',
  Ice: '#a6e0ff', Lightning: '#c79bff', Light: '#fff2c0', Dark: '#b07cff',
};
// ordre du cycle des jours FFXI (-> jour suivant)
const DAYS_ORDER = ['Fire', 'Earth', 'Water', 'Wind', 'Ice', 'Lightning', 'Light', 'Dark'];
const DAYNAME = {
  Fire: 'Firesday', Earth: 'Earthsday', Water: 'Watersday', Wind: 'Windsday',
  Ice: 'Iceday', Lightning: 'Lightningday', Light: 'Lightsday', Dark: 'Darksday',
};
const PHASES = [
  ['New', 0], ['Waxing Crescent', 25], ['First Quarter', 50], ['Waxing Gibbous', 74],
  ['Full', 100], ['Waning Gibbous', 74], ['Last Quarter', 50], ['Waning Crescent', 25],
];
const pad = (n) => String(n).padStart(2, '0');

// nom de phase depuis le % eclaire + le sens (pour la lune animee)
function phaseName(pct, waning) {
  if (pct <= 2) return 'New';
  if (pct >= 98) return 'Full';
  if (pct >= 48 && pct <= 52) return waning ? 'Last Quarter' : 'First Quarter';
  const half = pct < 50 ? 'Crescent' : 'Gibbous';
  return (waning ? 'Waning ' : 'Waxing ') + half;
}

// Lune SVG : le DISQUE entier est dessine en permanence (crateres FIXES sur la surface, ne bougent
// JAMAIS) + un leger relief spherique fixe. La phase = une OMBRE (le terminateur) qui GLISSE par-dessus,
// comme la vraie lune. Plus aucun "flip" des taches au changement de jour. f = fraction eclairee (0..1),
// waning -> l'ombre est du cote droit (lune decroissante).
function moon(f, lit, waning) {
  const r = 11, c = 13;
  f = Math.max(0, Math.min(1, f));
  const rx = (r * Math.abs(1 - 2 * f)).toFixed(3); // demi-largeur du terminateur (0 au quartier, r a new/full)
  const gibbous = f > 0.5;
  // limbe sombre : decroissante -> a droite, croissante -> a gauche.
  const limbSweep = waning ? 1 : 0;
  // sens du terminateur (arcs SVG) : a new (f=0) l'ombre doit couvrir TOUT le disque, a full (f=1)
  // RIEN. croissant -> terminateur bombe vers la lumiere (ombre > moitie) ; gibbeux -> bombe vers l'ombre.
  const termSweep = gibbous ? (waning ? 0 : 1) : (waning ? 1 : 0);
  // region D'OMBRE (et non plus la region eclairee) : limbe sombre + ellipse du terminateur.
  const shadow = `M${c} ${c - r} A${r} ${r} 0 0 ${limbSweep} ${c} ${c + r} A${rx} ${r} 0 0 ${termSweep} ${c} ${c - r} Z`;
  const uid = `${Math.round(f * 1000)}${waning ? 'w' : ''}`; // ids uniques (plusieurs lunes possibles sur la page)
  const craters = '<circle cx="9.4" cy="8.8" r="2.4" fill="#000" opacity=".17"/>'
    + '<circle cx="15.6" cy="14.6" r="1.8" fill="#000" opacity=".14"/>'
    + '<circle cx="10.8" cy="16.6" r="1.3" fill="#000" opacity=".12"/>'
    + '<circle cx="16.6" cy="8.4" r="1.1" fill="#000" opacity=".11"/>'
    + '<circle cx="7.8" cy="13.4" r="1.5" fill="#000" opacity=".1"/>'
    + '<circle cx="13.2" cy="11.2" r="0.9" fill="#fff" opacity=".13"/>';
  return `<svg viewBox="0 0 26 26" class="moonsvg">`
    + `<defs><clipPath id="md${uid}"><circle cx="${c}" cy="${c}" r="${r}"/></clipPath>`
    // relief spherique FIXE (ne bascule plus avec la phase) : juste pour donner le volume.
    + `<radialGradient id="mh${uid}" cx="42%" cy="30%" r="80%">`
    + `<stop offset="0%" stop-color="#fff" stop-opacity=".42"/>`
    + `<stop offset="52%" stop-color="#fff" stop-opacity="0"/>`
    + `<stop offset="100%" stop-color="#000" stop-opacity=".30"/></radialGradient>`
    + `<filter id="mb${uid}" x="-20%" y="-20%" width="140%" height="140%"><feGaussianBlur stdDeviation=".55"/></filter></defs>`
    + `<g clip-path="url(#md${uid})">`
    + `<circle cx="${c}" cy="${c}" r="${r}" fill="${lit}"/>${craters}`        // disque eclaire + taches FIXES
    + `<rect x="2" y="2" width="22" height="22" fill="url(#mh${uid})"/>`       // relief 3D fixe
    + `<path d="${shadow}" fill="#0b1022" fill-opacity=".8" filter="url(#mb${uid})"/>` // ombre qui GLISSE (terminateur doux)
    + `</g>`
    + `<circle cx="${c}" cy="${c}" r="${r}" fill="none" stroke="rgba(190,205,255,.32)" stroke-width="1"/></svg>`;
}

// projette des entites [type, distance (yalms), cap relatif (deg, 0 = devant)] -> points radar.
// portee = ~2/3 du rayon (edge = portee*1.5) ; au-dela de la portee = non suivi (cache).
function projectDots(ents, rng) {
  const edge = rng * 1.5;
  return ents.map(([t, dist, brg]) => {
    if (dist > rng) return '';
    const f = dist / edge, a = brg * Math.PI / 180;
    return `<i class="rdot ${t}" style="left:${(50 + f * 50 * Math.sin(a)).toFixed(1)}%;top:${(50 - f * 50 * Math.cos(a)).toFixed(1)}%"></i>`;
  }).join('');
}

// construit TOUT le contenu de la box depuis un etat -> reutilise par le 1er rendu ET l'animation.
// st = { el, hh, mm, phasePct, waning, heading, ents:[[type,dist,brg]], range }
function buildClock(st) {
  const el = ELEM[st.el] ? st.el : 'Water';
  const color = ELEM[el];
  const nextEl = DAYS_ORDER[(DAYS_ORDER.indexOf(el) + 1) % DAYS_ORDER.length];
  const pname = phaseName(st.phasePct, st.waning);
  // jours Vana'diel avant la prochaine nouvelle / pleine lune (cycle FFXI = 84 jours, pleine au milieu=42).
  // position dans le cycle deduite du % eclaire + sens : 0/84 = nouvelle, 42 = pleine.
  const mf = Math.max(0, Math.min(1, st.phasePct / 100));
  const mpos = st.waning ? 84 - (42 / Math.PI) * Math.acos(1 - 2 * mf) : (42 / Math.PI) * Math.acos(1 - 2 * mf);
  const dToNew = Math.round(84 - mpos);                        // nouvelle lune = position 0/84
  const dToFull = Math.round((mpos < 42 ? 42 : 126) - mpos);   // pleine lune = position 42
  // si on EST deja dans la phase (new/full dure plusieurs jours) -> "now", sinon le compte a rebours.
  // evite l'incoherence "lune New mais next New = 1-2d".
  const newStr = pname === 'New' ? 'now' : `${dToNew}d`;
  const fullStr = pname === 'Full' ? 'now' : `${dToFull}d`;
  const dots = projectDots(st.ents, st.range);
  const rings = [st.range / 2, st.range].map((v) => {
    const w = (v / (st.range * 1.5) * 100).toFixed(1);
    return `<div class="rring" style="width:${w}%;height:${w}%"><span class="rring-lbl">${Math.round(v)}</span></div>`;
  }).join('');
  const now = new Date();
  return /* html */ `
      <div class="vt">${st.hh}:${st.mm}</div>
      <div class="vday">
        <img class="vel" src="${elemIcon(el)}" alt="">
        <span class="vd-cur" style="color:${color}">${DAYNAME[el]}</span>
        ${icon('arrow', 8)}
        <span class="vd-next" style="color:${ELEM[nextEl]}">${DAYNAME[nextEl]}</span>
      </div>
      <div class="moonrow">
        ${moon(st.phasePct / 100, color, st.waning)}
        <span class="vmoon">${st.phasePct}% ${pname}</span>
        <div class="vmnext">
          <span class="vm-dot new"></span> New <b>${newStr}</b>
          <span class="vdot">·</span>
          <span class="vm-dot full"></span> Full <b>${fullStr}</b>
        </div>
      </div>
      <div class="vsep"></div>
      <div class="vrt2"><span class="vrt-l">Réel</span> ${pad(now.getHours())}:${pad(now.getMinutes())} <span class="vdot">·</span> <span class="vrt-l">GMT</span> ${pad(now.getUTCHours())}:${pad(now.getUTCMinutes())}</div>
      <div class="vsep"></div>
      <div class="radar">
        <div class="radar-bezel" style="transform:rotate(${(-st.heading).toFixed(1)}deg)"><span class="rc n">N</span><span class="rc e">E</span><span class="rc s">S</span><span class="rc w">W</span></div>
        ${rings}
        <i class="radar-me"></i>
        ${dots}
      </div>`;
}

// entites relatives FIXES pour le 1er rendu (statique) : [type, distance (yalms), cap relatif].
const STATIC_ENTS = [
  ['mobA', 5, 15], ['mob', 18, 60], ['mob', 34, 145], ['pc', 7, 205], ['pc', 10, 240],
  ['npc', 24, 292], ['mob', 49, 100], ['mobA', 13, 325], ['pc', 16, 168], ['mob', 46, 22], ['mob', 48, 255],
];

definePanel({
  id: 'clock',
  title: "Vana'diel",
  type: 'VanaClock',
  css: new URL('./clock.css', import.meta.url).href,
  pos: { x: 69.6, y: 3.6, w: 186 }, // largeur FIXE (calee sur le jour le plus long : Lightningday -> Lightsday)
  config: [
    { key: 'day', label: 'Jour (élément)', type: 'select', default: 'Lightning', options: Object.keys(ELEM).map((e) => ({ value: e, label: DAYNAME[e] })) },
    { key: 'phase', label: 'Lune (phase)', type: 'select', default: 'Waxing Gibbous', options: PHASES.map(([n]) => n) },
    { key: 'range', label: 'Portée radar', type: 'select', default: '50', options: [{ value: '15', label: '15 yalms' }, { value: '30', label: '30 yalms' }, { value: '50', label: '50 yalms (max jeu)' }] },
  ],
  render: (cfg = {}) => {
    const el = ELEM[cfg.day] ? cfg.day : 'Water';
    const [pname, ppct] = PHASES.find(([n]) => n === cfg.phase) || PHASES[3];
    const waning = /Waning|Last Quarter/.test(pname);
    const range = (cfg.range && [15, 30, 50].includes(+cfg.range)) ? +cfg.range : 50;
    return buildClock({ el, hh: '14', mm: '32', phasePct: ppct, waning, heading: 35, ents: STATIC_ENTS, range });
  },
});

// =========================================================================================
// ANIMATION DEMO (preview maquette) : fait DEFILER l'heure Vana'diel + les jours (la lune passe par
// toutes les phases), et simule un JOUEUR EN DEPLACEMENT dans un donjon (il avance/tourne -> les
// entites bougent sur le radar et le compas de laiton pivote). En PAUSE pendant le Mode edition.
// =========================================================================================
if (typeof window !== 'undefined' && !window.__aioClockAnim) {
  window.__aioClockAnim = true;
  // entites du "donjon" : positions monde fixes (yalms) -> le joueur se balade au milieu.
  const WENTS = [
    ['mobA', 6, 12], ['mob', -22, 28], ['mob', 33, -12], ['pc', 4, -9], ['pc', -13, 6],
    ['npc', -28, -24], ['mob', 38, 22], ['mobA', -6, -33], ['mob', 20, 42], ['pc', 12, 16],
  ];
  const R2D = 180 / Math.PI;
  const startMin = 14 * 60 + 32;
  loop((f) => {
    const body = document.querySelector('#clock .body');
    if (!body) return;
    // --- heure Vana'diel qui defile (un jour ~ toutes les 9-10 s) + jour elementaire ---
    const vmin = startMin + f * 6;
    const dayIdx = Math.floor(vmin / 1440) % DAYS_ORDER.length;
    const hh = pad(Math.floor((vmin % 1440) / 60)), mm = pad(vmin % 60);
    // --- lune : cycle complet (new -> full -> new) en ~28 s ---
    // luminosite SINUSOIDALE (= meme modele que les compteurs New/Full -> moon visuel et compteurs synchro,
    // et plus realiste : eclairage lent pres de new/full, rapide aux quartiers).
    const mc = (f / 700) % 1;
    const illum = (1 - Math.cos(2 * Math.PI * mc)) / 2;
    const phasePct = Math.round(illum * 100);
    const waning = mc > 0.5;
    // --- deplacement joueur (chemin de Lissajous = balade bornee) + cap = direction du mouvement ---
    const px = 24 * Math.sin(f * 0.011), py = 24 * Math.sin(f * 0.017 + 1.2);
    const dpx = 0.011 * Math.cos(f * 0.011), dpy = 0.017 * Math.cos(f * 0.017 + 1.2);
    const heading = Math.atan2(dpx, dpy) * R2D;
    const ents = WENTS.map(([t, wx, wy]) => {
      const dx = wx - px, dy = wy - py;
      return [t, Math.hypot(dx, dy), Math.atan2(dx, dy) * R2D - heading];
    });
    body.innerHTML = buildClock({ el: DAYS_ORDER[dayIdx], hh, mm, phasePct, waning, heading, ents, range: 50 });
  });
}
