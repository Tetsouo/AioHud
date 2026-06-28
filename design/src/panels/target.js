import { definePanel } from '../app/registry.js';
import { bar, statIcon } from '../components/ui.js';
import { icon } from '../core/icons.js';
import { loop } from '../core/anim.js';

// DEMO : 12 debuffs = cas realiste sur un gros mob (Dia, Slow, Para, Blind, Gravity, DoT elem...).
// Largeur de la box calee pour les tenir sur UNE ligne. IDs d'icones presents dans assets/buffIcons.
const DBF_IDS = [3, 4, 5, 10, 11, 12, 13, 14, 28, 29, 33, 40, 41, 42];
const DBF_TIMERS = ['2:10', '0:55', '1:28', '0:42', '3:40', '0:31', '1:12', '0:48', '4:10', '1:33'];
const debuffRow = () => DBF_IDS.map((id, i) => {
  const short = i % 7 === 3;
  const timer = short ? `0:0${(i % 9) + 1}` : DBF_TIMERS[i % DBF_TIMERS.length];
  return statIcon(`buffIcons/${id}.png`, { mine: i % 3 === 0, timer, short });
}).join('');

definePanel({
  id: 'target',
  title: 'Target',
  type: 'TargetBar',
  css: new URL('./target.css', import.meta.url).href,
  pos: { x: 36.5, y: 49.4, w: 420 }, // meme largeur que le Player Hub
  render: (cfg = {}) => /* html */ `
    <div class="thead">
      <div class="tnm">Eschan Shadow Dragon</div>
      <div class="posbadge" style="color:var(--good)"><span class="posring" style="--ang:180deg"><svg class="posarrow" viewBox="0 0 20 20"><path d="M10 2 L16 16 L10 12 L4 16 Z"/></svg></span> <span class="poslbl">Behind</span></div>
    </div>
    <div class="tmeta">
      <span class="tident">Lv.135 · Dragon · <i class="claimed">yours</i></span>
      <span class="tstats">Dist <b>4.2</b> · Spd <b class="g">+0%</b> · TH <b>4</b></span>
    </div>
    ${bar('hp', 74, { value: '74%', barStyle: 'margin-top:6px', inner: '<span class="drain" style="left:74%;width:9%"></span>' })}
    <div class="actbar" id="tact">
      <div class="ab-head">
        <span class="ab-name">Flame Breath</span>
        <span class="ab-tag">Conal · Fire</span>
        <span class="ab-mt">${icon('dblarrow', 10)}<b>Kaories</b></span>
      </div>
      <div class="ab-bar"><i></i></div>
    </div>
    <div class="result">Flame Breath <span class="dmg">387</span></div>
    <div class="tdebuffs">${debuffRow()}</div>`,
});

// =========================================================================================
// ANIMATION DEMO (maquette). Deux effets, pilotes par UNE boucle (pas d'innerHTML global -> rien
// ne se reinitialise). En PAUSE pendant le Mode edition.
//   1) Badge positionnel : le joueur "tourne" autour de la cible (fleche = direction joueur vs
//      facing cible, label Front/Flank/Behind, couleur bad/warn/good).
//   2) Barre "action en cours" UNIQUE : alterne entre un TP move readied (orange) et un sort en
//      cast (couleur d'element) -> montre que la meme barre sert aux deux cas (jamais en meme temps).
// =========================================================================================
if (typeof window !== 'undefined' && !window.__aioTargetAnim) {
  window.__aioTargetAnim = true;

  // rel = angle joueur autour de la cible : 0 = face du mob, 180 = dos.
  const seg = (rel) => {
    const r = ((rel % 360) + 360) % 360;
    const front = Math.min(r, 360 - r);          // ecart angulaire a l'avant (0..180)
    if (front <= 60) return { lbl: 'Front', color: 'var(--bad)' };
    if (front >= 120) return { lbl: 'Behind', color: 'var(--good)' };
    return { lbl: 'Flank', color: 'var(--warn)' };
  };

  // actions enchainees par la demo : TP move readied (orange) puis sort en cast (element).
  const ACTIONS = [
    { name: 'Flame Breath', tag: 'Conal · Fire', color: 'var(--alert)', dur: 55 },   // TP move (paquet category 7)
    { name: 'Thunder IV', tag: 'Magic · Lightning', color: 'var(--thunder)', dur: 70 }, // sort/magie (paquet category 8)
  ];
  const HOLD = 12;                               // frames a 100% (le move "part") avant de passer au suivant
  let ai = 0, ap = 0;                            // index d'action + progression (frames)

  let t = 180;
  loop(() => {
    // --- badge positionnel ---
    const badge = document.querySelector('#target .posbadge');
    if (badge) {
      t += 1.1;                                  // ~13 s/tour ; angle non borne -> pas de saut visuel
      const { lbl, color } = seg(t);
      badge.style.color = color;
      badge.querySelector('.posring')?.style.setProperty('--ang', `${t.toFixed(0)}deg`);
      const lblEl = badge.querySelector('.poslbl');
      if (lblEl && lblEl.textContent !== lbl) lblEl.textContent = lbl;
    }

    // --- barre d'action unique (remplit puis bascule sur l'autre type d'action) ---
    const ab = document.getElementById('tact');
    if (ab) {
      const act = ACTIONS[ai];
      ap++;
      if (ap > act.dur + HOLD) { ai = (ai + 1) % ACTIONS.length; ap = 0; } // action suivante
      const cur = ACTIONS[ai];
      const pct = Math.min(ap / cur.dur, 1) * 100;
      ab.style.setProperty('--ab', cur.color);
      const fill = ab.querySelector('.ab-bar > i');
      if (fill) fill.style.width = `${pct.toFixed(1)}%`;
      const nameEl = ab.querySelector('.ab-name');
      const tagEl = ab.querySelector('.ab-tag');
      if (nameEl && nameEl.textContent !== cur.name) nameEl.textContent = cur.name;
      if (tagEl && tagEl.textContent !== cur.tag) tagEl.textContent = cur.tag;
    }
  });
}
