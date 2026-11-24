// Tests of Include/agentinoqml/Qml/TerminalEmulator.js: the module is a QML JavaScript library, so it is read as a text and run in a vm.
//   - the recorded output of mc.exe (fixtures/mc-basic.json) gives the same screens as the reference emulator (xterm.js headless) after every chunk
//   - the same stream byte by byte gives the same screen (a sequence cut by a chunk)
//   - the sequences one by one
// node --test Tests/TerminalEmulator/emulator.test.mjs

import assertModule from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

// the values of the module come from another realm of the vm: they are compared as plain data
const plain = (value) => JSON.parse(JSON.stringify(value));
const assert = {
	deepEqual: (actual, expected, message) => assertModule.deepEqual(plain(actual), plain(expected), message),
	equal: (actual, expected, message) => assertModule.equal(actual, expected, message),
	ok: (value, message) => assertModule.ok(value, message),
};

const here = path.dirname(fileURLToPath(import.meta.url));
const source = fs.readFileSync(path.join(here, '..', '..', 'Include', 'agentinoqml', 'Qml', 'TerminalEmulator.js'), 'utf8').replace(/^\.pragma library/m, '');
const E = vm.runInNewContext(`${source}\n({ create, feed, resize, takeReplies, rowText, rowHtml, lastUsedColumn, clearScreen, encodeKey, encodeMouse, encodePaste, encodeFocus, selectionText, selectionColumns, FL_BOLD, FL_INVERSE, FL_UNDERLINE, FL_DIM, FL_ITALIC, FL_STRIKE, RGB_BASE })`);

// every recording of fixtures/ (made by tools/record.mjs) with the screens that the reference gave for it (tools/reference.mjs)
const fixtureDir = path.join(here, 'fixtures');
const fixtures = fs.readdirSync(fixtureDir)
	.filter((name) => name.endsWith('.json') && !name.endsWith('.expected.json'))
	.map((name) => ({ name: name.replace(/\.json$/, ''), recording: JSON.parse(fs.readFileSync(path.join(fixtureDir, name), 'utf8')), expected: JSON.parse(fs.readFileSync(path.join(fixtureDir, name.replace(/\.json$/, '.expected.json')), 'utf8')) }));

// the screen of the emulator in the form of the reference: for each row the runs [text, fg, bg, flags]
function color(value) {
	if (value < 0) return 'd';
	if (value >= E.RGB_BASE) return 'r' + (value - E.RGB_BASE).toString(16).padStart(6, '0');
	return 'p' + value;
}

function flags(cell) {
	return (cell.fl & E.FL_BOLD ? 'b' : '') + (cell.fl & E.FL_DIM ? 'd' : '') + (cell.fl & E.FL_ITALIC ? 'i' : '') + (cell.fl & E.FL_UNDERLINE ? 'u' : '') + (cell.fl & E.FL_INVERSE ? 'v' : '') + (cell.fl & E.FL_STRIKE ? 's' : '');
}

function runsOf(t) {
	return t.grid.map((cells) => {
		const runs = [];
		let current = null;
		for (const cell of cells) {
			if (cell.ch === '') continue;
			const fg = color(cell.fg);
			const bg = color(cell.bg);
			const fl = flags(cell);
			if (current !== null && current[1] === fg && current[2] === bg && current[3] === fl) current[0] += cell.ch;
			else runs.push((current = [cell.ch, fg, bg, fl]));
		}
		return runs;
	});
}

function screenText(t) {
	return t.grid.map((cells, r) => E.rowText(t, r));
}

function expectedText(sample) {
	return sample.rows.map((runs) => runs.map((run) => run[0]).join('').replace(/ +$/, ''));
}

