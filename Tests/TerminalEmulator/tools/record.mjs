// Records the raw output of mc.exe in a ConPTY while a script of keys is sent: node record.mjs <out.json>  (COLS, ROWS, MC are optional).
import pty from 'node-pty';
import fs from 'node:fs';

const COLS = Number(process.env.COLS || 80);
const ROWS = Number(process.env.ROWS || 24);
const out = process.argv[2] || 'mc-basic.raw';
const MC = process.env.MC || 'C:\\Program Files (x86)\\Midnight Commander\\mc.exe';

// the real script: menu, panel moves, a dialog, quit
const script = [
	[3000, ''], // wait for the first screen
	[500, '\x1b[B'],
	[500, '\x1b[B'],
	[500, '\x1b[A'],
	[800, '\x1b[20~'], // F9: the menu
	[800, '\x1b[C'],
	[800, '\x1b'],
	[800, '\x1b[1;5A'], // Ctrl+Up
	[800, '\x1b[21~'], // F10: quit dialog
	[800, '\x1b'],
	[800, '\x1b[21~'],
	[800, '\r'],
];

const chunks = [];
const started = Date.now();
const term = pty.spawn(MC, [], { name: 'xterm', cols: COLS, rows: ROWS, cwd: 'C:\\Windows', env: { ...process.env, TERM: 'xterm' }, useConpty: true });
term.onData((data) => chunks.push({ t: Date.now() - started, data }));
let exited = false;
term.onExit(() => {
	exited = true;
});

for (const [wait, keys] of script) {
	await new Promise((r) => setTimeout(r, wait));
	if (exited) break;
	if (keys) term.write(keys);
}
await new Promise((r) => setTimeout(r, 1500));
if (!exited) term.kill();
fs.writeFileSync(out, JSON.stringify({ cols: COLS, rows: ROWS, chunks }, null, 0));
console.log('chunks', chunks.length, 'bytes', chunks.reduce((n, c) => n + c.data.length, 0), 'exited', exited);
