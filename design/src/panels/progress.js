import { definePanel } from '../app/registry.js';

// PointWatch — calque d'AioHUD (targetbar/pw_data.lua). UNE barre de progression selon le stade :
//   <99 -> XP (leveling) · 99 -> CP (capacity) · master -> ML (Master Level EP) -- choisie en DOUBLE-CLIC.
//   + Merits TOUJOURS. Couleurs/labels/format (pw_kf "6.8k") fideles. Valeurs = demo (edit preview).
// ML en orange vif (pas dore) -> ne se confond plus avec les bouchons dores des fioles
const COL = { xp: 'rgb(120,205,120)', cp: 'rgb(80,175,235)', ml: 'rgb(255,150,45)', mer: 'rgb(190,140,225)' };

// % de la barre = courant/max de la valeur affichee (sinon la jauge ment).
const PROG = {
  xp: { label: 'XP', col: COL.xp, pct: 85, val: '44k/52k', rate: '3.1k/h' },                    // 44/52
  cp: { label: 'CP', col: COL.cp, pct: 40, val: '2.2k/5.5k <span style="color:rgb(120,170,200)">JP1200</span>', rate: '1.2k/h' }, // 2.2/5.5 ; TNJP cap = 5.5k
  master: { label: 'ML30', col: COL.ml, pct: 68, val: '6.8k/10k EP', rate: '2.1k/h' },            // 6.8/10
};
// Merits : limit points courant/10k (10000 par merit) + POOL n/max — max = 75 (pas 15).
const MERITS = { label: 'Merits', col: COL.mer, pct: 80, val: '8k/10k <span style="color:rgb(170,140,200)">53/75</span>', rate: '' }; // 8/10

const row = (r) => `
    <div class="pw-item">
      <div class="pw-head">
        <span class="pw-label" style="color:${r.col}">${r.label}</span>
        <span class="pw-val">${r.val}</span>
        ${r.rate ? `<span class="pw-rate">${r.rate}</span>` : ''}
      </div>
      <div class="pw-bar"><i style="width:${r.pct}%;background:${r.col}"></i></div>
    </div>`;

definePanel({
  id: 'progress',
  title: 'PointWatch',
  type: 'PointWatch',
  css: new URL('./progress.css', import.meta.url).href,
  pos: { x: 1.5, y: 4 }, // pas de w -> auto-largeur
  // double-clic en Mode edition : stade de progression + disposition de la liste (verticale / horizontale)
  config: [
    {
      key: 'mode', label: 'Progression', type: 'select', default: 'master',
      options: [
        { value: 'xp', label: 'Leveling (XP)' },
        { value: 'cp', label: 'Capacity (CP)' },
        { value: 'master', label: 'Master (ML)' },
      ],
    },
    {
      key: 'layout', label: 'Disposition', type: 'select', default: 'vertical',
      options: [
        { value: 'vertical', label: 'Verticale' },
        { value: 'horizontal', label: 'Horizontale' },
      ],
    },
  ],
  render: (cfg = {}) => {
    const layout = cfg.layout === 'horizontal' ? 'horizontal' : 'vertical';
    return `<div class="pw-list ${layout}">${row(PROG[cfg.mode] || PROG.master)}${row(MERITS)}</div>`;
  },
});