for (const { name, recording, expected } of fixtures) {
	test(`${name}: the recorded mc.exe gives the screens of the reference after every chunk`, () => {
		const t = E.create(recording.cols, recording.rows);
		recording.chunks.forEach((chunk, index) => {
			E.feed(t, chunk.data);
			const sample = expected.samples[index];
			assert.deepEqual(screenText(t), expectedText(sample), `text after chunk ${index}`);
			assert.deepEqual(runsOf(t), sample.rows, `attributes after chunk ${index}`);
			assert.deepEqual([t.cursorRow, t.cursorCol], sample.cursor, `cursor after chunk ${index}`);
		});
	});

	test(`${name}: the same stream one character at a time gives the same screen`, () => {
		const t = E.create(recording.cols, recording.rows);
		for (const chunk of recording.chunks) for (const ch of chunk.data) E.feed(t, ch);
		const last = expected.samples[expected.samples.length - 1];
		assert.deepEqual(runsOf(t), last.rows);
		assert.deepEqual([t.cursorRow, t.cursorCol], last.cursor);
	});

	test(`${name}: mc has no alternate screen and its screen does not scroll into the history`, () => {
		const t = E.create(recording.cols, recording.rows);
		const out = [];
		t.onScrollOut = (row) => out.push(E.rowText(t, row));
		for (const chunk of recording.chunks) E.feed(t, chunk.data);
		assert.equal(t.altActive, false);
		assert.deepEqual(out.filter((line) => line.trim() !== ''), [], 'no rows left the screen');
	});
}

function screenOf(text, cols = 20, rows = 5) {
	const t = E.create(cols, rows);
	E.feed(t, text);
	return t;
}

test('erase characters and repeat use the background that is set', () => {
	const t = screenOf('\x1b[44mab\x1b[1;1H\x1b[3X\x1b[0m');
	assert.deepEqual(t.grid[0].slice(0, 4).map((cell) => [cell.ch, cell.bg]), [[' ', 4], [' ', 4], [' ', 4], [' ', -1]]);
	const r = screenOf('-\x1b[4b');
	assert.equal(E.rowText(r, 0), '-----');
});

test('SGR: background, bright colors, 256, truecolor, attributes and their reset', () => {
	const t = screenOf('\x1b[41;93mA\x1b[100mB\x1b[38;5;200;48;5;17mC\x1b[38;2;1;2;3;48;2;4;5;6mD\x1b[1;3;4;9;2mE\x1b[22;23;24;29mF\x1b[7mG\x1b[27mH\x1b[0mI');
	const cells = t.grid[0];
	assert.deepEqual([cells[0].fg, cells[0].bg], [11, 1]);
	assert.deepEqual([cells[1].fg, cells[1].bg], [11, 8]);
	assert.deepEqual([cells[2].fg, cells[2].bg], [200, 17]);
	assert.deepEqual([cells[3].fg, cells[3].bg], [E.RGB_BASE + 0x010203, E.RGB_BASE + 0x040506]);
	assert.equal(cells[4].fl, E.FL_BOLD | E.FL_ITALIC | E.FL_UNDERLINE | E.FL_STRIKE | E.FL_DIM);
	assert.equal(cells[5].fl, 0);
	assert.equal(cells[6].fl, E.FL_INVERSE);
	assert.equal(cells[7].fl, 0);
	assert.deepEqual([cells[8].fg, cells[8].bg, cells[8].fl], [-1, -1, 0]);
});

test('the color forms with a colon and the short forms are taken', () => {
	const t = screenOf('\x1b[38:5:9mA\x1b[m\x1b[48;5;300mB');
	assert.equal(t.grid[0][0].fg, 9);
	assert.equal(t.grid[0][1].bg, 255);
});

test('the sets of characters: line drawing with ESC ( 0, shift out and back', () => {
	const t = screenOf('\x1b(0lqk\x1b(Bq\x0e\x1b)0x\x0fx');
	assert.equal(E.rowText(t, 0), '┌─┐q│x');
});

test('save and restore of the cursor with the attributes (ESC 7/8, CSI s/u)', () => {
	const t = screenOf('\x1b[2;3H\x1b[31m\x1b7\x1b[1;1H\x1b[0mx\x1b8y');
	assert.equal(E.rowText(t, 0), 'x');
	assert.equal(t.grid[1][2].ch, 'y');
	assert.equal(t.grid[1][2].fg, 1);
	const u = screenOf('\x1b[2;3H\x1b[s\x1b[1;1H\x1b[uz');
	assert.equal(u.grid[1][2].ch, 'z');
});

