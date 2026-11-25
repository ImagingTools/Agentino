// Records the raw output of mc.exe in a ConPTY while a script of keys is sent:
//   node record.mjs <scenario> <out.json>        scenario: basic | dialogs      (COLS, ROWS, MC are optional)
// The scenarios send the keys of xterm (the table is in Include/agentinoqml/Qml/TerminalEmulator.js, encodeKey).
import pty from 'node-pty';
import fs from 'node:fs';
import path from 'node:path';

const COLS = Number(process.env.COLS || 80);
const ROWS = Number(process.env.ROWS || 24);
const scenario = process.argv[2] || 'basic';
const out = process.argv[3] || `mc-${scenario}.json`;
const MC = process.env.MC || 'C:\\Program Files (x86)\\Midnight Commander\\mc.exe';

const K = {
	up: '\x1b[A', down: '\x1b[B', right: '\x1b[C', left: '\x1b[D', home: '\x1b[H', end: '\x1b[F', pageDown: '\x1b[6~', pageUp: '\x1b[5~',
	ctrlUp: '\x1b[1;5A', ctrlLeft: '\x1b[1;5D', ctrlRight: '\x1b[1;5C', shiftF10: '\x1b[21;2~',
	f1: '\x1bOP', f3: '\x1bOR', f4: '\x1bOS', f5: '\x1b[15~', f7: '\x1b[18~', f8: '\x1b[19~', f9: '\x1b[20~', f10: '\x1b[21~',
	esc: '\x1b', enter: '\r', tab: '\t',
};

// [wait before the keys in ms, keys]
const SCRIPTS = {
	basic: [
		[3000, ''], // the first screen
		[500, K.down],
		[500, K.down],
		[500, K.up],
		[800, K.f9], // the menu
		[800, K.right],
		[800, K.esc],
		[800, K.ctrlUp],
		[800, K.f10], // the quit dialog
		[800, K.esc],
		[800, K.f10],
		[800, K.enter],
	],
	dialogs: [
		[3000, ''],
		[600, K.down],
		[400, K.down],
		[400, K.down], // a file
		[600, K.f3], // the viewer
		[1200, K.pageDown],
		[800, K.esc],
		[800, K.f4], // the editor
		[1500, 'abc'],
		[800, K.down],
		[800, K.esc], // asks to save a changed file
		[1000, 'n'],
		[800, K.f5], // the dialog of copy
		[1000, K.tab],
		[600, K.esc],
		[800, K.f7], // the dialog of a new directory
		[1000, 'new'],
		[600, K.esc],
		[800, K.tab], // the other panel
		[600, K.ctrlLeft],
		[600, K.ctrlRight],
		[600, K.end],
		[600, K.home],
		[800, K.f1], // the help
		[1500, K.pageDown],
		[800, K.esc],
		[800, K.f9],
		[800, K.right],
		[800, K.down],
		[800, K.esc],
		[800, K.f10],
		[1000, K.enter],
	],
};
if (!(scenario in SCRIPTS)) throw new Error(`scenario ${scenario}`);

// a folder with a few small files of its own, so that nothing of the machine is on the screen
const shared = 'C:\\Users\\Public';
const demo = path.join(shared, 'mcdemo');
fs.rmSync(demo, { recursive: true, force: true });
fs.mkdirSync(path.join(demo, 'docs'), { recursive: true });
fs.mkdirSync(path.join(demo, 'src'), { recursive: true });
fs.writeFileSync(path.join(demo, 'notes.txt'), Array.from({ length: 40 }, (_, i) => `line ${i + 1} of the notes`).join('\r\n') + '\r\n');
fs.writeFileSync(path.join(demo, 'readme.txt'), 'A small file.\r\nIt has a few lines.\r\n');
fs.writeFileSync(path.join(demo, 'data.csv'), 'a,b,c\r\n1,2,3\r\n');

const chunks = [];
const started = Date.now();
const env = { ...process.env, TERM: 'xterm', TEMP: shared, TMP: shared };
const term = pty.spawn(MC, [], { name: 'xterm', cols: COLS, rows: ROWS, cwd: demo, env, useConpty: true });
term.onData((data) => chunks.push({ t: Date.now() - started, data }));
let exited = false;
term.onExit(() => {
	exited = true;
});

for (const [wait, keys] of SCRIPTS[scenario]) {
	await new Promise((r) => setTimeout(r, wait));
	if (exited) break;
	if (keys) term.write(keys);
}
await new Promise((r) => setTimeout(r, 1500));
if (!exited) term.kill();
fs.writeFileSync(out, JSON.stringify({ cols: COLS, rows: ROWS, chunks }, null, 0));
console.log('chunks', chunks.length, 'bytes', chunks.reduce((n, c) => n + c.data.length, 0), 'exited', exited);
