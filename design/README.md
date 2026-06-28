# AioHUD — maquette concept (design)

Maquette HTML/CSS/JS de l'UI **AioHUD** (la vraie UI est native C++). Sert de
référence visuelle : vrais assets du jeu, barres = placeholders des fioles natives.

## Lancer

```bash
node serve.mjs            # ou double-clic serve.bat  (port 8777)
```

Puis ouvrir **http://localhost:8777/plugins/_aiohud_re/design/aiohud_mockup.html**

Le serveur sert toute la racine Windower (pour `/addons/...`), fait l'**auto-save**
(`PUT *.json`) et le **live-reload** (SSE qui watch le dossier `design/`).

## Choix d'architecture

**ES Modules natifs, sans build, sans dépendance.** Pas de framework : l'outil est
servi en statique et doit rester lisible/débogable directement. Une box = un module.

```
design/
├─ aiohud_mockup.html      Shell minimal : <link> du coeur + <script type=module> main.js
├─ serve.mjs               Serveur dev (statique + auto-save PUT + live-reload SSE)
├─ serve.bat               Raccourci de lancement
├─ zones.json              Données : zones interdites (auto-sauvées)
├─ panels.json             Données : positions des box (auto-sauvées)
└─ src/
   ├─ main.js              Entrée : importe les box (effet de bord) puis startApp()
   ├─ core/                Briques sans état de domaine
   │  ├─ config.js         Constantes (assets, URLs data, clés storage, snap)
   │  ├─ dom.js            Helpers DOM (qs, el, injectStylesheet)
   │  ├─ store.js          Persistance (localStorage + fetch/PUT JSON)
   │  └─ geometry.js       Maths de rectangles (clamp, overlap, snap) — pures
   ├─ components/
   │  └─ ui.js             Briques HTML réutilisables (bar, statIcon, jobIcon)
   ├─ app/
   │  ├─ registry.js       definePanel() / panels()  (registre des box)
   │  ├─ App.js            Montage : box + editbar + systèmes
   │  └─ Editbar.js        Barre d'édition (modes editing / zoning)
   ├─ systems/
   │  ├─ dragSystem.js     Déplacement box : snap + anti-chevauchement + persistance
   │  ├─ zonesSystem.js    Zones interdites : édition + auto-save + exception `allow`
   │  └─ liveReload.js     Live-reload client (SSE + repli polling)
   ├─ styles/              CSS du coeur (partagé)
   │  ├─ tokens.css        Design tokens (couleurs, rayons)
   │  ├─ base.css          Reset + scene
   │  ├─ panel.css         Chrome d'une box (cadre, titre, corps)
   │  ├─ components.css    Composants partagés (barres, icônes, lignes communes)
   │  ├─ zones.css         Zones interdites (+ mode zoning)
   │  └─ editmode.css      Editbar + retours visuels du drag
   └─ panels/              UNE BOX = un .js + un .css
      ├─ skillchains.js / .css
      ├─ target.js / .css
      ├─ party.js / .css
      ├─ hate.js / .css
      ├─ treasurePool.js / .css
      ├─ clock.js / .css
      ├─ progress.js / .css
      ├─ zoneTracker.js / .css
      ├─ empyPop.js / .css
      ├─ bstPet.js / .css
      ├─ grimoire.js / .css
      ├─ selfAction.js / .css
      ├─ focusTarget.js / .css
      └─ playerHub.js / .css
```

## Ajouter une box

1. Créer `src/panels/maBox.js` :

```js
import { definePanel } from '../app/registry.js';
definePanel({
  id: 'maBox',
  title: 'Ma Box',
  css: new URL('./maBox.css', import.meta.url).href,
  pos: { x: 50, y: 50, w: 240 },   // x/y en % de l'écran, w en px
  render: () => `<div class="tsub">contenu…</div>`,
});
```

2. Créer `src/panels/maBox.css` (styles propres à la box).
3. Ajouter `import './panels/maBox.js';` dans `src/main.js`.

Le montage, le drag, le snap, l'anti-chevauchement et la persistance sont gérés
automatiquement par le coeur.

## Modes (barre du haut)

- **⚙ Mode édition** — déplace les box. Magnétisme (snap) sur les voisines,
  anti-chevauchement (box ↔ box et box ↔ zones interdites). Positions auto-sauvées
  dans `panels.json`.
- **⛔ Zones jeu** — édite les zones interdites (les box sont masquées). Déplacer /
  redimensionner / ➕ ajouter / renommer (double-clic) / supprimer. Auto-save
  `zones.json`. `allow: ["id"]` sur une zone autorise cette box à la chevaucher.

## Persistance

`localStorage` (cache navigateur) + écriture disque via `PUT` (serveur dev). Bump
le suffixe des clés dans `core/config.js` (`STORAGE`) pour invalider un cache.