test('cursor movement: next and previous line, absolute column and row, clamped', () => {
	const t = screenOf('\x1b[3;5H\x1b[2Eab\x1b[1Fc\x1b[1Gd\x1b[100;100Hz');
	assert.equal(t.grid[4].slice(0, 2).map((cell) => cell.ch).join(''), 'ab');
	assert.equal(t.grid[3][0].ch, 'd');
	assert.equal(t.grid[3][1].ch, ' ');
	assert.equal(t.grid[4][19].ch, 'z');
});

test('deferred wrap: a full line followed by CR LF does not make an empty row', () => {
	const t = screenOf('abcdefghijklmnopqrst\r\nx');
	assert.equal(E.rowText(t, 0), 'abcdefghijklmnopqrst');
	assert.equal(E.rowText(t, 1), 'x');
	const w = screenOf('abcdefghijklmnopqrstu');
	assert.equal(E.rowText(w, 1), 'u');
});

test('autowrap off keeps the last column', () => {
	const t = screenOf('\x1b[?7labcdefghijklmnopqrstuvw\x1b[?7h');
	assert.equal(E.rowText(t, 0), 'abcdefghijklmnopqrsw');
	assert.equal(E.rowText(t, 1), '');
});

test('scroll: the rows that leave the top go to the history only on the main screen', () => {
	const t = E.create(10, 3);
	const out = [];
	t.onScrollOut = (row) => out.push(E.rowText(t, row));
	E.feed(t, 'a\r\nb\r\nc\r\nd\r\ne');
	assert.deepEqual(out, ['a', 'b']);
	assert.deepEqual(screenText(t), ['c', 'd', 'e']);
	E.feed(t, '\x1b[?1049h\r\nx\r\ny\r\nz\r\nw');
	assert.deepEqual(out, ['a', 'b'], 'the alternate screen keeps nothing');
});

test('the scroll region, scroll up and down, insert and delete of lines and characters', () => {
	const t = screenOf('1\r\n2\r\n3\r\n4\r\n5', 10, 5);
	E.feed(t, '\x1b[2;4r\x1b[4;1H\n');
	assert.deepEqual(screenText(t), ['1', '3', '4', '', '5']);
	E.feed(t, '\x1b[1T');
	assert.deepEqual(screenText(t), ['1', '', '3', '4', '5']);
	E.feed(t, '\x1b[2;1H\x1b[1M');
	assert.deepEqual(screenText(t), ['1', '3', '4', '', '5']);
	E.feed(t, '\x1b[2;1H\x1b[1L');
	assert.deepEqual(screenText(t), ['1', '', '3', '4', '5']);
	const c = screenOf('abcdef');
	E.feed(c, '\x1b[1;3H\x1b[2P');
	assert.equal(E.rowText(c, 0), 'abef');
	E.feed(c, '\x1b[1;2H\x1b[2@');
	assert.equal(E.rowText(c, 0), 'a  bef');
});

test('erase in line and in display with the current background', () => {
	const t = screenOf('abc\r\ndef\x1b[44m\x1b[2;2H\x1b[1K');
	assert.deepEqual(t.grid[1].slice(0, 3).map((cell) => [cell.ch, cell.bg]), [[' ', 4], [' ', 4], ['f', -1]]);
	const d = screenOf('abc\r\ndef\r\nghi\x1b[44m\x1b[2;2H\x1b[0J');
	assert.deepEqual(d.grid[1].slice(0, 3).map((cell) => [cell.ch, cell.bg]), [['d', -1], [' ', 4], [' ', 4]]);
	assert.equal(d.grid[2][0].bg, 4);
	assert.equal(d.grid[0][0].ch, 'a');
	const all = screenOf('abc\x1b[2J');
	assert.deepEqual(screenText(all), ['', '', '', '', '']);
});

