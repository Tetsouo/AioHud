/**
 * Orchestrateur : monte les box enregistrees, la barre d'edition, puis demarre
 * les systemes (zones interdites, drag & drop, config, live-reload).
 */
import { panels } from './registry.js';
import { mountEditbar } from './Editbar.js';
import { injectStylesheet, qs } from '../core/dom.js';
import { resolveConfig, loadConfigFile } from '../core/config-store.js';
import { loadVisibility, applyVisibility } from '../core/visibility.js';
import { loadJob } from '../core/job.js';
import { initDragSystem } from '../systems/dragSystem.js';
import { initZonesSystem } from '../systems/zonesSystem.js';
import { initConfigSystem } from '../systems/configSystem.js';
import { initLiveReload } from '../systems/liveReload.js';

const defById = {}; // pour le re-rendu apres changement de config

/** Construit l'element DOM d'une box a partir de sa definition. */
function buildPanel(def) {
  const node = document.createElement('div');
  // `bare` = box sans chrome (ni cadre ni titre), ex: le grimoire ScHud (image nue).
  node.className = def.bare ? 'panel bare' : 'panel';
  node.id = def.id;
  node.dataset.label = (def.title || def.id).replace(/<[^>]*>/g, '').trim(); // nom lisible (pastille edition)
  const pos = def.pos || {};
  if (pos.x != null) node.style.left = pos.x + '%';
  if (pos.y != null) node.style.top = pos.y + '%';
  // largeur fixe si pos.w, sinon la box s'adapte a son contenu (max-content)
  node.style.width = (pos.w != null) ? pos.w + 'px' : 'max-content';
  const title = def.bare ? '' :
    `<div class="title"><span class="grip"></span> ${def.title} <span class="gear">⚙</span></div>`;
  node.innerHTML = `${title}<div class="body">${def.render(resolveConfig(def))}</div>`;
  // flag "box configurable" : l'indicateur ⚙ vit dans la pastille flottante (poschip),
  // JAMAIS dans la box -> la box ne change ni de forme ni de position entre les modes.
  if (def.config?.length) node.dataset.config = '1';
  // exigence de job (ex ['BST']) -> box affichee seulement si main/sub correspond (gere par visibility).
  if (def.jobs?.length) node.dataset.jobs = def.jobs.join(',');
  // ancrage par le HAUT force (ex: hate list) -> le haut reste fixe, le contenu change par le bas.
  if (def.growDown) node.dataset.growDown = '1';
  return node;
}

/** Re-rend le corps d'une box (apres un changement de config). */
export function rerenderPanel(id) {
  const def = defById[id];
  const body = document.getElementById(id)?.querySelector('.body');
  if (def && body) body.innerHTML = def.render(resolveConfig(def));
}

export async function startApp() {
  await loadConfigFile(); // config prete avant le premier rendu (pas de flash)
  await loadVisibility(); // etat affiche/masque pret avant le montage
  await loadJob();        // job courant (conditionne l'affichage de certaines box)

  const scene = qs('.scene');

  // conteneur des zones interdites (rempli par le systeme de zones)
  const zones = document.createElement('div');
  zones.id = 'zones';
  scene.appendChild(zones);

  // monte chaque box + injecte son CSS dedie
  const frag = document.createDocumentFragment();
  for (const def of panels()) {
    defById[def.id] = def;
    injectStylesheet(def.css);
    frag.appendChild(buildPanel(def));
  }
  scene.appendChild(frag);
  applyVisibility();        // masque les box choisies (apres montage) + gating par job

  // UI d'edition puis systemes (l'editbar doit exister avant les systemes qui le cablent)
  mountEditbar();
  initZonesSystem();
  initDragSystem();
  initConfigSystem({ rerender: rerenderPanel });
  initLiveReload();
}
