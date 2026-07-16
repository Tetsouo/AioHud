import { definePanel } from '../app/registry.js';
import { icon } from '../core/icons.js';

// Skillchains — calque fidele d'AioHUD (modules/skillchains.lua, render() + M.preview).
// Titre "Skillchains" (centre, contenu) + timer Go! + Step + en-tete [Nom] (elements)
// + liste des weaponskills : "Nom -> Lv.X Propriete" (propriete color-codee).
// Donnees = le sample officiel M.preview (chaine Radiance Lv.4, contenu maximal).

// couleurs des proprietes de skillchain + des elements (extraites des \cs() du sample)
const COL = {
  Fusion: 'rgb(255,102,102)', Distortion: 'rgb(51,153,255)', Darkness: 'rgb(140,140,255)',
  Gravitation: 'rgb(190,130,70)', Fragmentation: 'rgb(250,156,247)', Detonation: 'rgb(102,255,102)',
  Light: 'rgb(255,255,255)', Scission: 'rgb(205,140,70)', Impaction: 'rgb(255,0,255)',
  Transfixion: 'rgb(255,255,255)',
  Fire: 'rgb(255,80,80)', Wind: 'rgb(102,255,102)', Lightning: 'rgb(255,0,255)',
};
const col = (n) => COL[n] || '#fff';

const SC = {
  timer: '3.0',                    // fenetre de burst (Go!)
  step: 4, name: 'Radiance',
  elements: ['Fire', 'Wind', 'Lightning', 'Light'],
  ws: [
    ['Savage Blade', 2, 'Fusion'],
    ['Resolution', 2, 'Distortion'],
    ['Torcleaver', 3, 'Darkness'],
    ['Insurgency', 2, 'Gravitation'],
    ['Entropy', 2, 'Fragmentation'],
    ['Ground Strike', 2, 'Fusion'],
    ['Dimidiation', 2, 'Light'],
    ['Herculean Slash', 1, 'Detonation'],
    ['Upheaval', 1, 'Scission'],
    ['Impulse Drive', 1, 'Impaction'],
    ['Sickle Moon', 1, 'Transfixion'],
    ['Crescent Moon', 1, 'Scission'],
  ],
};

const wsRow = ([name, lv, prop]) =>
  `<div class="wsrow"><span class="wsn">${name}</span><span class="wsp">${icon('arrow', 9)}Lv.${lv} <b style="color:${col(prop)}">${prop}</b></span></div>`;

definePanel({
  id: 'sc',
  title: 'Skillchains',
  type: 'Skillchains',
  css: new URL('./skillchains.css', import.meta.url).href,
  pos: { x: 42.6, y: 7.0, w: 258 },
  render: (cfg = {}) => /* html */ `
    <div class="sctitle">Skillchains</div>
    <div class="scgo">Go!&nbsp;&nbsp;${SC.timer}</div>
    <div class="scstep">Step: ${SC.step} ${icon('arrow', 9)} ${SC.name}</div>
    <div class="schdr">[<span class="scname">${SC.name}</span>] (${SC.elements.map((n) => `<span style="color:${col(n)}">${n}</span>`).join(', ')})</div>
    ${SC.ws.map(wsRow).join('')}`,
});