test('tabs: the stops are every 8, set and clear', () => {
	const t = screenOf('a\tb\x1b[1;20H\x1b[Zc');
	assert.equal(t.grid[0][8].ch, 'b');
	assert.equal(t.grid[0][16].ch, 'c');
	const s = screenOf('\x1b[3g\x1b[1;5H\x1bH\x1b[1;1H\tx');
	assert.equal(s.grid[0][4].ch, 'x');
});

test('wide characters take two cells, combining ones none, a character outside the BMP is one', () => {
	const t = screenOf('a中b́c😀d');
	assert.deepEqual(t.grid[0].slice(0, 8).map((cell) => cell.ch), ['a', '中', '', 'b́', 'c', '😀', '', 'd']);
	assert.equal(E.rowText(t, 0), 'a中b́c😀d');
});

test('a surrogate pair cut by a chunk is one character', () => {
	const t = E.create(10, 2);
	E.feed(t, 'x\ud83d');
	E.feed(t, '\ude00y');
	assert.equal(E.rowText(t, 0), 'x😀y');
});

test('modes: cursor, application keys, bracketed paste, mouse, focus, insert', () => {
	const t = screenOf('\x1b[?25l\x1b[?1h\x1b[?2004h\x1b[?1002h\x1b[?1006h\x1b[?1004h');
	assert.deepEqual([t.cursorVisible, t.appCursorKeys, t.bracketedPaste, t.mouseMode, t.mouseSgr, t.focusEvents], [false, true, true, 1002, true, true]);
	E.feed(t, '\x1b[?25h\x1b[?1l\x1b[?2004l\x1b[?1002l\x1b[?1006l\x1b[?1004l');
	assert.deepEqual([t.cursorVisible, t.appCursorKeys, t.bracketedPaste, t.mouseMode, t.mouseSgr, t.focusEvents], [true, false, false, 0, false, false]);
	const i = screenOf('abc\x1b[1;1H\x1b[4hxy\x1b[4l');
	assert.equal(E.rowText(i, 0), 'xyabc');
});

test('the alternate screen: the main one comes back as it was', () => {
	const t = screenOf('main\x1b[?1049hother\x1b[2J');
	assert.equal(t.altActive, true);
	assert.equal(E.rowText(t, 0), '');
	E.feed(t, '\x1b[?1049l');
	assert.equal(t.altActive, false);
	assert.equal(E.rowText(t, 0), 'main');
	assert.deepEqual([t.cursorRow, t.cursorCol], [0, 4]);
});

test('the replies: device attributes, status, cursor position, size', () => {
	const t = screenOf('\x1b[2;4H\x1b[c\x1b[5n\x1b[6n\x1b[>c\x1b[18t');
	assert.equal(E.takeReplies(t), '\x1b[?1;2c\x1b[0n\x1b[2;4R\x1b[>0;0;0c\x1b[8;5;20t');
	assert.equal(E.takeReplies(t), '');
});

test('the title (OSC 0 and 2) ends with BEL or ST and is cut by chunks', () => {
	const t = E.create(10, 2);
	E.feed(t, '\x1b]0;one\x07\x1b]2;tw');
	E.feed(t, 'o\x1b\\x');
	assert.equal(t.title, 'two');
	assert.equal(E.rowText(t, 0), 'x');
});

test('strings that are skipped and the unknown sequences print nothing', () => {
	const t = screenOf('a\x1bP1$rtext\x1b\\b\x1b_apc\x07c\x1b[?9999hd\x1b=\x1b>e\x1b[ qf');
	assert.equal(E.rowText(t, 0), 'abcdef');
});

test('a resize keeps what fits, and ConPTY says its size with CSI 8 ; rows ; cols t', () => {
	const t = screenOf('abcdef\r\nxyz');
	E.resize(t, 4, 3);
	assert.deepEqual(screenText(t), ['abcd', 'xyz', '']);
	E.feed(t, '\x1b[8;5;9t');
	assert.deepEqual([t.cols, t.rows], [9, 5]);
});

test('the origin mode counts the rows from the top of the scroll region', () => {
	const t = screenOf('\x1b[2;4r\x1b[?6h\x1b[1;1Hx\x1b[6n');
	assert.equal(t.grid[1][0].ch, 'x');
	assert.equal(E.takeReplies(t), '\x1b[1;2R');
});

