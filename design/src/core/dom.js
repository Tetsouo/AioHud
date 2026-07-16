/** Petits helpers DOM, sans dependance. */

/** querySelector / querySelectorAll raccourcis. */
export const qs = (sel, root = document) => root.querySelector(sel);
export const qsa = (sel, root = document) => [...root.querySelectorAll(sel)];

/**
 * Cree un element avec attributs + enfants (string HTML ou Node).
 *   el('div', { class:'panel', id:'x' }, '<i></i>')
 */
export function el(tag, attrs = {}, html) {
  const node = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (v == null || v === false) continue;
    if (k === 'class') node.className = v;
    else if (k === 'dataset') Object.assign(node.dataset, v);
    else node.setAttribute(k, v === true ? '' : v);
  }
  if (html != null) node.innerHTML = html;
  return node;
}

const _injected = new Set();
/** Injecte une feuille de style une seule fois (dedupe par href). */
export function injectStylesheet(href) {
  if (!href || _injected.has(href)) return;
  _injected.add(href);
  const link = document.createElement('link');
  link.rel = 'stylesheet';
  link.href = href;
  document.head.appendChild(link);
}
