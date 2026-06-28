/**
 * Job principal / sub job courant (global). Conditionne l'affichage de certaines box
 * (ex: la box BST ne s'affiche que si main OU sub = BST). Persiste comme le reste
 * (localStorage prioritaire, sinon job.json).
 */
import { loadLocal, saveLocal, putJSON } from './store.js';

const KEY = 'aiohud.job.v1';
const FILE = 'job.json';
const DEFAULT = { main: 'PLD', sub: 'WAR' };
let job = { ...DEFAULT };

// liste des jobs FFXI (ordre canonique)
export const JOBS = ['WAR', 'MNK', 'WHM', 'BLM', 'RDM', 'THF', 'PLD', 'DRK', 'BST', 'BRD', 'RNG',
  'SAM', 'NIN', 'DRG', 'SMN', 'BLU', 'COR', 'PUP', 'DNC', 'SCH', 'GEO', 'RUN'];

export async function loadJob() {
  const local = loadLocal(KEY);
  if (local) { job = { ...DEFAULT, ...local }; return; }
  try {
    const res = await fetch(FILE, { cache: 'no-store' });
    if (res.ok) job = { ...DEFAULT, ...((await res.json()).job || {}) };
  } catch { /* fichier absent : defaut */ }
}

export function getJob() { return job; }

export function setJob(patch) {
  job = { ...job, ...patch };
  saveLocal(KEY, job);
  putJSON(FILE, { _README: 'Job principal / sub courant (conditionne l\'affichage de certaines box).', job });
}

/** Une exigence de job (ex ['BST']) est-elle satisfaite par le job courant (main OU sub) ? */
export function jobMatches(jobsCsv) {
  if (!jobsCsv) return true;
  const allowed = jobsCsv.split(',');
  return allowed.includes(job.main) || allowed.includes(job.sub);
}