test('rowHtml: spans by style, the background, inverse, bold, the cursor, escaping and the trimmed end', () => {
	const t = screenOf('\x1b[1;93;44mAB\x1b[0m <&>\x1b[7mi\x1b[0m   ');
	const options = { defaultFg: '#111111', defaultBg: '#ffffff', cursorFg: '#ffffff', cursorBg: '#005fb8' };
	const html = E.rowHtml(t, 0, options, -1, true);
	assert.equal(html, '<span style="color:#ffff55;background-color:#0000cc;font-weight:bold">AB</span><span style="color:#111111"> &lt;&amp;&gt;</span><span style="color:#ffffff;background-color:#111111">i</span>');
	const withCursor = E.rowHtml(t, 0, options, 3, true);
	assert.ok(withCursor.includes('<span style="color:#ffffff;background-color:#005fb8">&lt;</span>'));
	const full = E.rowHtml(t, 0, options, -1, false);
	assert.ok(full.endsWith('</span>') && full.length > html.length, 'a full row has its blanks');
	const bgRow = screenOf('\x1b[44m\x1b[K');
	assert.ok(E.rowHtml(bgRow, 0, options, -1, true).includes('background-color:#0000cc'), 'a row of background is not trimmed');
	// the default foreground on a dark background of the program is light, on a light one it is the foreground of the view
	const contrast = screenOf('[44ma[47mb[0m[7mc');
	const contrastHtml = E.rowHtml(contrast, 0, options, -1, true);
	assert.ok(contrastHtml.includes('<span style="color:#cccccc;background-color:#0000cc">a</span>'), 'light on blue');
	assert.ok(contrastHtml.includes('<span style="color:#111111;background-color:#cccccc">b</span>'), 'the foreground of the view on light gray');
	const wide = screenOf('中x');
	assert.equal(E.rowHtml(wide, 0, options, -1, true), '<span style="color:#111111">中x</span>');
});

// ── the keyboard ────────────────────────────────────────────────────────────

const K = {
	Escape: 0x01000000, Tab: 0x01000001, Backtab: 0x01000002, Backspace: 0x01000003, Return: 0x01000004, Enter: 0x01000005,
	Insert: 0x01000006, Delete: 0x01000007, Home: 0x01000010, End: 0x01000011, Left: 0x01000012, Up: 0x01000013, Right: 0x01000014,
	Down: 0x01000015, PageUp: 0x01000016, PageDown: 0x01000017, Shift: 0x01000020, Control: 0x01000021, Alt: 0x01000023,
	Space: 0x20,
};
for (let i = 1; i <= 20; i++) K['F' + i] = 0x0100002f + i;

function keys(spec, t = E.create(10, 2)) {
	return E.encodeKey(t, { text: '', ctrl: false, alt: false, shift: false, ...spec });
}

test('keys: the cursor keys, normal and application mode, with the modifiers', () => {
	assert.equal(keys({ key: K.Up }), '\x1b[A');
	assert.equal(keys({ key: K.Down }), '\x1b[B');
	assert.equal(keys({ key: K.Right }), '\x1b[C');
	assert.equal(keys({ key: K.Left }), '\x1b[D');
	assert.equal(keys({ key: K.Home }), '\x1b[H');
	assert.equal(keys({ key: K.End }), '\x1b[F');
	const app = screenOf('\x1b[?1h');
	assert.equal(keys({ key: K.Up }, app), '\x1bOA');
	assert.equal(keys({ key: K.End }, app), '\x1bOF');
	assert.equal(keys({ key: K.Up, ctrl: true }, app), '\x1b[1;5A', 'a modifier makes the CSI form even in the application mode');
	assert.equal(keys({ key: K.Right, ctrl: true }), '\x1b[1;5C');
	assert.equal(keys({ key: K.Left, shift: true }), '\x1b[1;2D');
	assert.equal(keys({ key: K.Down, alt: true }), '\x1b[1;3B');
	assert.equal(keys({ key: K.Home, ctrl: true, shift: true }), '\x1b[1;6H');
	assert.equal(keys({ key: K.End, ctrl: true, alt: true, shift: true }), '\x1b[1;8F');
});

