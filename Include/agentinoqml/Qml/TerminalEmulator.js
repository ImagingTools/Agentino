.pragma library

// The VT emulator of the terminal of Agentino: it has no UI and no QML types, so it runs in the QML (import "TerminalEmulator.js" as
// Emulator) and in node (the tests in Tests/TerminalEmulator read it as a text). The design is Docs/Architecture/TerminalFullScreenApps.md.
//
//   var t = create(cols, rows)       the state: a grid of cells, the cursor, the modes, the parser
//   feed(t, text)                    the output of the pty (a sequence cut by a chunk goes on in the next one)
//   takeReplies(t)                   what the terminal answers to the queries of the program (write it to the pty)
//   rowHtml(t, row, options)         the rich text of a row; rowText(t, row) the plain text
//   resize(t, cols, rows)            the grid changes its size and keeps what it can
//   encodeKey(t, key)                the bytes that a key of the keyboard sends to the pty (see below)
//   t.owned                          true while a program owns the screen: the alternate screen, or a program that draws the main one (see updateOwnership)
//   encodeMouse, encodePaste, encodeFocus   the bytes of a mouse event, of a paste and of the focus when the program asked for them
//   selectionText, selectionColumns  the text of a rectangle-free (line by line) selection of the grid and its columns in a row
//
// A cell is {ch, fg, bg, fl}: ch is the character ("" after a wide one), fg and bg are -1 (the default), a palette index 0..255 or
// 0x1000000 + 0xRRGGBB, fl is a mask of FL_* below. A cell is never changed once it is in the grid, a new one replaces it.

var FL_BOLD = 1;
var FL_DIM = 2;
var FL_ITALIC = 4;
var FL_UNDERLINE = 8;
var FL_BLINK = 16;
var FL_INVERSE = 32;
var FL_CONCEAL = 64;
var FL_STRIKE = 128;

var RGB_BASE = 0x1000000;

// The colors 0..15: the scheme "Campbell" of Windows Terminal and of the console of Windows 10/11, the one that a program of a Windows console is made to look
// right in (the blue panels and the cyan menu of mc.exe come out as they do there: measured on a screenshot of mc.exe in Windows Terminal, blue #0036d9 and
// cyan #378fdc against #0037da and #3a96dd of the scheme).
var ANSI16 = ["#0c0c0c", "#c50f1f", "#13a10e", "#c19c00", "#0037da", "#881798", "#3a96dd", "#cccccc",
	"#767676", "#e74856", "#16c60c", "#f9f1a5", "#3b78ff", "#b4009e", "#61d6d6", "#f2f2f2"];
var CUBE_LEVELS = [0, 95, 135, 175, 215, 255];

// The DEC special graphics set (ESC ( 0): the characters of the lines
var DEC_GRAPHICS = {
	"_": " ", "`": "◆", "a": "▒", "b": "␉", "c": "␌", "d": "␍", "e": "␊", "f": "°", "g": "±",
	"h": "␤", "i": "␋", "j": "┘", "k": "┐", "l": "┌", "m": "└", "n": "┼", "o": "⎺", "p": "⎻",
	"q": "─", "r": "⎼", "s": "⎽", "t": "├", "u": "┤", "v": "┴", "w": "┬", "x": "│", "y": "≤",
	"z": "≥", "{": "π", "|": "≠", "}": "£", "~": "·"
};

var S_GROUND = 0;
var S_ESCAPE = 1;
var S_ESC_INTERMEDIATE = 2;
var S_CSI = 3;
var S_OSC = 4;
var S_STRING = 5; // DCS, APC, PM and SOS: skipped up to the string terminator
var S_STRING_ESC = 6; // an ESC in a string: "\" ends it
var S_OSC_ESC = 7;

// ── colors ──────────────────────────────────────────────────────────────────

function paletteColor(index){
	if (index < 16){
		return ANSI16[index];
	}
	if (index < 232){
		var n = index - 16;
		return rgbString(CUBE_LEVELS[Math.floor(n / 36)], CUBE_LEVELS[Math.floor(n / 6) % 6], CUBE_LEVELS[n % 6]);
	}
	var level = 8 + (index - 232) * 10;
	return rgbString(level, level, level);
}

function rgbString(r, g, b){
	return "#" + hex2(r) + hex2(g) + hex2(b);
}

function hex2(value){
	var text = Math.max(0, Math.min(255, Math.round(value))).toString(16);
	return text.length < 2 ? "0" + text : text;
}

// The color of fg or bg as "#rrggbb", or null for the default.
function colorString(value){
	if (value < 0){
		return null;
	}
	if (value >= RGB_BASE){
		var rgb = value - RGB_BASE;
		return rgbString(rgb >> 16, (rgb >> 8) & 255, rgb & 255);
	}
	return paletteColor(value);
}

function parseHex(color){
	return [parseInt(color.substring(1, 3), 16), parseInt(color.substring(3, 5), 16), parseInt(color.substring(5, 7), 16)];
}

// Whether a color is dark enough for light text to read on it (the relative luminance is under one half).
function isDarkColor(color){
	var c = parseHex(color);
	return (0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]) < 128;
}

// The foreground of the text of the default foreground that stands on a background of a program: the default of the view is for the
// background of the view, so on a dark background of the program (the blue panels of mc) the light gray of the 16 colors is used.
var LIGHT_ON_DARK = "#cccccc";

