/* Page d'export : rend les box party/alliances isolees (fond transparent) pour capture PNG.
   ?mode=party | alliances | group   (group = alliance1 + alliance2 + party empilees avec jointures). */
import './src/panels/party.js';
import './src/panels/alliance.js';
import { panels } from './src/app/registry.js';
import { resolveConfig } from './src/core/config-store.js';
import { injectStylesheet } from './src/core/dom.js';

function buildBox(def) {
  const node = document.createElement('div');
  node.className = 'panel';
  node.id = def.id;
  node.style.width = (def.pos && def.pos.w != null) ? def.pos.w + 'px' : 'max-content';
  injectStylesheet(def.css);
  node.innerHTML = `<div class="body">${def.render(resolveConfig(def))}</div>`;
  return node;
}

const mode = new URLSearchParams(location.search).get('mode') || 'group';
const order = mode === 'party' ? ['party']
  : mode === 'alliances' ? ['alliance1', 'alliance2']
    : ['alliance1', 'alliance2', 'party'];

const root = document.getElementById('export');
order.forEach((id, i) => {
  const def = panels().find((p) => p.id === id);
  const node = buildBox(def);
  if (i > 0) node.classList.add('snap-top');                 // joint au-dessus (rempli par ::after)
  if (i < order.length - 1) node.classList.add('snap-bot');
  root.appendChild(node);
});

// taille exacte du contenu -> window-size de la capture
requestAnimationFrame(() => {
  const r = root.getBoundingClientRect();
  document.title = `READY ${Math.ceil(r.width)}x${Math.ceil(r.height)}`;
  document.body.dataset.ready = '1';
});