test('keys: F1..F12 and the modified ones', () => {
	const plainKeys = ['\x1bOP', '\x1bOQ', '\x1bOR', '\x1bOS', '\x1b[15~', '\x1b[17~', '\x1b[18~', '\x1b[19~', '\x1b[20~', '\x1b[21~', '\x1b[23~', '\x1b[24~'];
	plainKeys.forEach((sequence, index) => assert.equal(keys({ key: K['F' + (index + 1)] }), sequence, 'F' + (index + 1)));
	assert.equal(keys({ key: K.F1, ctrl: true }), '\x1b[1;5P');
	assert.equal(keys({ key: K.F4, shift: true }), '\x1b[1;2S');
	assert.equal(keys({ key: K.F5, shift: true }), '\x1b[15;2~');
	assert.equal(keys({ key: K.F10, alt: true }), '\x1b[21;3~');
	assert.equal(keys({ key: K.F12, ctrl: true, shift: true }), '\x1b[24;6~');
	assert.equal(keys({ key: K.F13 }), '\x1b[25~');
	assert.equal(keys({ key: K.F20 }), '\x1b[34~');
});

test('keys: Insert, Delete, PageUp, PageDown with and without a modifier', () => {
	assert.equal(keys({ key: K.Insert }), '\x1b[2~');
	assert.equal(keys({ key: K.Delete }), '\x1b[3~');
	assert.equal(keys({ key: K.PageUp }), '\x1b[5~');
	assert.equal(keys({ key: K.PageDown }), '\x1b[6~');
	assert.equal(keys({ key: K.Delete, ctrl: true }), '\x1b[3;5~');
	assert.equal(keys({ key: K.PageUp, shift: true }), '\x1b[5;2~');
	assert.equal(keys({ key: K.Insert, alt: true }), '\x1b[2;3~');
});

test('keys: Tab, Backtab, Backspace, Enter and Escape', () => {
	assert.equal(keys({ key: K.Tab }), '\t');
	assert.equal(keys({ key: K.Tab, shift: true }), '\x1b[Z');
	assert.equal(keys({ key: K.Backtab }), '\x1b[Z');
	assert.equal(keys({ key: K.Tab, alt: true }), '\x1b\t');
	assert.equal(keys({ key: K.Backspace }), '\x7f');
	assert.equal(keys({ key: K.Backspace, ctrl: true }), '\x08');
	assert.equal(keys({ key: K.Backspace, alt: true }), '\x1b\x7f');
	assert.equal(keys({ key: K.Return }), '\r');
	assert.equal(keys({ key: K.Enter }), '\r');
	assert.equal(keys({ key: K.Return, alt: true }), '\x1b\r');
	assert.equal(keys({ key: K.Escape }), '\x1b');
	assert.equal(keys({ key: K.Escape, alt: true }), '\x1b\x1b');
});

test('keys: Ctrl with the letters and with the keys that make the other control characters', () => {
	for (let code = 0x41; code <= 0x5a; code++) assert.equal(keys({ key: code, ctrl: true }), String.fromCharCode(code - 0x40), String.fromCharCode(code));
	assert.equal(keys({ key: 0x4f, ctrl: true, shift: true }), '\x0f', 'Ctrl+Shift+O is Ctrl+O');
	assert.equal(keys({ key: 0x4f, ctrl: true, text: '\x0f' }), '\x0f', 'the text of Qt with Ctrl is not sent twice');
	assert.equal(keys({ key: K.Space, ctrl: true }), '\x00');
	assert.equal(keys({ key: 0x40, ctrl: true }), '\x00');
	assert.equal(keys({ key: 0x5b, ctrl: true }), '\x1b');
	assert.equal(keys({ key: 0x5c, ctrl: true }), '\x1c');
	assert.equal(keys({ key: 0x5d, ctrl: true }), '\x1d');
	assert.equal(keys({ key: 0x5e, ctrl: true }), '\x1e');
	assert.equal(keys({ key: 0x5f, ctrl: true }), '\x1f');
	assert.equal(keys({ key: 0x3f, ctrl: true }), '\x7f');
	assert.equal(keys({ key: 0x37, ctrl: true }), '\x1f');
	assert.equal(keys({ key: 0x31, ctrl: true }), '', 'Ctrl+1 has no character');
	assert.equal(keys({ key: 0x58, ctrl: true, alt: true }), '\x1b\x18', 'Alt+Ctrl+X');
});

