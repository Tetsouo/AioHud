/**
 * Barre d'edition (en haut). Gere les deux modes mutuellement exclusifs :
 *   - editing : deplacer les box (drag + snap + anti-chevauchement)
 *   - zoning  : editer les zones interdites (les box sont masquees)
 * Les boutons des zones sont cables par le systeme de zones (zonesSystem).
 */
import { el } from '../core/dom.js';
import { icon } from '../core/icons.js';
import { refreshChips, copyPositions, toggleGrid, isGridOn } from '../systems/dragSystem.js';
import { panels } from './registry.js';
import { isHidden, setHidden, applyVisibility } from '../core/visibility.js';
import { getJob, setJob, jobMatches, JOBS } from '../core/job.js';
import { buildLayout } from './exportLayout.js';
import { putJSON } from '../core/store.js';

// Visibilite des hints/boutons geree en CSS via les classes body.editing / body.zoning.
const TEMPLATE = /* html */ `
  <button class="btn" id="editBtn">${icon('settings')} Mode édition</button>
  <button class="btn" id="zonesBtn">${icon('block')} Zones jeu</button>
  <button class="btn" id="boxesBtn">${icon('list')} Boxes</button>
  <button class="btn" id="jobBtn">${icon('user')} <span id="jobLbl"></span></button>
  <button class="btn" id="exportBtn">${icon('download')} Export</button>
  <button class="btn" id="gridBtn">${icon('grid')} Grille</button>
  <button class="btn" id="copyBtn">${icon('copy')} Copier positions</button>
  <span id="zonebar">
    <button class="btn" id="zoneAddBtn">${icon('add')} Zone</button>
    <button class="btn" id="zoneExportBtn">${icon('copy')} Export zones</button>
    <button class="btn" id="zoneReloadBtn">${icon('refresh')} Fichier</button>
    <span class="hint">glisse pour déplacer · coin pour redimensionner · clic label pour renommer</span>
  </span>
  <span class="hint" id="editHint">${icon('move', 13)} glisser = déplacer · ${icon('block', 13)} zones rouges = interdites · ${icon('settings', 13)} doré = double-clic pour options</span>`;

export function mountEditbar() {
  const bar = el('div', { id: 'editbar' }, TEMPLATE);
  document.body.appendChild(bar);

  const body = document.body;
  const $ = (id) => bar.querySelector('#' + id);

  $('zonesBtn').addEventListener('click', () => {
    const on = body.classList.toggle('zoning');
    if (on) body.classList.remove('editing');
  });

  $('editBtn').addEventListener('click', () => {
    const on = body.classList.toggle('editing');
    if (on) { body.classList.remove('zoning'); refreshChips(); }
    else closeBoxesMenu();
  });

  $('copyBtn').addEventListener('click', copyPositions);

  // ---- grille (snap) -------------------------------------------------------
  const gridBtn = $('gridBtn');
  gridBtn.classList.toggle('active', isGridOn());
  gridBtn.addEventListener('click', () => gridBtn.classList.toggle('active', toggleGrid()));

  // ---- export du layout (descriptor JSON -> exports/layout.json + presse-papier) ----
  $('exportBtn').addEventListener('click', () => {
    const layout = buildLayout();
    putJSON('exports/layout.json', layout);
    navigator.clipboard?.writeText(JSON.stringify(layout, null, 2));
    console.log('[AioHUD] layout exporté → design/exports/layout.json (+ presse-papier)', layout);
    const b = $('exportBtn');
    b.classList.add('active');
    setTimeout(() => b.classList.remove('active'), 900);
  });

  // ---- menu Boxes : afficher / masquer chaque box --------------------------
  let menu = null;
  const closeBoxesMenu = () => { if (menu) { menu.remove(); menu = null; } };

  const openBoxesMenu = (btn) => {
    menu = el('div', { id: 'boxesmenu' });
    for (const def of panels()) {
      const label = (def.title || def.id).replace(/<[^>]*>/g, '').trim();
      const jobsCsv = def.jobs ? def.jobs.join(',') : '';
      const tag = def.jobs ? `<span class="jobtag">${def.jobs.join('/')}</span>` : '';
      const row = el('div', { class: 'boxrow' }, `<span class="boxchk"></span>${label}${tag}`);
      const sync = () => {
        row.classList.toggle('off', isHidden(def.id));               // masquee a la main (case vide)
        row.classList.toggle('jobgated', jobsCsv && !jobMatches(jobsCsv)); // inactive : job non concordant
      };
      sync();
      row.addEventListener('click', (e) => { e.stopPropagation(); setHidden(def.id, !isHidden(def.id)); sync(); });
      menu.appendChild(row);
    }
    document.body.appendChild(menu);
    const r = btn.getBoundingClientRect();
    menu.style.left = Math.round(r.left) + 'px';
    menu.style.top = Math.round(r.bottom + 5) + 'px';
  };

  // ---- menu Job : choisir main / sub job -----------------------------------
  let jobMenu = null;
  const closeJobMenu = () => { if (jobMenu) { jobMenu.remove(); jobMenu = null; } };
  const updateJobLbl = () => { const { main, sub } = getJob(); $('jobLbl').textContent = `${main}/${sub || '—'}`; };

  const openJobMenu = (btn) => {
    const { main, sub } = getJob();
    const opt = (j, sel) => `<option value="${j}"${j === sel ? ' selected' : ''}>${j}</option>`;
    jobMenu = el('div', { id: 'jobmenu' },
      `<label class="jobrow"><span>Main</span><select data-k="main">${JOBS.map((j) => opt(j, main)).join('')}</select></label>`
      + `<label class="jobrow"><span>Sub</span><select data-k="sub"><option value=""${sub ? '' : ' selected'}>—</option>${JOBS.map((j) => opt(j, sub)).join('')}</select></label>`);
    document.body.appendChild(jobMenu);
    const r = btn.getBoundingClientRect();
    jobMenu.style.left = Math.round(r.left) + 'px';
    jobMenu.style.top = Math.round(r.bottom + 5) + 'px';
    jobMenu.querySelectorAll('select').forEach((s) => s.addEventListener('change', () => {
      setJob({ [s.dataset.k]: s.value });
      applyVisibility();        // box gatees par job (Pet BST, Grimoire SCH...) apparaissent/disparaissent
      updateJobLbl();
    }));
  };

  $('boxesBtn').addEventListener('click', (e) => {
    e.stopPropagation(); closeJobMenu();
    if (menu) closeBoxesMenu(); else openBoxesMenu(e.currentTarget);
  });
  $('jobBtn').addEventListener('click', (e) => {
    e.stopPropagation(); closeBoxesMenu();
    if (jobMenu) closeJobMenu(); else openJobMenu(e.currentTarget);
  });
  updateJobLbl();

  // clic ailleurs -> ferme les menus (closest -> robuste si on clique l'icone dans le bouton)
  document.addEventListener('mousedown', (e) => {
    if (menu && !menu.contains(e.target) && !e.target.closest('#boxesBtn')) closeBoxesMenu();
    if (jobMenu && !jobMenu.contains(e.target) && !e.target.closest('#jobBtn')) closeJobMenu();
  });
}
