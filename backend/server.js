const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
const frontend = path.join(root, 'frontend');
const compilerDir = path.join(root, 'compiler');
const executable = path.join(compilerDir, process.platform === 'win32' ? 'tac_generator.exe' : 'tac_generator');
const port = Number(process.env.PORT || 3000);

function compileGenerator() {
  const result = spawnSync('gcc', ['-std=c11', '-Wall', '-Wextra', '-O2', 'tac_generator.c', '-o', executable], { cwd: compilerDir, encoding: 'utf8' });
  if (result.status !== 0) throw new Error(result.stderr || 'Could not compile the C generator.');
}

function send(response, status, body, contentType) {
  response.writeHead(status, {
    'Content-Type': contentType,
    'Cache-Control': 'no-cache',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type'
  });
  response.end(body);
}

function handleGenerate(request, response) {
  let body = '';
  request.on('data', chunk => { body += chunk; });
  request.on('end', () => {
    try {
      const expression = JSON.parse(body).expression;
      if (typeof expression !== 'string' || expression.trim().length === 0) return send(response, 400, JSON.stringify({ error: 'Please enter an expression.' }), 'application/json');
      const result = spawnSync(executable, { cwd: compilerDir, input: expression, encoding: 'utf8' });
      if (result.status !== 0) return send(response, 422, JSON.stringify({ error: result.stderr || 'Invalid Expression' }), 'application/json');
      return send(response, 200, JSON.stringify({ tac: result.stdout.trimEnd() }), 'application/json');
    } catch (error) {
      return send(response, 400, JSON.stringify({ error: 'Request must contain a valid expression.' }), 'application/json');
    }
  });
}

function serveStatic(request, response) {
  const requested = request.url === '/' ? '/index.html' : request.url;
  const filePath = path.normalize(path.join(frontend, requested));
  if (!filePath.startsWith(frontend)) return send(response, 403, 'Forbidden', 'text/plain');
  const extensions = { '.html': 'text/html', '.css': 'text/css', '.js': 'text/javascript', '.svg': 'image/svg+xml' };
  fs.readFile(filePath, (error, data) => {
    if (error) return send(response, 404, 'Not found', 'text/plain');
    send(response, 200, data, extensions[path.extname(filePath)] || 'application/octet-stream');
  });
}

try {
  compileGenerator();
  http.createServer((request, response) => {
    if (request.method === 'OPTIONS') return send(response, 204, '', 'text/plain');
    if (request.method === 'POST' && request.url === '/api/generate') return handleGenerate(request, response);
    if (request.method === 'GET') return serveStatic(request, response);
    return send(response, 405, 'Method not allowed', 'text/plain');
  }).listen(port, () => console.log(`TAC Generator running at http://localhost:${port}`));
} catch (error) {
  console.error(error.message);
  process.exit(1);
}