test('keys: text, Alt in front of a character, and the keys that send nothing', () => {
	assert.equal(keys({ key: 0x41, text: 'a' }), 'a');
	assert.equal(keys({ key: 0x41, text: 'A', shift: true }), 'A');
	assert.equal(keys({ key: 0x416, text: 'ж' }), 'ж');
	assert.equal(keys({ key: 0x46, text: 'f', alt: true }), '\x1bf');
	assert.equal(keys({ key: K.Space, text: ' ' }), ' ');
	assert.equal(keys({ key: K.Space }), ' ');
	assert.equal(keys({ key: K.Space, alt: true, text: ' ' }), '\x1b ');
	assert.equal(keys({ key: K.Shift }), '');
	assert.equal(keys({ key: K.Control, ctrl: true }), '');
	assert.equal(keys({ key: K.Alt, alt: true }), '');
});

// ── the mouse, the paste, the focus and the selection ───────────────────────

function mouse(t, spec) {
	return E.encodeMouse(t, { button: 0, col: 0, row: 0, shift: false, alt: false, ctrl: false, ...spec });
}

test('mouse: nothing without a mouse mode', () => {
	const t = screenOf('');
	assert.equal(mouse(t, { action: 'press' }), '');
	assert.equal(mouse(t, { action: 'wheelUp' }), '');
});

test('mouse: the SGR form (?1006) with the buttons, the wheel, the modifiers and the release', () => {
	const t = screenOf('\x1b[?1000h\x1b[?1006h');
	assert.equal(mouse(t, { action: 'press', col: 4, row: 2 }), '\x1b[<0;5;3M');
	assert.equal(mouse(t, { action: 'release', col: 4, row: 2 }), '\x1b[<0;5;3m');
	assert.equal(mouse(t, { action: 'press', button: 2 }), '\x1b[<2;1;1M');
	assert.equal(mouse(t, { action: 'release', button: 2 }), '\x1b[<2;1;1m');
	assert.equal(mouse(t, { action: 'press', button: 1 }), '\x1b[<1;1;1M');
	assert.equal(mouse(t, { action: 'wheelUp', col: 1, row: 1 }), '\x1b[<64;2;2M');
	assert.equal(mouse(t, { action: 'wheelDown' }), '\x1b[<65;1;1M');
	assert.equal(mouse(t, { action: 'press', shift: true }), '\x1b[<4;1;1M');
	assert.equal(mouse(t, { action: 'press', alt: true }), '\x1b[<8;1;1M');
	assert.equal(mouse(t, { action: 'press', ctrl: true }), '\x1b[<16;1;1M');
	assert.equal(mouse(t, { action: 'press', col: 119, row: 37, shift: true, ctrl: true }), '\x1b[<20;120;38M');
});

test('mouse: the motion is reported as the mode says', () => {
	const buttons = screenOf('\x1b[?1000h\x1b[?1006h');
	assert.equal(mouse(buttons, { action: 'move', button: 0 }), '', '?1000 has no motion');
	const drag = screenOf('\x1b[?1002h\x1b[?1006h');
	assert.equal(mouse(drag, { action: 'move', button: 0, col: 2, row: 1 }), '\x1b[<32;3;2M', 'a drag');
	assert.equal(mouse(drag, { action: 'move', button: -1 }), '', '?1002: no motion without a button');
	const any = screenOf('\x1b[?1003h\x1b[?1006h');
	assert.equal(mouse(any, { action: 'move', button: -1, col: 5, row: 5 }), '\x1b[<35;6;6M');
	assert.equal(mouse(any, { action: 'move', button: 2 }), '\x1b[<34;1;1M');
	any.mouseMode = 0;
	E.feed(any, '\x1b[?1003l');
	assert.equal(mouse(any, { action: 'press' }), '');
});

