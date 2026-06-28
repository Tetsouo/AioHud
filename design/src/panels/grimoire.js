import { definePanel } from '../app/registry.js';
import { asset } from '../core/config.js';

// GRIMOIRE (Scholar) — box INDEPENDANTE (snappable librement). Affichee seulement si SCH main/sub
// (jobs -> systeme de visibilite). Texture HD du livre (Light/Dark) + numeros charge/recast en
// pastille + AURA "Addendum" codee et animee (remplace les anciennes couches -Add).

// >>> REGLAGE LIBRE des numeros (live-reload) : x/y en % de la texture (x: 0 gauche -> 100 droite,
//     y: 0 haut -> 100 bas), centres sur le point. GRIM_FONT = taille en px.
const GRIM_FONT = 14;
const GRIM_CHARGE = { x: 17, y: 78, text: '4' };   // charges (page de gauche)
const GRIM_TIMER = { x: 81, y: 78, text: '20' };   // recast  (page de droite)
const grimNum = (cls, p) =>
  `<span class="${cls}" style="left:${p.x}%;top:${p.y}%;font-size:${GRIM_FONT}px">${p.text}</span>`;

// 4 etats d'Arts : texture (Light/Dark) + classes (.addendum -> aura solaire). Choisi en config.
const ART = {
  'light': { cls: 'light', tex: 'Grimoire-Light-HD.png' },
  'dark': { cls: 'dark', tex: 'Grimoire-Dark-HD.png' },
  'light-add': { cls: 'light addendum', tex: 'Grimoire-Light-HD.png' },
  'dark-add': { cls: 'dark addendum', tex: 'Grimoire-Dark-HD.png' },
};

definePanel({
  id: 'schud',
  title: 'Grimoire',
  type: 'Grimoire',
  bare: true,                 // box sans chrome : juste le livre qui flotte
  css: new URL('./grimoire.css', import.meta.url).href,
  pos: { x: 28, y: 58 },
  jobs: ['SCH'],              // affichee seulement si SCH en main ou sub
  // double-clic (Mode edition) -> choisir l'Art (texture + aura)
  config: [{
    key: 'grim', label: 'Arts', type: 'select', default: 'light-add',
    options: [
      { value: 'light', label: 'Light Arts' },
      { value: 'dark', label: 'Dark Arts' },
      { value: 'light-add', label: 'Addendum: White' },
      { value: 'dark-add', label: 'Addendum: Black' },
    ],
  }],
  render: (cfg = {}) => {
    const a = ART[cfg.grim] || ART['light-add'];
    // .grimoire a un PADDING = la portee de l'aura -> la box (snap) inclut l'aura.
    // .grim-inner fait la taille du LIVRE -> les numeros (% ) restent cales dessus.
    return /* html */ `
      <div class="grimoire ${a.cls}">
        <div class="grim-aura"></div>
        <div class="grim-inner">
          <img class="grim-base" src="${asset(a.tex)}" alt="">
          <div class="grim-shine"></div>
          ${grimNum('grim-charge', GRIM_CHARGE)}
          ${grimNum('grim-timer', GRIM_TIMER)}
        </div>
      </div>`;
  },
});