// A color halfway to the other one (the dim attribute).
function mixColors(from, to){
	var a = parseHex(from);
	var b = parseHex(to);
	return rgbString((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2);
}

// ── the state ───────────────────────────────────────────────────────────────

function create(cols, rows){
	var t = {
		cols: 1,
		rows: 1,
		grid: [],
		cursorRow: 0,
		cursorCol: 0,
		cursorVisible: true,
		wrapPending: false,
		scrollTop: 0,
		scrollBottom: 0,
		fg: -1,
		bg: -1,
		fl: 0,
		saved: null,
		tabs: [],
		charsets: ["B", "B"],
		shifted: false,
		lastChar: "",
		title: "",
		altActive: false,
		savedPrimary: null,
		owned: false,
		scrolledOut: 0,
		version: 0,
		replies: [],
		onScrollOut: null,
		// the parser
		state: S_GROUND,
		params: "",
		intermediates: "",
		osc: "",
		high: 0,
		// the modes
		autoWrap: true,
		originMode: false,
		insertMode: false,
		newlineMode: false,
		appCursorKeys: false,
		bracketedPaste: false,
		mouseMode: 0,
		mouseSgr: false,
		focusEvents: false
	};
	resetScreen(t, cols, rows);
	return t;
}

// A row that changed forgets the rich text that rowHtml made for it. The rows are arrays that travel with the scroll, insert and delete of lines, so the
// text stays with the row; only a change of its cells (a character, an erase, an insert or a delete of characters) has to say so.
function touch(cells){
	cells.cacheKey = null;
}

function blankCell(t){
	return {ch: " ", fg: -1, bg: t.bg, fl: 0};
}

function blankRow(t, cols){
	var row = [];
	for (var c = 0; c < cols; ++c){
		row.push(blankCell(t));
	}
	return row;
}

function resetTabs(t){
	t.tabs = [];
	for (var c = 8; c < t.cols; c += 8){
		t.tabs.push(c);
	}
}

// A new empty screen of this size (the attributes are kept: the erase uses the background that is set).
function resetScreen(t, cols, rows){
	t.cols = Math.max(1, cols);
	t.rows = Math.max(1, rows);
	t.scrollTop = 0;
	t.scrollBottom = t.rows - 1;
	t.cursorRow = 0;
	t.cursorCol = 0;
	t.wrapPending = false;
	t.grid = [];
	for (var r = 0; r < t.rows; ++r){
		t.grid.push(blankRow(t, t.cols));
	}
	resetTabs(t);
	t.version++;
}

// The screen is emptied (with the background that is set) and the cursor goes home.
function clearScreen(t){
	resetScreen(t, t.cols, t.rows);
}

// RIS: everything is as at the start
function fullReset(t){
	t.scrolledOut = 0;
	t.owned = false;
	t.fg = -1;
	t.bg = -1;
	t.fl = 0;
	t.saved = null;
	t.charsets = ["B", "B"];
	t.shifted = false;
	t.cursorVisible = true;
	t.autoWrap = true;
	t.originMode = false;
	t.insertMode = false;
	t.newlineMode = false;
	t.appCursorKeys = false;
	t.bracketedPaste = false;
	t.mouseMode = 0;
	t.mouseSgr = false;
	t.focusEvents = false;
	resetScreen(t, t.cols, t.rows);
}

// The grid gets a new size; the rows and the cells that are still there are kept (the pty repaints the screen after a resize).
function resize(t, cols, rows){
	var newCols = Math.max(1, cols);
	var newRows = Math.max(1, rows);
	var saveBg = t.bg;
	t.bg = -1;
	var grid = [];
	for (var r = 0; r < newRows; ++r){
		var row = blankRow(t, newCols);
		var old = t.grid[r];
		if (old){
			for (var c = 0; c < newCols && c < old.length; ++c){
				row[c] = old[c];
			}
		}
		grid.push(row);
	}
	t.bg = saveBg;
	t.grid = grid;
	t.cols = newCols;
	t.rows = newRows;
	t.scrollTop = 0;
	t.scrollBottom = newRows - 1;
	t.cursorRow = Math.min(t.cursorRow, newRows - 1);
	t.cursorCol = Math.min(t.cursorCol, newCols - 1);
	t.wrapPending = false;
	resetTabs(t);
	updateOwnership(t);
	t.version++;
}

function takeReplies(t){
	var out = t.replies.join("");
	t.replies = [];
	return out;
}

// ── a program that owns the screen ──────────────────────────────────────────
// A program of the alternate screen (vim, htop) owns the screen as far as the terminal is told. Under ConPTY the alternate screen is not told (it is the
// main screen that is repainted with cursor addressing), and nothing in the stream tells such a program (mc.exe, vim, nano) from a shell that has written
// a screenful. What tells them apart is how the screen is written: a shell writes from the top down and the cursor is at the end of what it wrote, so
// there is nothing under the cursor and the lines go up into the history when the screen is full; a program that owns the screen writes its whole screen
// at places, with its status line under the cursor, and does not scroll the main screen while it draws. So a program owns the screen when
//   - nothing has scrolled off the top since the cursor was last put in the top row or the screen was last cleared (scrolledOut): a program starts
//     its screen at the top row, and ConPTY scrolls the old lines up when a program clears the screen, which must not count against it, and
//   - most of the rows are written (60% of them, and at least 5), and
//   - there is a written row under the cursor,
// and it keeps it while a quarter of the rows (at least 2) is written; the first line that scrolls off ends it. The view keeps a short delay before it
// gives the pane back, because a program may clear its screen and write it again in the next chunk.
function updateOwnership(t){
	if (t.altActive){
		t.owned = true;
		return;
	}
	if (t.scrolledOut > 0){
		t.owned = false;
		return;
	}
	var used = 0;
	var lastUsed = -1;
	for (var r = 0; r < t.rows; ++r){
		if (lastUsedColumn(t.grid[r]) >= 0){
			used++;
			lastUsed = r;
		}
	}
	if (t.owned){
		t.owned = used >= Math.max(2, Math.floor(t.rows * 0.25));
	}
	else{
		t.owned = lastUsed > t.cursorRow && used >= Math.max(5, Math.ceil(t.rows * 0.6));
	}
}

// ── the alternate screen ────────────────────────────────────────────────────

function enterAltScreen(t, saveCursor){
	if (t.altActive){
		return;
	}
	t.savedPrimary = {grid: t.grid, cols: t.cols, rows: t.rows, cursorRow: t.cursorRow, cursorCol: t.cursorCol, fg: t.fg, bg: t.bg, fl: t.fl,
		wrapPending: t.wrapPending, scrollTop: t.scrollTop, scrollBottom: t.scrollBottom};
	t.altActive = true;
	t.fg = -1;
	t.bg = -1;
	t.fl = 0;
	resetScreen(t, t.cols, t.rows);
}

function exitAltScreen(t){
	if (!t.altActive){
		return;
	}
	t.altActive = false;
	var saved = t.savedPrimary;
	t.savedPrimary = null;
	var cols = t.cols;
	var rows = t.rows;
	if (saved === null){
		resetScreen(t, cols, rows);
		return;
	}
	t.grid = saved.grid;
	t.cols = saved.cols;
	t.rows = saved.rows;
	t.cursorRow = saved.cursorRow;
	t.cursorCol = saved.cursorCol;
	t.fg = saved.fg;
	t.bg = saved.bg;
	t.fl = saved.fl;
	t.wrapPending = false;
	t.scrollTop = saved.scrollTop;
	t.scrollBottom = saved.scrollBottom;
	resize(t, cols, rows);
}

// ── the grid ────────────────────────────────────────────────────────────────

function scrollUp(t){
	if (!t.altActive && t.scrollTop === 0){
		t.scrolledOut++;
		if (t.onScrollOut !== null){
			t.onScrollOut(t.grid[0]);
		}
	}
	t.grid.splice(t.scrollTop, 1);
	t.grid.splice(t.scrollBottom, 0, blankRow(t, t.cols));
}

function scrollDown(t){
	t.grid.splice(t.scrollBottom, 1);
	t.grid.splice(t.scrollTop, 0, blankRow(t, t.cols));
}

function lineFeed(t){
	if (t.cursorRow === t.scrollBottom){
		scrollUp(t);
	}
	else if (t.cursorRow < t.rows - 1){
		t.cursorRow++;
	}
}

function reverseIndex(t){
	if (t.cursorRow === t.scrollTop){
		scrollDown(t);
	}
	else if (t.cursorRow > 0){
		t.cursorRow--;
	}
}

function clampCursor(t){
	t.cursorRow = Math.max(0, Math.min(t.rows - 1, t.cursorRow));
	t.cursorCol = Math.max(0, Math.min(t.cols - 1, t.cursorCol));
}

function setCursor(t, row, col){
	t.wrapPending = false;
	if (t.originMode){
		t.cursorRow = Math.max(t.scrollTop, Math.min(t.scrollBottom, t.scrollTop + row));
	}
	else{
		t.cursorRow = Math.max(0, Math.min(t.rows - 1, row));
	}
	t.cursorCol = Math.max(0, Math.min(t.cols - 1, col));
}

function charWidth(code){
	if (code < 0x300){
		return 1;
	}
	if ((code >= 0x300 && code <= 0x36f) || (code >= 0x200b && code <= 0x200f) || (code >= 0xfe00 && code <= 0xfe0f) || (code >= 0x20d0 && code <= 0x20ff) || code === 0xfeff){
		return 0;
	}
	if ((code >= 0x1100 && code <= 0x115f) || (code >= 0x2e80 && code <= 0xa4cf && code !== 0x303f) || (code >= 0xac00 && code <= 0xd7a3)
			|| (code >= 0xf900 && code <= 0xfaff) || (code >= 0xfe30 && code <= 0xfe6f) || (code >= 0xff00 && code <= 0xff60)
			|| (code >= 0xffe0 && code <= 0xffe6) || (code >= 0x1f300 && code <= 0x1f64f) || (code >= 0x1f900 && code <= 0x1f9ff)
			|| (code >= 0x20000 && code <= 0x3fffd)){
		return 2;
	}
	return 1;
}

// A printable character at the cursor (a deferred wrap as in VT100: a character in the last column leaves the cursor there and the next one wraps).
function putChar(t, text, code){
	var width = charWidth(code);
	if (width === 0){
		// a combining character belongs to the cell before the cursor
		var row0 = t.grid[t.cursorRow];
		var at = t.wrapPending ? t.cursorCol : t.cursorCol - 1;
		while (row0 && at > 0 && row0[at].ch === ""){
			at--;
		}
		if (row0 && at >= 0){
			var prev = row0[at];
			row0[at] = {ch: prev.ch + text, fg: prev.fg, bg: prev.bg, fl: prev.fl};
			touch(row0);
		}
		return;
	}
	if (t.wrapPending){
		t.wrapPending = false;
		if (t.autoWrap){
			t.cursorCol = 0;
			lineFeed(t);
		}
	}
	if (width === 2 && t.cursorCol >= t.cols - 1){
		// a wide character does not fit at the end of the line
		if (t.autoWrap){
			t.grid[t.cursorRow][t.cursorCol] = blankCell(t);
			touch(t.grid[t.cursorRow]);
			t.cursorCol = 0;
			lineFeed(t);
		}
		else{
			return;
		}
	}
	var row = t.grid[t.cursorRow];
	touch(row);
	if (t.insertMode){
		for (var i = 0; i < width; ++i){
			row.splice(t.cursorCol, 0, blankCell(t));
			row.pop();
		}
	}
	row[t.cursorCol] = {ch: text, fg: t.fg, bg: t.bg, fl: t.fl};
	if (width === 2){
		row[t.cursorCol + 1] = {ch: "", fg: t.fg, bg: t.bg, fl: t.fl};
	}
	t.lastChar = text;
	var next = t.cursorCol + width;
	if (next >= t.cols){
		t.cursorCol = t.cols - 1;
		t.wrapPending = true;
	}
	else{
		t.cursorCol = next;
	}
}

function eraseCells(t, row, from, to){
	var cells = t.grid[row];
	touch(cells);
	for (var c = Math.max(0, from); c <= to && c < t.cols; ++c){
		cells[c] = blankCell(t);
	}
}

function eraseInLine(t, mode){
	if (mode === 1){
		eraseCells(t, t.cursorRow, 0, t.cursorCol);
	}
	else if (mode === 2){
		eraseCells(t, t.cursorRow, 0, t.cols - 1);
	}
	else{
		eraseCells(t, t.cursorRow, t.cursorCol, t.cols - 1);
	}
}

function eraseInDisplay(t, mode){
	var r;
	if (mode === 2 || mode === 3){
		t.scrolledOut = 0;
		for (r = 0; r < t.rows; ++r){
			t.grid[r] = blankRow(t, t.cols);
		}
	}
	else if (mode === 1){
		for (r = 0; r < t.cursorRow; ++r){
			t.grid[r] = blankRow(t, t.cols);
		}
		eraseInLine(t, 1);
	}
	else{
		eraseInLine(t, 0);
		for (r = t.cursorRow + 1; r < t.rows; ++r){
			t.grid[r] = blankRow(t, t.cols);
		}
	}
}

function insertLines(t, n){
	if (t.cursorRow < t.scrollTop || t.cursorRow > t.scrollBottom){
		return;
	}
	for (var i = 0; i < n; ++i){
		t.grid.splice(t.scrollBottom, 1);
		t.grid.splice(t.cursorRow, 0, blankRow(t, t.cols));
	}
	t.cursorCol = 0;
	t.wrapPending = false;
}

function deleteLines(t, n){
	if (t.cursorRow < t.scrollTop || t.cursorRow > t.scrollBottom){
		return;
	}
	for (var i = 0; i < n; ++i){
		t.grid.splice(t.cursorRow, 1);
		t.grid.splice(t.scrollBottom, 0, blankRow(t, t.cols));
	}
	t.cursorCol = 0;
	t.wrapPending = false;
}

function deleteChars(t, n){
	var row = t.grid[t.cursorRow];
	touch(row);
	var count = Math.min(n, t.cols - t.cursorCol);
	row.splice(t.cursorCol, count);
	for (var i = 0; i < count; ++i){
		row.push(blankCell(t));
	}
}

function insertChars(t, n){
	var row = t.grid[t.cursorRow];
	touch(row);
	var count = Math.min(n, t.cols - t.cursorCol);
	for (var i = 0; i < count; ++i){
		row.splice(t.cursorCol, 0, blankCell(t));
		row.pop();
	}
}

function tabForward(t, n){
	for (var i = 0; i < n; ++i){
		var next = t.cols - 1;
		for (var k = 0; k < t.tabs.length; ++k){
			if (t.tabs[k] > t.cursorCol){
				next = t.tabs[k];
				break;
			}
		}
		t.cursorCol = Math.min(t.cols - 1, next);
	}
	t.wrapPending = false;
}

function tabBackward(t, n){
	for (var i = 0; i < n; ++i){
		var prev = 0;
		for (var k = t.tabs.length - 1; k >= 0; --k){
			if (t.tabs[k] < t.cursorCol){
				prev = t.tabs[k];
				break;
			}
		}
		t.cursorCol = prev;
	}
	t.wrapPending = false;
}

// ── SGR ─────────────────────────────────────────────────────────────────────

function sgr(t, params){
	if (params.length === 0){
		params = [0];
	}
	for (var i = 0; i < params.length; ++i){
		var p = params[i];
		if (p === 0){
			t.fg = -1;
			t.bg = -1;
			t.fl = 0;
		}
		else if (p === 1){
			t.fl |= FL_BOLD;
		}
		else if (p === 2){
			t.fl |= FL_DIM;
		}
		else if (p === 3){
			t.fl |= FL_ITALIC;
		}
		else if (p === 4 || p === 21){
			t.fl |= FL_UNDERLINE;
		}
		else if (p === 5 || p === 6){
			t.fl |= FL_BLINK;
		}
		else if (p === 7){
			t.fl |= FL_INVERSE;
		}
		else if (p === 8){
			t.fl |= FL_CONCEAL;
		}
		else if (p === 9){
			t.fl |= FL_STRIKE;
		}
		else if (p === 22){
			t.fl &= ~(FL_BOLD | FL_DIM);
		}
		else if (p === 23){
			t.fl &= ~FL_ITALIC;
		}
		else if (p === 24){
			t.fl &= ~FL_UNDERLINE;
		}
		else if (p === 25){
			t.fl &= ~FL_BLINK;
		}
		else if (p === 27){
			t.fl &= ~FL_INVERSE;
		}
		else if (p === 28){
			t.fl &= ~FL_CONCEAL;
		}
		else if (p === 29){
			t.fl &= ~FL_STRIKE;
		}
		else if (p >= 30 && p <= 37){
			t.fg = p - 30;
		}
		else if (p === 39){
			t.fg = -1;
		}
		else if (p >= 40 && p <= 47){
			t.bg = p - 40;
		}
		else if (p === 49){
			t.bg = -1;
		}
		else if (p >= 90 && p <= 97){
			t.fg = p - 90 + 8;
		}
		else if (p >= 100 && p <= 107){
			t.bg = p - 100 + 8;
		}
		else if (p === 38 || p === 48){
			var color = -2;
			if (params[i + 1] === 5 && i + 2 < params.length){
				color = Math.max(0, Math.min(255, params[i + 2]));
				i += 2;
			}
			else if (params[i + 1] === 2 && i + 4 < params.length){
				color = RGB_BASE + (((params[i + 2] & 255) << 16) | ((params[i + 3] & 255) << 8) | (params[i + 4] & 255));
				i += 4;
			}
			if (color !== -2){
				if (p === 38){
					t.fg = color;
				}
				else{
					t.bg = color;
				}
			}
		}
	}
}

// ── modes ───────────────────────────────────────────────────────────────────

function setPrivateMode(t, mode, enable){
	if (mode === 1){
		t.appCursorKeys = enable;
	}
	else if (mode === 6){
		t.originMode = enable;
		setCursor(t, 0, 0);
	}
	else if (mode === 7){
		t.autoWrap = enable;
	}
	else if (mode === 25){
		t.cursorVisible = enable;
	}
	else if (mode === 47 || mode === 1047 || mode === 1049){
		if (enable){
			enterAltScreen(t, mode === 1049);
		}
		else{
			exitAltScreen(t);
		}
	}
	else if (mode === 1000 || mode === 1002 || mode === 1003){
		t.mouseMode = enable ? mode : 0;
	}
	else if (mode === 1006){
		t.mouseSgr = enable;
	}
	else if (mode === 1004){
		t.focusEvents = enable;
	}
	else if (mode === 2004){
		t.bracketedPaste = enable;
	}
}

function setAnsiMode(t, mode, enable){
	if (mode === 4){
		t.insertMode = enable;
	}
	else if (mode === 20){
		t.newlineMode = enable;
	}
}

// ── the sequences ───────────────────────────────────────────────────────────

function parseParams(text){
	if (text.length === 0){
		return [];
	}
	var parts = text.split(/[;:]/);
	var out = [];
	for (var i = 0; i < parts.length; ++i){
		var value = parseInt(parts[i], 10);
		out.push(isNaN(value) ? 0 : value);
	}
	return out;
}

function csi(t, paramsText, intermediates, finalChar){
	var privateMark = "";
	var body = paramsText;
	if (body.length > 0 && "?<=>".indexOf(body.charAt(0)) >= 0){
		privateMark = body.charAt(0);
		body = body.substring(1);
	}
	var nums = parseParams(body);
	var p = function(index, def){
		return (nums.length > index && nums[index] > 0) ? nums[index] : def;
	};
	if (finalChar !== "m"){
		// every sequence but the attributes ends a pending wrap
		t.wrapPending = false;
	}

	if (privateMark === "?"){
		if (finalChar === "h" || finalChar === "l"){
			for (var k = 0; k < nums.length; ++k){
				setPrivateMode(t, nums[k], finalChar === "h");
			}
		}
		return;
	}
	if (privateMark === ">"){
		if (finalChar === "c"){
			t.replies.push("\x1b[>0;0;0c");
		}
		return;
	}
	if (privateMark !== "" || intermediates.length > 0){
		if (intermediates === " " && finalChar === "q"){
			return; // the shape of the cursor
		}
		if (intermediates === "!" && finalChar === "p"){
			fullReset(t); // DECSTR: close enough to a reset
		}
		return;
	}

	var n;
	switch (finalChar){
	case "A":
		t.cursorRow = Math.max(t.originMode ? t.scrollTop : 0, t.cursorRow - p(0, 1));
		break;
	case "B":
	case "e":
		t.cursorRow = Math.min(t.originMode ? t.scrollBottom : t.rows - 1, t.cursorRow + p(0, 1));
		break;
	case "C":
	case "a":
		t.cursorCol = Math.min(t.cols - 1, t.cursorCol + p(0, 1));
		break;
	case "D":
		t.cursorCol = Math.max(0, t.cursorCol - p(0, 1));
		break;
	case "E":
		t.cursorRow = Math.min(t.rows - 1, t.cursorRow + p(0, 1));
		t.cursorCol = 0;
		break;
	case "F":
		t.cursorRow = Math.max(0, t.cursorRow - p(0, 1));
		t.cursorCol = 0;
		break;
	case "G":
	case "`":
		t.cursorCol = Math.max(0, Math.min(t.cols - 1, p(0, 1) - 1));
		break;
	case "H":
	case "f":
		setCursor(t, p(0, 1) - 1, p(1, 1) - 1);
		if (p(0, 1) === 1){
			// the cursor goes to the top row: a program starts to draw its screen (see updateOwnership)
			t.scrolledOut = 0;
		}
		break;
	case "d":
		setCursor(t, p(0, 1) - 1, t.cursorCol);
		break;
	case "I":
		tabForward(t, p(0, 1));
		break;
	case "Z":
		tabBackward(t, p(0, 1));
		break;
	case "J":
		eraseInDisplay(t, nums.length > 0 ? nums[0] : 0);
		break;
	case "K":
		eraseInLine(t, nums.length > 0 ? nums[0] : 0);
		break;
	case "X":
		eraseCells(t, t.cursorRow, t.cursorCol, t.cursorCol + p(0, 1) - 1);
		break;
	case "L":
		insertLines(t, p(0, 1));
		break;
	case "M":
		deleteLines(t, p(0, 1));
		break;
	case "P":
		deleteChars(t, p(0, 1));
		break;
	case "@":
		insertChars(t, p(0, 1));
		break;
	case "S":
		for (n = 0; n < p(0, 1); ++n){
			scrollUp(t);
		}
		break;
	case "T":
		for (n = 0; n < p(0, 1); ++n){
			scrollDown(t);
		}
		break;
	case "b":
		if (t.lastChar.length > 0){
			var count = Math.min(p(0, 1), t.cols * t.rows);
			for (n = 0; n < count; ++n){
				putChar(t, t.lastChar, t.lastChar.codePointAt(0));
			}
		}
		break;
	case "r":
		var top = p(0, 1) - 1;
		var bottom = p(1, t.rows) - 1;
		if (top < bottom && bottom < t.rows){
			t.scrollTop = top;
			t.scrollBottom = bottom;
			setCursor(t, 0, 0);
		}
		break;
	case "s":
		saveCursor(t);
		break;
	case "u":
		restoreCursor(t);
		break;
	case "g":
		if (nums.length === 0 || nums[0] === 0){
			removeTab(t, t.cursorCol);
		}
		else if (nums[0] === 3){
			t.tabs = [];
		}
		break;
	case "m":
		sgr(t, nums);
		break;
	case "h":
	case "l":
		for (var m = 0; m < nums.length; ++m){
			setAnsiMode(t, nums[m], finalChar === "h");
		}
		break;
	case "c":
		if (nums.length === 0 || nums[0] === 0){
			t.replies.push("\x1b[?1;2c");
		}
		break;
	case "n":
		if (nums[0] === 5){
			t.replies.push("\x1b[0n");
		}
		else if (nums[0] === 6){
			var row = t.originMode ? t.cursorRow - t.scrollTop : t.cursorRow;
			t.replies.push("\x1b[" + (row + 1) + ";" + (t.cursorCol + 1) + "R");
		}
		break;
	case "t":
		// ConPTY says the size that the following repaint is drawn for: the grid follows it
		if (nums.length >= 3 && nums[0] === 8 && nums[1] > 0 && nums[2] > 0){
			resize(t, nums[2], nums[1]);
		}
		else if (nums[0] === 18){
			t.replies.push("\x1b[8;" + t.rows + ";" + t.cols + "t");
		}
		break;
	default:
		break;
	}
}

function removeTab(t, col){
	var at = t.tabs.indexOf(col);
	if (at >= 0){
		t.tabs.splice(at, 1);
	}
}

function addTab(t, col){
	if (t.tabs.indexOf(col) < 0){
		t.tabs.push(col);
		t.tabs.sort(function(a, b){ return a - b; });
	}
}

function saveCursor(t){
	t.saved = {row: t.cursorRow, col: t.cursorCol, fg: t.fg, bg: t.bg, fl: t.fl, charsets: t.charsets.slice(), shifted: t.shifted, wrapPending: t.wrapPending, originMode: t.originMode};
}

function restoreCursor(t){
	var s = t.saved;
	if (s === null){
		t.cursorRow = 0;
		t.cursorCol = 0;
		t.wrapPending = false;
		return;
	}
	t.cursorRow = Math.min(s.row, t.rows - 1);
	t.cursorCol = Math.min(s.col, t.cols - 1);
	t.fg = s.fg;
	t.bg = s.bg;
	t.fl = s.fl;
	t.charsets = s.charsets.slice();
	t.shifted = s.shifted;
	t.wrapPending = s.wrapPending;
	t.originMode = s.originMode;
}

function escape(t, intermediates, finalChar){
	if (intermediates.length > 0){
		var first = intermediates.charAt(0);
		if (first === "(" || first === ")" || first === "*" || first === "+"){
			var slot = first === "(" ? 0 : (first === ")" ? 1 : -1);
			if (slot >= 0){
				t.charsets[slot] = finalChar;
			}
		}
		else if (intermediates === "#" && finalChar === "8"){
			for (var r = 0; r < t.rows; ++r){
				touch(t.grid[r]);
				for (var c = 0; c < t.cols; ++c){
					t.grid[r][c] = {ch: "E", fg: -1, bg: -1, fl: 0};
				}
			}
		}
		return;
	}
	switch (finalChar){
	case "7":
		saveCursor(t);
		break;
	case "8":
		restoreCursor(t);
		break;
	case "c":
		fullReset(t);
		break;
	case "D":
		lineFeed(t);
		break;
	case "E":
		t.cursorCol = 0;
		lineFeed(t);
		break;
	case "H":
		addTab(t, t.cursorCol);
		break;
	case "M":
		reverseIndex(t);
		break;
	default:
		break;
	}
}

function oscEnd(t){
	var text = t.osc;
	var semi = text.indexOf(";");
	var code = semi >= 0 ? text.substring(0, semi) : text;
	if (code === "0" || code === "2"){
		t.title = semi >= 0 ? text.substring(semi + 1) : "";
	}
	t.osc = "";
}

// Feeds the output of the pty; the sequences that a chunk cuts go on with the next chunk.
function feed(t, text){
	var n = text.length;
	for (var i = 0; i < n; ++i){
		var code = text.charCodeAt(i);
		var ch = text.charAt(i);

		// a surrogate pair is one character
		if (t.state === S_GROUND && code >= 0xd800 && code <= 0xdbff){
			if (i + 1 < n){
				ch = text.substring(i, i + 2);
				code = text.codePointAt(i);
				i++;
			}
			else{
				t.high = code;
				continue;
			}
		}
		else if (t.high !== 0 && t.state === S_GROUND && code >= 0xdc00 && code <= 0xdfff){
			ch = String.fromCharCode(t.high, code);
			code = ch.codePointAt(0);
			t.high = 0;
		}
		else{
			t.high = 0;
		}

		// the controls that work in every state
		if (code === 0x18 || code === 0x1a){
			t.state = S_GROUND;
			continue;
		}
		if (code === 0x1b){
			if (t.state === S_OSC){
				t.state = S_OSC_ESC;
			}
			else if (t.state === S_STRING){
				t.state = S_STRING_ESC;
			}
			else{
				t.state = S_ESCAPE;
				t.params = "";
				t.intermediates = "";
			}
			continue;
		}

		switch (t.state){
		case S_GROUND:
			if (code < 0x20){
				control(t, code);
			}
			else if (code === 0x7f){
				// ignored
			}
			else{
				var shown = ch;
				if (code < 0x80){
					var set = t.charsets[t.shifted ? 1 : 0];
					if (set === "0" && DEC_GRAPHICS[ch] !== undefined){
						shown = DEC_GRAPHICS[ch];
						code = shown.charCodeAt(0);
					}
				}
				putChar(t, shown, code);
			}
			break;
		case S_ESCAPE:
			if (ch === "["){
				t.state = S_CSI;
				t.params = "";
				t.intermediates = "";
			}
			else if (ch === "]"){
				t.state = S_OSC;
				t.osc = "";
			}
			else if (ch === "P" || ch === "_" || ch === "^" || ch === "X"){
				t.state = S_STRING;
			}
			else if (code >= 0x20 && code <= 0x2f){
				t.intermediates += ch;
				t.state = S_ESC_INTERMEDIATE;
			}
			else if (ch === "\\"){
				t.state = S_GROUND;
			}
			else{
				t.state = S_GROUND;
				if (code >= 0x30){
					escape(t, "", ch);
				}
			}
			break;
		case S_ESC_INTERMEDIATE:
			if (code >= 0x20 && code <= 0x2f){
				t.intermediates += ch;
			}
			else{
				t.state = S_GROUND;
				escape(t, t.intermediates, ch);
			}
			break;
		case S_CSI:
			if ((code >= 0x30 && code <= 0x3f)){
				t.params += ch;
			}
			else if (code >= 0x20 && code <= 0x2f){
				t.intermediates += ch;
			}
			else if (code >= 0x40 && code <= 0x7e){
				t.state = S_GROUND;
				csi(t, t.params, t.intermediates, ch);
			}
			else if (code < 0x20){
				control(t, code); // a control inside a sequence is executed and the sequence goes on
			}
			else{
				t.state = S_GROUND;
			}
			break;
		case S_OSC:
			if (code === 0x07){
				t.state = S_GROUND;
				oscEnd(t);
			}
			else if (t.osc.length < 4096){
				t.osc += ch;
			}
			break;
		case S_OSC_ESC:
			t.state = S_GROUND;
			oscEnd(t);
			if (ch !== "\\"){
				t.state = S_ESCAPE;
				i--;
			}
			break;
		case S_STRING:
			if (code === 0x07){
				t.state = S_GROUND;
			}
			break;
		case S_STRING_ESC:
			t.state = S_GROUND;
			break;
		default:
			t.state = S_GROUND;
			break;
		}
	}
	updateOwnership(t);
	t.version++;
}

function control(t, code){
	switch (code){
	case 0x08:
		t.wrapPending = false;
		if (t.cursorCol > 0){
			t.cursorCol--;
		}
		break;
	case 0x09:
		tabForward(t, 1);
		break;
	case 0x0a:
	case 0x0b:
	case 0x0c:
		t.wrapPending = false;
		lineFeed(t);
		if (t.newlineMode){
			t.cursorCol = 0;
		}
		break;
	case 0x0d:
		t.wrapPending = false;
		t.cursorCol = 0;
		break;
	case 0x0e:
		t.shifted = true;
		break;
	case 0x0f:
		t.shifted = false;
		break;
	default:
		break;
	}
}

// ── what the view reads ─────────────────────────────────────────────────────

function isBlankCell(cell){
	return cell.ch === " " && cell.bg < 0 && (cell.fl & (FL_INVERSE | FL_UNDERLINE | FL_STRIKE)) === 0;
}

// The last column that shows something (a character, a background, a line), or -1.
function lastUsedColumn(row){
	if (!row){
		return -1;
	}
	for (var c = row.length - 1; c >= 0; --c){
		if (!isBlankCell(row[c])){
			return c;
		}
	}
	return -1;
}

// The plain text of a row without the blanks at its end.
function rowText(t, row){
	var cells = typeof row === "number" ? t.grid[row] : row;
	if (!cells){
		return "";
	}
	var end = -1;
	for (var c = cells.length - 1; c >= 0; --c){
		if (cells[c].ch !== " " && cells[c].ch !== ""){
			end = c;
			break;
		}
	}
	var text = "";
	for (var i = 0; i <= end; ++i){
		text += cells[i].ch;
	}
	return text;
}

function escapeHtml(text){
	return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

// "#rrggbb" from what the view gives (a color of QML may come as "#aarrggbb"), or the fallback.
function hexColor(value, fallback){
	var text = String(value).toLowerCase();
	if (/^#[0-9a-f]{6}$/.test(text)){
		return text;
	}
	if (/^#[0-9a-f]{8}$/.test(text)){
		return "#" + text.substring(3);
	}
	return fallback;
}

// The style of one cell. options: defaultFg and defaultBg ("#rrggbb", the colors of the view), cursorFg and cursorBg (the cursor cell), selectionFg and
// selectionBg with selectedFrom and selectedTo (the columns of the selection in this row, both included).
function cellStyle(cell, options, isCursor, isSelected){
	var fg = colorString(cell.fg);
	var bg = colorString(cell.bg);
	var defaultFg = options.defaultFg;
	var defaultBg = options.defaultBg;
	if (isCursor){
		return "color:" + options.cursorFg + ";background-color:" + options.cursorBg + (cell.fl & FL_BOLD ? ";font-weight:bold" : "");
	}
	if (isSelected){
		return "color:" + options.selectionFg + ";background-color:" + options.selectionBg + (cell.fl & FL_BOLD ? ";font-weight:bold" : "");
	}
	if (cell.fl & FL_INVERSE){
		var swap = fg !== null ? fg : defaultFg;
		fg = bg !== null ? bg : defaultBg;
		bg = swap;
	}
	else if (fg === null){
		fg = (bg !== null && isDarkColor(bg)) ? LIGHT_ON_DARK : defaultFg;
	}
	if (cell.fl & FL_DIM){
		fg = mixColors(fg, bg !== null ? bg : defaultBg);
	}
	if (cell.fl & FL_CONCEAL){
		fg = bg !== null ? bg : defaultBg;
	}
	var style = "color:" + fg;
	if (bg !== null){
		style += ";background-color:" + bg;
	}
	if (cell.fl & FL_BOLD){
		style += ";font-weight:bold";
	}
	if (cell.fl & FL_ITALIC){
		style += ";font-style:italic";
	}
	if ((cell.fl & (FL_UNDERLINE | FL_STRIKE)) !== 0){
		style += ";text-decoration:" + ((cell.fl & FL_UNDERLINE) ? "underline" : "") + ((cell.fl & FL_UNDERLINE) && (cell.fl & FL_STRIKE) ? " " : "") + ((cell.fl & FL_STRIKE) ? "line-through" : "");
	}
	return style;
}

// The rich text of a row (spans of the cells with the same style, no wrapper). cursorCol >= 0 draws the cursor cell; trim: the cells
// after the last one that shows something are left out (a row of the scrollback), otherwise the whole row is made (a row of a full-screen view).
// The text is kept on the row (cacheKey, cacheHtml) with what it was made for (the options, the cursor, the selection, trim) until a change of the cells
// of the row (touch): of a screen of a program only the rows that it wrote to are made again.
function rowHtml(t, row, options, cursorCol, trim){
	var cells = typeof row === "number" ? t.grid[row] : row;
	if (!cells){
		return "";
	}
	var key = (trim ? "t" : "f") + "|" + cursorCol + "|" + options.defaultFg + "|" + options.defaultBg + "|" + options.cursorFg + "|" + options.cursorBg
		+ "|" + options.selectionFg + "|" + options.selectionBg + "|" + options.selectedFrom + "|" + options.selectedTo;
	if (cells.cacheKey === key){
		return cells.cacheHtml;
	}
	var html = buildRowHtml(cells, options, cursorCol, trim);
	cells.cacheKey = key;
	cells.cacheHtml = html;
	return html;
}

// rowHtml without the kept text.
function buildRowHtml(cells, options, cursorCol, trim){
	options = {
		defaultFg: hexColor(options.defaultFg, "#000000"),
		defaultBg: hexColor(options.defaultBg, "#ffffff"),
		cursorFg: hexColor(options.cursorFg, "#ffffff"),
		cursorBg: hexColor(options.cursorBg, "#005fb8"),
		selectionFg: hexColor(options.selectionFg, "#ffffff"),
		selectionBg: hexColor(options.selectionBg, "#005fb8"),
		selectedFrom: options.selectedFrom === undefined ? -1 : options.selectedFrom,
		selectedTo: options.selectedTo === undefined ? -1 : options.selectedTo
	};
	var end = trim ? Math.max(lastUsedColumn(cells), cursorCol) : cells.length - 1;
	var html = "";
	var open = false;
	var current = "";
	// the style of the cells that are neither the cursor nor selected depends on three numbers: it is made once for each of the combinations in the row
	var styles = {};
	for (var c = 0; c <= end; ++c){
		var cell = cells[c];
		if (!cell){
			cell = {ch: " ", fg: -1, bg: -1, fl: 0};
		}
		if (cell.ch === ""){
			continue; // the second half of a wide character
		}
		var style;
		if (c === cursorCol || (c >= options.selectedFrom && c <= options.selectedTo)){
			style = cellStyle(cell, options, c === cursorCol, c >= options.selectedFrom && c <= options.selectedTo);
		}
		else{
			var styleKey = cell.fg + "," + cell.bg + "," + cell.fl;
			style = styles[styleKey];
			if (style === undefined){
				style = cellStyle(cell, options, false, false);
				styles[styleKey] = style;
			}
		}
		if (!open || style !== current){
			if (open){
				html += "</span>";
			}
			html += "<span style=\"" + style + "\">";
			open = true;
			current = style;
		}
		html += escapeHtml(cell.ch);
	}
	if (open){
		html += "</span>";
	}
	return html;
}

// ── the keyboard ────────────────────────────────────────────────────────────

// The Qt key codes (the same numbers are in desktop Qt and in the JQML engine) of the keys that are not text.
var KEY_ESCAPE = 0x01000000;
var KEY_TAB = 0x01000001;
var KEY_BACKTAB = 0x01000002;
var KEY_BACKSPACE = 0x01000003;
var KEY_RETURN = 0x01000004;
var KEY_ENTER = 0x01000005;
var KEY_INSERT = 0x01000006;
var KEY_DELETE = 0x01000007;
var KEY_HOME = 0x01000010;
var KEY_END = 0x01000011;
var KEY_LEFT = 0x01000012;
var KEY_UP = 0x01000013;
var KEY_RIGHT = 0x01000014;
var KEY_DOWN = 0x01000015;
var KEY_PAGEUP = 0x01000016;
var KEY_PAGEDOWN = 0x01000017;
var KEY_F1 = 0x01000030;
var KEY_F20 = 0x01000043;
var KEY_SPACE = 0x20;

// the final of the cursor keys: ESC [ A, or ESC O A in the application mode (DECCKM)
var CURSOR_FINALS = {};
CURSOR_FINALS[KEY_UP] = "A";
CURSOR_FINALS[KEY_DOWN] = "B";
CURSOR_FINALS[KEY_RIGHT] = "C";
CURSOR_FINALS[KEY_LEFT] = "D";
CURSOR_FINALS[KEY_HOME] = "H";
CURSOR_FINALS[KEY_END] = "F";

// the number of the keys that go as ESC [ n ~
var TILDE_KEYS = {};
TILDE_KEYS[KEY_INSERT] = 2;
TILDE_KEYS[KEY_DELETE] = 3;
TILDE_KEYS[KEY_PAGEUP] = 5;
TILDE_KEYS[KEY_PAGEDOWN] = 6;

// F5..F20 as ESC [ n ~ (F1..F4 are ESC O P..S)
var FUNCTION_NUMBERS = [15, 17, 18, 19, 20, 21, 23, 24, 25, 26, 28, 29, 31, 32, 33, 34];
var FUNCTION_FINALS = ["P", "Q", "R", "S"];

// The control character of a key with Ctrl (the letters, and the keys of xterm for @ [ \ ] ^ _ ? and the digits 2..8), or -1.
function controlCode(key){
	if (key >= 0x41 && key <= 0x5a){
		return key - 0x40;
	}
	if (key >= 0x61 && key <= 0x7a){
		return key - 0x60;
	}
	switch (key){
	case 0x40: case 0x32: return 0;
	case 0x5b: case 0x33: return 0x1b;
	case 0x5c: case 0x34: return 0x1c;
	case 0x5d: case 0x35: return 0x1d;
	case 0x5e: case 0x36: return 0x1e;
	case 0x5f: case 0x37: case 0x2f: case 0x2d: return 0x1f;
	case 0x3f: case 0x38: return 0x7f;
	case KEY_SPACE: return 0;
	}
	return -1;
}

// The bytes that a key sends. key: {key (the Qt key code), text, ctrl, alt, shift}; "" when the key sends nothing (a modifier alone, a
// key that the terminal has no sequence for). The encoding is that of xterm: the modifier of a cursor key, an F-key, Insert, Delete,
// PageUp and PageDown is the parameter 1 + shift + 2 * alt + 4 * ctrl (ESC [ 1 ; 5 A, ESC [ 15 ; 2 ~); the cursor keys go as ESC O A
// while the application asks for it (DECCKM, `appCursorKeys`); Alt in front of a character is ESC.
function encodeKey(t, spec){
	var key = spec.key;
	var modifier = 1 + (spec.shift ? 1 : 0) + (spec.alt ? 2 : 0) + (spec.ctrl ? 4 : 0);
	var modified = modifier > 1;
	var prefix = spec.alt ? "\x1b" : "";

	if (CURSOR_FINALS[key] !== undefined){
		var letter = CURSOR_FINALS[key];
		if (modified){
			return "\x1b[1;" + modifier + letter;
		}
		return (t.appCursorKeys ? "\x1bO" : "\x1b[") + letter;
	}
	if (TILDE_KEYS[key] !== undefined){
		return "\x1b[" + TILDE_KEYS[key] + (modified ? ";" + modifier : "") + "~";
	}
	if (key >= KEY_F1 && key <= KEY_F20){
		var number = key - KEY_F1;
		if (number < 4){
			return modified ? "\x1b[1;" + modifier + FUNCTION_FINALS[number] : "\x1bO" + FUNCTION_FINALS[number];
		}
		return "\x1b[" + FUNCTION_NUMBERS[number - 4] + (modified ? ";" + modifier : "") + "~";
	}
	switch (key){
	case KEY_BACKTAB:
		return "\x1b[Z";
	case KEY_TAB:
		return spec.shift ? "\x1b[Z" : prefix + "\t";
	case KEY_BACKSPACE:
		return spec.ctrl ? prefix + "\x08" : prefix + "\x7f";
	case KEY_RETURN:
	case KEY_ENTER:
		return prefix + "\r";
	case KEY_ESCAPE:
		return prefix + "\x1b";
	}

	if (spec.ctrl){
		var control = controlCode(key);
		if (control >= 0){
			return prefix + String.fromCharCode(control);
		}
	}
	if (spec.text !== undefined && spec.text.length > 0 && !spec.ctrl){
		return prefix + spec.text;
	}
	if (key === KEY_SPACE){
		return prefix + " ";
	}
	return "";
}

// ── the mouse, the paste and the focus ──────────────────────────────────────

// The bytes of a mouse event when the program asked for the mouse (`?1000`: the buttons, `?1002`: and the motion while a button is down,
// `?1003`: all the motion), in the form that it chose (`?1006`: ESC [ < b ; x ; y M or m, otherwise ESC [ M and three characters).
// ev: {action: "press" | "release" | "move" | "wheelUp" | "wheelDown", button: 0 left, 1 middle, 2 right, -1 none (a move), col, row (from 0),
// shift, alt, ctrl}. "" when the event is not for the program (no mouse mode, a move that the mode does not report, a place the old form cannot say).
function encodeMouse(t, ev){
	if (t.mouseMode === 0){
		return "";
	}
	var cb;
	var release = false;
	if (ev.action === "wheelUp"){
		cb = 64;
	}
	else if (ev.action === "wheelDown"){
		cb = 65;
	}
	else if (ev.action === "move"){
		if (t.mouseMode === 1000 || (t.mouseMode === 1002 && ev.button < 0)){
			return "";
		}
		cb = (ev.button < 0 ? 3 : ev.button) + 32;
	}
	else if (ev.action === "release"){
		release = true;
		cb = t.mouseSgr ? Math.max(0, ev.button) : 3;
	}
	else{
		cb = Math.max(0, ev.button);
	}
	cb += (ev.shift ? 4 : 0) + (ev.alt ? 8 : 0) + (ev.ctrl ? 16 : 0);
	var x = ev.col + 1;
	var y = ev.row + 1;
	if (t.mouseSgr){
		return "\x1b[<" + cb + ";" + x + ";" + y + (release ? "m" : "M");
	}
	if (x > 223 || y > 223){
		return "";
	}
	return "\x1b[M" + String.fromCharCode(32 + cb) + String.fromCharCode(32 + x) + String.fromCharCode(32 + y);
}

// What a paste sends: the new lines are the Enter key; when the program asked for the bracketed paste (`?2004`) the text goes between
// ESC [ 200 ~ and ESC [ 201 ~ (the escape characters of the text are dropped, so it cannot end the paste itself).
function encodePaste(t, text){
	var data = String(text).replace(/\r\n/g, "\r").replace(/\n/g, "\r");
	if (t.bracketedPaste){
		return "\x1b[200~" + data.replace(/\x1b/g, "") + "\x1b[201~";
	}
	return data;
}

// The report of the focus (`?1004`): ESC [ I when the view got it, ESC [ O when it lost it.
function encodeFocus(t, focused){
	if (!t.focusEvents){
		return "";
	}
	return focused ? "\x1b[I" : "\x1b[O";
}

// ── the selection of the grid ───────────────────────────────────────────────
// A selection is {r1, c1, r2, c2}: where the button went down and where the pointer is (any order); it runs along the lines as text does.

function orderedSelection(sel){
	if (sel.r1 > sel.r2 || (sel.r1 === sel.r2 && sel.c1 > sel.c2)){
		return {r1: sel.r2, c1: sel.c2, r2: sel.r1, c2: sel.c1};
	}
	return {r1: sel.r1, c1: sel.c1, r2: sel.r2, c2: sel.c2};
}

// The columns of a row that the selection covers, [from, to] with both included, or null.
function selectionColumns(sel, row, cols){
	var s = orderedSelection(sel);
	if (row < s.r1 || row > s.r2){
		return null;
	}
	return [row === s.r1 ? s.c1 : 0, row === s.r2 ? s.c2 : cols - 1];
}

// The text of the selection: the rows one under the other, the blanks at the end of each of them dropped.
function selectionText(t, sel){
	var lines = [];
	for (var r = 0; r < t.rows; ++r){
		var range = selectionColumns(sel, r, t.cols);
		if (range === null){
			continue;
		}
		var cells = t.grid[r];
		var text = "";
		for (var c = range[0]; c <= range[1] && c < cells.length; ++c){
			text += cells[c].ch;
		}
		lines.push(text.replace(/ +$/, ""));
	}
	return lines.join("\n");
}