test('mouse: the old form (no ?1006) is ESC [ M and three characters, and has a limit', () => {
	const t = screenOf('\x1b[?1000h');
	assert.equal(mouse(t, { action: 'press' }), '\x1b[M !!');
	assert.equal(mouse(t, { action: 'release' }), '\x1b[M#!!');
	assert.equal(mouse(t, { action: 'press', button: 2, col: 9, row: 4 }), '\x1b[M"*%');
	assert.equal(mouse(t, { action: 'wheelUp' }), '\x1b[M`!!');
	assert.equal(mouse(t, { action: 'press', col: 300 }), '', 'a column that cannot be said');
});

test('paste: new lines are Enter, and the bracketed form when the program asks for it', () => {
	const t = screenOf('');
	assert.equal(E.encodePaste(t, 'a\nb\r\nc'), 'a\rb\rc');
	const b = screenOf('\x1b[?2004h');
	assert.equal(E.encodePaste(b, 'a\nb'), '\x1b[200~a\rb\x1b[201~');
	assert.equal(E.encodePaste(b, 'x\x1b[201~y'), '\x1b[200~x[201~y\x1b[201~', 'the end of the paste cannot be sent inside it');
	E.feed(b, '\x1b[?2004l');
	assert.equal(E.encodePaste(b, 'a'), 'a');
});

test('focus: reported only when the program asked for it', () => {
	const t = screenOf('');
	assert.equal(E.encodeFocus(t, true), '');
	E.feed(t, '\x1b[?1004h');
	assert.equal(E.encodeFocus(t, true), '\x1b[I');
	assert.equal(E.encodeFocus(t, false), '\x1b[O');
});

test('selection: the columns of every row, in any direction, and the text', () => {
	const t = screenOf('abcdef\r\nghijkl\r\nmnopqr', 6, 4);
	const sel = { r1: 0, c1: 2, r2: 2, c2: 3 };
	assert.deepEqual(E.selectionColumns(sel, 0, 6), [2, 5]);
	assert.deepEqual(E.selectionColumns(sel, 1, 6), [0, 5]);
	assert.deepEqual(E.selectionColumns(sel, 2, 6), [0, 3]);
	assert.equal(E.selectionColumns(sel, 3, 6), null);
	assert.equal(E.selectionText(t, sel), 'cdef\nghijkl\nmnop');
	assert.equal(E.selectionText(t, { r1: 2, c1: 3, r2: 0, c2: 2 }), 'cdef\nghijkl\nmnop', 'the pointer went up');
	assert.equal(E.selectionText(t, { r1: 1, c1: 1, r2: 1, c2: 3 }), 'hij');
});

test('selection: the blanks at the end of a row are dropped, a wide character is one', () => {
	const t = screenOf('ab\r\n中x', 8, 3);
	assert.equal(E.selectionText(t, { r1: 0, c1: 0, r2: 1, c2: 7 }), 'ab\n中x');
	assert.equal(E.selectionText(t, { r1: 1, c1: 0, r2: 1, c2: 1 }), '中');
});

test('rowHtml: the selected columns have the selection colors, the cursor goes over them', () => {
	const t = screenOf('abcd');
	const options = { defaultFg: '#111111', defaultBg: '#ffffff', cursorFg: '#ffffff', cursorBg: '#005fb8', selectionFg: '#fefefe', selectionBg: '#336699', selectedFrom: 1, selectedTo: 2 };
	assert.equal(E.rowHtml(t, 0, options, -1, true), '<span style="color:#111111">a</span><span style="color:#fefefe;background-color:#336699">bc</span><span style="color:#111111">d</span>');
	assert.ok(E.rowHtml(t, 0, options, 2, true).includes('<span style="color:#ffffff;background-color:#005fb8">c</span>'));
});
