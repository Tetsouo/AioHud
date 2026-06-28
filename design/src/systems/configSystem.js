/**
 * Configuration des box : en Mode edition, DOUBLE-CLIC sur une box qui declare un schema
 * `config` -> ouvre un MODAL CENTRAL (voile sombre sur le reste, la box editee reste visible
 * au-dessus pour l'apercu live). Les sections (ex: chaque joueur de la party) sont des
 * groupes PLIABLES (<details>) -> menu compact meme avec beaucoup de reglages.
 * Le choix est persiste (config-store) et la box est re-rendue en direct.
 */
import { panels } from '../app/registry.js';
import { icon } from '../core/icons.js';
import { setConfig, resolveConfig } from '../core/config-store.js';

let rerender = null;
let modal = null, backdrop = null, raised = null;

export function initConfigSystem(opts = {}) {
  rerender = opts.rerender;
  document.addEventListener('dblclick', onDblClick);
  document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
}

const defOf = (id) => panels().find((p) => p.id === id);

function onDblClick(e) {
  if (!document.body.classList.contains('editing')) return;
  const el = e.target.closest('.panel');
  if (!el) return;
  const def = defOf(el.id);
  if (!def || !def.config || !def.config.length) return; // box sans reglages
  e.preventDefault();
  open(def, el);
}

// un champ reglable (nombre ou liste deroulante)
function field(f, value) {
  if (f.type === 'number') {
    const a = [`data-key="${f.key}"`, `value="${value ?? ''}"`, `step="${f.step ?? 1}"`];
    if (f.min != null) a.push(`min="${f.min}"`);
    if (f.max != null) a.push(`max="${f.max}"`);
    return `<label class="cfg-field"><span>${f.label}</span><input type="number" ${a.join(' ')}></label>`;
  }
  if (f.type === 'select') {
    const opts = f.options.map((o) => {
      const v = o.value ?? o, l = o.label ?? o;
      return `<option value="${v}"${v === value ? ' selected' : ''}>${l}</option>`;
    }).join('');
    return `<label class="cfg-field"><span>${f.label}</span><select data-key="${f.key}">${opts}</select></label>`;
  }
  if (f.type === 'toggle') {
    // interrupteur on/off (bouton) au lieu d'une liste Affiche/Masque
    const on = String(value ?? f.default ?? '1') !== '0';
    return `<div class="cfg-field"><span>${f.label}</span>`
      + `<button type="button" class="cfg-toggle${on ? ' on' : ''}" role="switch" aria-checked="${on}" data-key="${f.key}" data-val="${on ? '1' : '0'}"><i class="cfg-knob"></i></button></div>`;
  }
  return '';
}

// regroupe le schema : chaque 'section' ouvre un groupe pliable ; les champs avant toute
// section (ou les box sans section) forment un groupe sans entete.
function groupsOf(def, cfg) {
  const groups = [];
  let cur = { label: null, html: '' };
  for (const f of def.config) {
    if (f.type === 'section') {
      if (cur.label || cur.html) groups.push(cur);
      cur = { label: f.label, html: '' };
    } else {
      cur.html += field(f, cfg[f.key]);
    }
  }
  if (cur.label || cur.html) groups.push(cur);
  return groups;
}

function open(def, el) {
  close();
  const cfg = resolveConfig(def);
  const body = groupsOf(def, cfg).map((g) => g.label
    ? `<details class="cfg-group"><summary>${g.label}</summary><div class="cfg-fields">${g.html}</div></details>`
    : `<div class="cfg-fields">${g.html}</div>`).join('');

  backdrop = document.createElement('div');
  backdrop.className = 'cfg-backdrop';
  backdrop.addEventListener('mousedown', close);
  document.body.appendChild(backdrop);

  modal = document.createElement('div');
  modal.className = 'cfg-modal';
  modal.innerHTML =
    `<div class="cfg-head"><span>${def.title} — réglages</span>`
    + `<button class="cfg-close" type="button" title="Fermer (Échap)">${icon('close', 16)}</button></div>`
    + `<div class="cfg-body">${body}</div>`;
  document.body.appendChild(modal);
  modal.querySelector('.cfg-close').addEventListener('click', close);

  // la box editee reste VISIBLE au-dessus du voile (apercu live des changements)
  raised = el;
  el.dataset.cfgRaised = '1';

  modal.querySelectorAll('[data-key]').forEach((inp) => {
    const apply = () => { setConfig(def.id, { [inp.dataset.key]: inp.dataset.val ?? inp.value }); rerender?.(def.id); def.onChange && def.onChange(); };
    if (inp.tagName === 'BUTTON') {                 // toggle on/off
      inp.addEventListener('click', () => {
        const on = inp.dataset.val !== '0';
        inp.dataset.val = on ? '0' : '1';
        inp.classList.toggle('on', !on);
        inp.setAttribute('aria-checked', String(!on));
        apply();
      });
    } else if (inp.tagName === 'SELECT') inp.addEventListener('change', apply);
    else inp.addEventListener('input', apply);     // number : live
  });
}

function close() {
  if (modal) { modal.remove(); modal = null; }
  if (backdrop) { backdrop.remove(); backdrop = null; }
  if (raised) { delete raised.dataset.cfgRaised; raised = null; }
}
