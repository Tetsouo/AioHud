import { definePanel } from '../app/registry.js';
import { bar, statIcon } from '../components/ui.js';

// Cible SECONDAIRE auto-suivie -> lecture la plus COMPACTE possible :
// nom + distance sur une ligne, barre HP fine (% integre, pas de label redondant),
// rangee de debuffs serree (icones reduites). Liseré or = debuff applique par toi.
definePanel({
  id: 'focus',
  title: 'Focus Target',
  type: 'FocusBar',
  css: new URL('./focusTarget.css', import.meta.url).href,
  pos: { x: 51.0, y: 61.2, w: 176 },
  render: (cfg = {}) => /* html */ `
    <div class="frow">
      <span class="focusnm">Bumba</span>
      ${bar('hp', 93, { value: '93%', cls: 'fhp' })}
      <span class="fdist">18.4y</span>
    </div>
    <div class="tdebuffs fdebuffs">
      ${statIcon('buffIcons/134.png', { mine: true, timer: '2:10' })}
      ${statIcon('buffIcons/13.png', { mine: true, timer: '0:55' })}
      ${statIcon('buffIcons/5.png', { timer: '0:07', short: true })}
    </div>`,
});
