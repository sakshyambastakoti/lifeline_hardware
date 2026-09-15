const http = require('http');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const DEFAULT_PORT = 8080;
const ROOT_DIR = __dirname;

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.ttf': 'font/ttf',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.map': 'application/json',
};

function resolveFilePath(urlPath) {
  let cleanPath = urlPath.split('?')[0].split('#')[0];
  try {
    cleanPath = decodeURIComponent(cleanPath);
  } catch (e) {}

  // Specific convenience routes
  if (cleanPath === '/' || cleanPath === '/demo' || cleanPath === '/companion') {
    return path.join(ROOT_DIR, 'bluefy_companion', 'index.html');
  }
  if (cleanPath === '/showroom' || cleanPath === '/portal' || cleanPath === '/preview') {
    return path.join(ROOT_DIR, 'portal_preview', 'index.html');
  }
  if (cleanPath === '/tx') {
    return path.join(ROOT_DIR, 'portal_preview', 'tx_portal.html');
  }
  if (cleanPath === '/rx') {
    return path.join(ROOT_DIR, 'portal_preview', 'rx_portal.html');
  }
  if (cleanPath === '/scan') {
    return path.join(ROOT_DIR, 'lifeline_companion', 'scan_qr.html');
  }
  if (cleanPath === '/presentation' || cleanPath === '/pdf' || cleanPath === '/dossier') {
    return path.join(ROOT_DIR, 'presentation.html');
  }
  if (cleanPath === '/expo' || cleanPath === '/expo/') {
    return path.join(ROOT_DIR, 'lifeline_companion', 'dist', 'index.html');
  }

  // Expo static asset routing (_expo/... or assets/...)
  if (cleanPath.startsWith('/_expo/') || cleanPath.startsWith('/assets/')) {
    const expoPath = path.join(ROOT_DIR, 'lifeline_companion', 'dist', cleanPath);
    if (fs.existsSync(expoPath) && fs.statSync(expoPath).isFile()) {
      return expoPath;
    }
  }

  // Check in bluefy_companion
  const bluefyPath = path.join(ROOT_DIR, 'bluefy_companion', cleanPath.replace(/^\//, ''));
  if (fs.existsSync(bluefyPath) && fs.statSync(bluefyPath).isFile()) {
    return bluefyPath;
  }

  // Check in portal_preview
  const portalPath = path.join(ROOT_DIR, 'portal_preview', cleanPath.replace(/^\//, ''));
  if (fs.existsSync(portalPath) && fs.statSync(portalPath).isFile()) {
    return portalPath;
  }

  // Fallback to relative from root
  const rawPath = path.join(ROOT_DIR, cleanPath.replace(/^\//, ''));
  if (fs.existsSync(rawPath) && fs.statSync(rawPath).isFile()) {
    return rawPath;
  }

  return null;
}

const server = http.createServer((req, res) => {
  const filePath = resolveFilePath(req.url);

  if (!filePath || !fs.existsSync(filePath)) {
    // If asking for a file inside Expo web build, fallback to expo index.html (SPA)
    if (req.url.startsWith('/expo')) {
      const expoIndex = path.join(ROOT_DIR, 'lifeline_companion', 'dist', 'index.html');
      if (fs.existsSync(expoIndex)) {
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        fs.createReadStream(expoIndex).pipe(res);
        return;
      }
    }
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end(`404 Not Found: ${req.url}`);
    return;
  }

  const ext = path.extname(filePath).toLowerCase();
  const contentType = MIME_TYPES[ext] || 'application/octet-stream';

  res.writeHead(200, {
    'Content-Type': contentType,
    'Cache-Control': 'no-cache',
    'Access-Control-Allow-Origin': '*',
  });

  fs.createReadStream(filePath).pipe(res);
});

const isCheckOnly = process.argv.includes('--check');
const port = parseInt(process.env.PORT || DEFAULT_PORT, 10);

server.listen(port, '0.0.0.0', () => {
  const url = `http://localhost:${port}`;
  console.log('====================================================');
  console.log('  LIFELINE TACTICAL // BROWSER DEMO SERVER ACTIVE');
  console.log('====================================================');
  console.log(`  🚀 Interactive App Demo:  ${url}`);
  console.log(`  🏛️  Portal Showroom:       ${url}/showroom`);
  console.log(`  📱 Expo Web App Build:    ${url}/expo`);
  console.log(`  📡 TX Tactical Unit:      ${url}/tx`);
  console.log(`  📡 RX Base Station:       ${url}/rx`);
  console.log(`  📷 QR Scanner Page:       ${url}/scan`);
  console.log(`  📄 ICT Award Presentation: ${url}/presentation`);
  console.log('====================================================');

  if (isCheckOnly) {
    console.log('[Check Passed]: Server successfully initialized and listening.');
    server.close(() => process.exit(0));
    return;
  }

  // Attempt to open browser automatically
  const startCmd = process.platform === 'win32' ? `start ${url}` : `xdg-open ${url}`;
  exec(startCmd, () => {});
});

server.on('error', (err) => {
  if (err.code === 'EADDRINUSE') {
    console.warn(`Port ${port} is occupied, trying ${port + 1}...`);
    server.listen(port + 1, '0.0.0.0');
  } else {
    console.error('Server error:', err);
    process.exit(1);
  }
});
