// Feeds a recording to the xterm.js headless emulator and writes the screen after every chunk: for each row the runs of
// [text, fg, bg, flags] (fg/bg: "d" default, "p<n>" palette, "r<hex>" rgb; flags: b bold, d dim, i italic, u underline, v inverse, s strike).
import fs from 'node:fs';
import xterm from '@xterm/headless';

const [input, output] = process.argv.slice(2);
const rec = JSON.parse(fs.readFileSync(input, 'utf8'));
const term = new xterm.Terminal({ cols: rec.cols, rows: rec.rows, allowProposedApi: true, scrollback: 1000 });

const color = (isDefault, isPalette, isRgb, value) => (isDefault ? 'd' : isPalette ? 'p' + value : isRgb ? 'r' + value.toString(16).padStart(6, '0') : 'd');

function snapshot() {
	const buffer = term.buffer.active;
	const rows = [];
	for (let y = 0; y < rec.rows; y++) {
		const line = buffer.getLine(buffer.baseY + y);
		const runs = [];
		let current = null;
		for (let x = 0; x < rec.cols; x++) {
			const cell = line ? line.getCell(x) : null;
			if (cell === null || cell === undefined) continue;
			if (cell.getWidth() === 0) continue;
			const chars = cell.getChars() === '' ? ' ' : cell.getChars();
			const fg = color(cell.isFgDefault(), cell.isFgPalette(), cell.isFgRGB(), cell.getFgColor());
			const bg = color(cell.isBgDefault(), cell.isBgPalette(), cell.isBgRGB(), cell.getBgColor());
			const flags = (cell.isBold() ? 'b' : '') + (cell.isDim() ? 'd' : '') + (cell.isItalic() ? 'i' : '') + (cell.isUnderline() ? 'u' : '') + (cell.isInverse() ? 'v' : '') + (cell.isStrikethrough() ? 's' : '');
			if (current !== null && current[1] === fg && current[2] === bg && current[3] === flags) current[0] += chars;
			else runs.push((current = [chars, fg, bg, flags]));
		}
		rows.push(runs);
	}
	return { cursor: [buffer.cursorY, buffer.cursorX], scrolledRows: buffer.baseY, rows };
}

const samples = [];
let fed = '';
for (const chunk of rec.chunks) {
	await new Promise((resolve) => term.write(chunk.data, resolve));
	fed += chunk.data;
	samples.push({ t: chunk.t, length: fed.length, ...snapshot() });
}
fs.writeFileSync(output, JSON.stringify({ cols: rec.cols, rows: rec.rows, samples }));
const last = samples[samples.length - 1];
console.log('samples', samples.length);
// the screen at the sample that has the most characters outside the first rows: print sample 6 as text
const pick = Number(process.env.PICK || 6);
const s = samples[Math.min(pick, samples.length - 1)];
console.log(`--- sample ${pick} (cursor ${s.cursor}) ---`);
for (const row of s.rows) console.log(row.map((run) => run[0]).join(''));
console.log('--- colors used (fg/bg) in this sample ---');
const used = new Set();
for (const row of s.rows) for (const run of row) used.add(`${run[1]}/${run[2]}/${run[3]}`);
console.log([...used].join('  '));
