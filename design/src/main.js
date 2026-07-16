/**
 * Point d'entree de la maquette AioHUD.
 * Importe chaque box pour son effet de bord (definePanel), puis demarre l'app.
 * L'ordre des imports = l'ordre de montage dans le DOM.
 */
import './panels/skillchains.js';
import './panels/target.js';
import './panels/party.js';
import './panels/alliance.js';
import './panels/hate.js';
import './panels/treasurePool.js';
import './panels/clock.js';
import './panels/progress.js';
import './panels/zoneTracker.js';
import './panels/empyPop.js';
import './panels/bstPet.js';
import './panels/grimoire.js';
import './panels/focusTarget.js';
import './panels/playerHub.js';

import { startApp } from './app/App.js';

startApp();
