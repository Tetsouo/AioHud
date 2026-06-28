// AioHUD design dev-server : sert la racine Windower + auto-save + live-reload.
//   - GET  : fichiers statiques (/addons/..., /plugins/..., screenshots, src/...)
//   - PUT  : ecrit un .json sur le disque (auto-save zones.json / panels.json)
//   - SSE  : GET /__livereload  -> pousse "reload" quand un fichier source change
//   - GET  /__version           -> mtime max (repli polling du live-reload)
// Lancer :  node serve.mjs        (port 8777 par defaut, ou: node serve.mjs 9000)
// Page   :  http://localhost:8777/plugins/_aiohud_re/design/aiohud_mockup.html
import http from 'node:http';
import { promises as fs } from 'node:fs';
import { createReadStream, watch } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const DESIGN = __dirname;                                  // dossier de la maquette (watch)
const ROOT = path.resolve(__dirname, '..', '..', '..');    // racine Windower (sert les assets)
const PORT = Number(process.argv[2] || process.env.PORT || 8777);
const PAGE = '/plugins/_aiohud_re/design/aiohud_mockup.html';

// les fichiers de DONNEES sont auto-sauves par la page : leur ecriture ne doit PAS recharger
const DATA_FILES = new Set(['zones.json', 'panels.json', 'boxconfig.json', 'visibility.json', 'job.json', 'layout.json']);

const MIME = {
  '.html': 'text/html; charset=utf-8', '.htm': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8', '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8', '.json': 'application/json; charset=utf-8',
  '.png': 'image/png', '.jpg': 'image/jpeg', '.jpeg': 'image/jpeg', '.gif': 'image/gif',
  '.bmp': 'image/bmp', '.webp': 'image/webp', '.svg': 'image/svg+xml', '.ico': 'image/x-icon',
  '.woff': 'font/woff', '.woff2': 'font/woff2', '.ttf': 'font/ttf',
};

// resout une URL vers un chemin disque, en bloquant toute sortie de ROOT
function resolveSafe(urlPath) {
  const clean = decodeURIComponent(urlPath.split('?')[0]).replace(/\\/g, '/');
  const abs = path.normalize(path.join(ROOT, clean));
  if (abs !== ROOT && !abs.startsWith(ROOT + path.sep)) return null; // path traversal
  return abs;
}

/* ---- live-reload : watch du dossier design, broadcast SSE ------------------ */
const clients = new Set();           // reponses SSE ouvertes
let version = Date.now();            // "version" = horodatage du dernier changement
let debounce = null;

watch(DESIGN, { recursive: true }, (_evt, filename) => {
  const name = (filename || '').replace(/\\/g, '/');
  const base = name.split('/').pop();
  if (DATA_FILES.has(base)) return;        // ignore les ecritures de donnees (auto-save)
  if (base && base.endsWith('~')) return;  // fichiers temporaires d'editeurs
  clearTimeout(debounce);
  debounce = setTimeout(() => {
    version = Date.now();
    for (const res of clients) { try { res.write('data: reload\n\n'); } catch { /* client parti */ } }
    console.log('  reload ' + name);
  }, 120);
});

const server = http.createServer(async (req, res) => {
  try {
    const url = req.url || '/';

    // ---- SSE live-reload -----------------------------------------------
    if (url === '/__livereload') {
      res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        Connection: 'keep-alive',
      });
      res.write('retry: 1000\n\n');
      clients.add(res);
      req.on('close', () => clients.delete(res));
      return;
    }
    // ---- version (repli polling) ---------------------------------------
    if (url === '/__version') {
      res.writeHead(200, { 'Content-Type': 'text/plain', 'Cache-Control': 'no-store' }).end(String(version));
      return;
    }

    const urlPath = url === '/' ? PAGE : url;
    const abs = resolveSafe(urlPath);
    if (!abs) { res.writeHead(403).end('Forbidden'); return; }

    // ---- AUTO-SAVE : ecriture d'un .json -------------------------------
    if (req.method === 'PUT' || req.method === 'POST') {
      if (path.extname(abs).toLowerCase() !== '.json') {
        res.writeHead(415).end('Only .json writes allowed'); return;
      }
      const chunks = [];
      for await (const c of req) chunks.push(c);
      const body = Buffer.concat(chunks).toString('utf8');
      try { JSON.parse(body); } catch { res.writeHead(400).end('Invalid JSON'); return; }
      await fs.writeFile(abs, body, 'utf8');
      console.log('  saved  ' + path.relative(ROOT, abs).replace(/\\/g, '/'));
      res.writeHead(200, { 'Content-Type': 'application/json' }).end('{"ok":true}');
      return;
    }

    // ---- GET / HEAD : fichiers statiques -------------------------------
    if (req.method !== 'GET' && req.method !== 'HEAD') {
      res.writeHead(405).end('Method Not Allowed'); return;
    }
    let stat;
    try { stat = await fs.stat(abs); } catch { res.writeHead(404).end('Not found: ' + urlPath); return; }
    if (stat.isDirectory()) { res.writeHead(403).end('Directory listing disabled'); return; }

    const type = MIME[path.extname(abs).toLowerCase()] || 'application/octet-stream';
    res.writeHead(200, { 'Content-Type': type, 'Content-Length': stat.size, 'Cache-Control': 'no-store' });
    if (req.method === 'HEAD') { res.end(); return; }
    createReadStream(abs).pipe(res);
  } catch (e) {
    console.error(e);
    if (!res.headersSent) res.writeHead(500).end('Server error');
  }
});

server.listen(PORT, () => {
  console.log('AioHUD design server');
  console.log('  root   ' + ROOT);
  console.log('  page   http://localhost:' + PORT + PAGE);
  console.log('  PUT *.json -> auto-save · live-reload SSE actif. Ctrl+C pour arreter.');
});
