import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import Qt.labs.platform 1.1
import Qt.labs.settings 1.0

/**
	One terminal session tab content (Windows Terminal-style multi-tab host lives in TerminalView).

	Each pane owns its own TerminalController, subscription and output buffer.
	ANSI SGR colour codes are translated to HTML spans at render time.
*/
Item {
	id: root;

	// Wired by TerminalView when the tab is created.
	property string agentId: "";
	property string shellTypeId: "";
	property string shellName: "";
	// When true, close the host tab after the remote session is torn down.
	property bool removeTabWhenClosed: false;

	readonly property bool running: controller.running;
	readonly property bool busy: controller.busy;
	readonly property bool connectionLost: controller.connectionLost;
	readonly property string sessionId: controller.sessionId;

	property string errorText: "";
	property string actionHint: "";

	// Copy-all feedback: disable the button and show PopupView for a short time.
	property bool copyFeedbackActive: false;
	readonly property int copyFeedbackMs: 2000;

	signal requestRemoveTab();
	signal sessionStateChanged();

	// Plain text for copy/download (no HTML).
	property string plainLog: "";
	property string richLog: "";
	property bool startupOutputPhase: true;

	property var commandHistory: [];
	property int historyIndex: -1;
	property string historyDraft: "";
	readonly property int maxCommandHistory: 100;
	readonly property int maxOutputLines: 5000;
	readonly property int maxOutputLength: 200000;

	readonly property int idleCloseExitCode: -2;
	readonly property int manualCloseExitCode: -1;

	// ── Full-screen (alternate screen buffer) emulation state ───────────────
	// Curses-style apps (vim, mc, top) switch the pty into the "alternate screen"
	// (CSI ?1049h/?47h) and address a real 2D cursor; the line-oriented scrollback
	// above cannot render that, so those bytes are routed to a small character-grid
	// emulator instead and the two views are swapped based on altScreenActive.
	property bool altScreenActive: false;
	property var screenGrid: [];
	property int gridCols: 80;
	property int gridRows: 24;
	property int cursorRow: 0;
	property int cursorCol: 0;
	property bool cursorVisible: true;
	property int scrollTop: 0;
	property int scrollBottom: 23;
	property var altSgrState: ({color: null, bold: false});
	property string altPendingPartial: "";
	property var savedCursorForDECSC: null;
	// Bumped once per processed chunk so the grid delegates' text bindings
	// (which read this property) know to re-evaluate; screenGrid is mutated
	// in place and does not fire property-change notifications on its own.
	property int screenVersion: 0;
	// Held-back tail of a possibly-split alt-screen-transition sequence
	// (\x1b[?1049h/l, \x1b[?47h/l) so it is not misread across two chunks.
	property string routerPendingPartial: "";

	// ── Primary-screen live line (raw keyboard passthrough) ─────────────────
	// With raw passthrough every keystroke round-trips to the shell, which echoes it and
	// edits the current command line in place (backspace, \r rewrite for Tab-completion /
	// history recall). The scrollback model is append-only and cannot show in-place edits,
	// so the still-unterminated last line is held here and rendered live (below the
	// committed scrollback, with a cursor block) instead of being appended as fragments;
	// it is committed to the scrollback only when its terminating newline arrives.
	property string primaryBuffer: "";
	property string currentLineHtml: "";

	// ── ANSI / HTML helpers ─────────────────────────────────────────────────

	function htmlEscape(text){
		return String(text)
					.replace(/&/g, "&amp;")
					.replace(/</g, "&lt;")
					.replace(/>/g, "&gt;");
	}

	// Minimal SGR translator: \x1b[…m colour/reset only. Cursor/TUI sequences are dropped.
	function ansiToHtml(text, defaultColor){
		let src = String(text);
		// Drop non-SGR CSI sequences (cursor move, erase, etc.).
		src = src.replace(/\x1b\[[0-9;?]*[A-Za-ln-z]/g, "");
		src = src.replace(/\x1b\][^\x07]*(?:\x07|\x1b\\)/g, "");

		let result = "";
		let color = defaultColor;
		let bold = false;
		let re = /\x1b\[([0-9;]*)m/g;
		let last = 0;
		let match = re.exec(src);
		while (match){
			let chunk = src.substring(last, match.index);
			if (chunk.length > 0){
				let style = "color:" + color;
				if (bold){
					style += ";font-weight:bold";
				}
				result += "<span style=\"" + style + "\">" + root.htmlEscape(chunk).replace(/\n/g, "<br/>") + "</span>";
			}
			let codes = match[1].length === 0 ? ["0"] : match[1].split(";");
			for (let i = 0; i < codes.length; ++i){
				let code = parseInt(codes[i], 10);
				if (isNaN(code) || code === 0){
					color = defaultColor;
					bold = false;
				}
				else if (code === 1){
					bold = true;
				}
				else if (code === 22){
					bold = false;
				}
				else if (code === 30){ color = "#000000"; }
				else if (code === 31){ color = "#cc0000"; }
				else if (code === 32){ color = "#00aa00"; }
				else if (code === 33){ color = "#cccc00"; }
				else if (code === 34){ color = "#0000cc"; }
				else if (code === 35){ color = "#aa00aa"; }
				else if (code === 36){ color = "#00aaaa"; }
				else if (code === 37){ color = "#cccccc"; }
				else if (code === 39){ color = defaultColor; }
				else if (code === 90){ color = "#666666"; }
				else if (code === 91){ color = "#ff5555"; }
				else if (code === 92){ color = "#55ff55"; }
				else if (code === 93){ color = "#ffff55"; }
				else if (code === 94){ color = "#5555ff"; }
				else if (code === 95){ color = "#ff55ff"; }
				else if (code === 96){ color = "#55ffff"; }
				else if (code === 97){ color = "#ffffff"; }
			}
			last = match.index + match[0].length;
			match = re.exec(src);
		}
		let tail = src.substring(last);
		if (tail.length > 0){
			let style = "color:" + color;
			if (bold){
				style += ";font-weight:bold";
			}
			result += "<span style=\"" + style + "\">" + root.htmlEscape(tail).replace(/\n/g, "<br/>") + "</span>";
		}
		return result;
	}

	// Shared 16-colour SGR table, also used by the alt-screen grid emulator below
	// (kept intentionally separate from ansiToHtml's own inline table: that function
	// tracks the *effective* colour directly, this one tracks a null-able "no explicit
	// colour set" state, so merging them would add a branch to already-working code
	// for no real benefit).
	function sgrColorForCode(code){
		switch (code){
		case 30: return "#000000";
		case 31: return "#cc0000";
		case 32: return "#00aa00";
		case 33: return "#cccc00";
		case 34: return "#0000cc";
		case 35: return "#aa00aa";
		case 36: return "#00aaaa";
		case 37: return "#cccccc";
		case 90: return "#666666";
		case 91: return "#ff5555";
		case 92: return "#55ff55";
		case 93: return "#ffff55";
		case 94: return "#5555ff";
		case 95: return "#ff55ff";
		case 96: return "#55ffff";
		case 97: return "#ffffff";
		}
		return null;
	}

	// Mutates state {color, bold} from a list of already-split numeric SGR codes.
	// state.color === null means "no explicit colour set, use the default".
	function applySgrToState(codes, state){
		for (let i = 0; i < codes.length; ++i){
			let code = codes[i];
			if (code === 0){
				state.color = null;
				state.bold = false;
			}
			else if (code === 1){
				state.bold = true;
			}
			else if (code === 22){
				state.bold = false;
			}
			else if (code === 39){
				state.color = null;
			}
			else{
				let mapped = root.sgrColorForCode(code);
				if (mapped !== null){
					state.color = mapped;
				}
			}
		}
	}

	// ── Alt-screen character-grid emulator ───────────────────────────────────

	function currentGridColor(){
		return root.altSgrState.color !== null ? root.altSgrState.color : root.streamColor("STDOUT");
	}

	function makeBlankRow(cols, color){
		let row = [];
		for (let c = 0; c < cols; ++c){
			row.push({ch: " ", color: color, bold: false});
		}
		return row;
	}

	function resetGrid(cols, rows){
		root.gridCols = Math.max(1, cols);
		root.gridRows = Math.max(1, rows);
		root.scrollTop = 0;
		root.scrollBottom = root.gridRows - 1;
		root.cursorRow = 0;
		root.cursorCol = 0;
		root.cursorVisible = true;
		root.savedCursorForDECSC = null;
		let grid = [];
		for (let r = 0; r < root.gridRows; ++r){
			grid.push(root.makeBlankRow(root.gridCols, root.currentGridColor()));
		}
		root.screenGrid = grid;
		root.screenVersion++;
	}

	// Called whenever the panel's computed character size changes (see sendTerminalSize);
	// curses apps redraw fully on SIGWINCH, so the grid is simply reset to the new size
	// rather than trying to preserve/reflow stale content.
	function applyLocalGridSize(columns, rows){
		if (columns === root.gridCols && rows === root.gridRows){
			return;
		}
		root.resetGrid(columns, rows);
	}

	function enterAltScreen(){
		if (root.altScreenActive){
			return;
		}
		root.altScreenActive = true;
		root.altSgrState = {color: null, bold: false};
		root.altPendingPartial = "";
		root.resetGrid(root.gridCols, root.gridRows);
		gridView.forceActiveFocus();
	}

	function exitAltScreen(){
		if (!root.altScreenActive){
			return;
		}
		root.altScreenActive = false;
		root.screenVersion++;
		if (controller.running && !controller.connectionLost){
			inputField.forceActiveFocus();
		}
	}

	function vtInputForKey(event){
		if ((event.modifiers & Qt.ControlModifier)
				&& event.key >= Qt.Key_A && event.key <= Qt.Key_Z){
			return String.fromCharCode(event.key - Qt.Key_A + 1);
		}

		switch (event.key){
		case Qt.Key_Up: return "\x1b[A";
		case Qt.Key_Down: return "\x1b[B";
		case Qt.Key_Right: return "\x1b[C";
		case Qt.Key_Left: return "\x1b[D";
		case Qt.Key_Home: return "\x1b[H";
		case Qt.Key_End: return "\x1b[F";
		case Qt.Key_PageUp: return "\x1b[5~";
		case Qt.Key_PageDown: return "\x1b[6~";
		case Qt.Key_Insert: return "\x1b[2~";
		case Qt.Key_Delete: return "\x1b[3~";
		case Qt.Key_F1: return "\x1bOP";
		case Qt.Key_F2: return "\x1bOQ";
		case Qt.Key_F3: return "\x1bOR";
		case Qt.Key_F4: return "\x1bOS";
		case Qt.Key_F5: return "\x1b[15~";
		case Qt.Key_F6: return "\x1b[17~";
		case Qt.Key_F7: return "\x1b[18~";
		case Qt.Key_F8: return "\x1b[19~";
		case Qt.Key_F9: return "\x1b[20~";
		case Qt.Key_F10: return "\x1b[21~";
		case Qt.Key_Backspace: return "\x7f";
		case Qt.Key_Backtab: return "\x1b[Z";
		case Qt.Key_Tab: return "\t";
		case Qt.Key_Return:
		case Qt.Key_Enter: return "\r";
		case Qt.Key_Escape: return "\x1b";
		}

		if (event.text.length === 0){
			return "";
		}
		return (event.modifiers & Qt.AltModifier) ? "\x1b" + event.text : event.text;
	}

	function sendAltScreenKey(event){
		if (!root.altScreenActive || !controller.running || controller.connectionLost){
			return;
		}

		let data = root.vtInputForKey(event);
		if (data.length > 0){
			controller.sendRawInput(data);
			event.accepted = true;
		}
	}

	function scrollGridUp(){
		let blank = root.makeBlankRow(root.gridCols, root.currentGridColor());
		root.screenGrid.splice(root.scrollTop, 1);
		root.screenGrid.splice(root.scrollBottom, 0, blank);
	}

	function scrollGridDown(){
		let blank = root.makeBlankRow(root.gridCols, root.currentGridColor());
		root.screenGrid.splice(root.scrollBottom, 1);
		root.screenGrid.splice(root.scrollTop, 0, blank);
	}

	function moveCursorDownWithScroll(){
		if (root.cursorRow >= root.scrollBottom){
			root.scrollGridUp();
		}
		else if (root.cursorRow < root.gridRows - 1){
			root.cursorRow++;
		}
	}

	function gridPutChar(ch){
		if (root.cursorRow >= 0 && root.cursorRow < root.gridRows){
			let row = root.screenGrid[root.cursorRow];
			if (row && root.cursorCol >= 0 && root.cursorCol < root.gridCols){
				row[root.cursorCol] = {ch: ch, color: root.currentGridColor(), bold: root.altSgrState.bold};
			}
		}
		root.cursorCol++;
		if (root.cursorCol >= root.gridCols){
			root.cursorCol = 0;
			root.moveCursorDownWithScroll();
		}
	}

	function moveCursor(dRow, dCol){
		root.cursorRow = Math.max(0, Math.min(root.gridRows - 1, root.cursorRow + dRow));
		root.cursorCol = Math.max(0, Math.min(root.gridCols - 1, root.cursorCol + dCol));
	}

	function setCursorPosition(row1based, col1based){
		root.cursorRow = Math.max(0, Math.min(root.gridRows - 1, row1based - 1));
		root.cursorCol = Math.max(0, Math.min(root.gridCols - 1, col1based - 1));
	}

	function eraseInLine(mode){
		let row = root.screenGrid[root.cursorRow];
		if (!row){
			return;
		}
		let from = 0;
		let to = root.gridCols - 1;
		if (mode === 0){
			from = root.cursorCol;
		}
		else if (mode === 1){
			to = root.cursorCol;
		}
		for (let c = from; c <= to; ++c){
			row[c] = {ch: " ", color: root.currentGridColor(), bold: false};
		}
	}

	function eraseInDisplay(mode){
		if (mode === 2 || mode === 3){
			for (let r = 0; r < root.gridRows; ++r){
				root.screenGrid[r] = root.makeBlankRow(root.gridCols, root.currentGridColor());
			}
			return;
		}
		if (mode === 0){
			root.eraseInLine(0);
			for (let r = root.cursorRow + 1; r < root.gridRows; ++r){
				root.screenGrid[r] = root.makeBlankRow(root.gridCols, root.currentGridColor());
			}
		}
		else if (mode === 1){
			root.eraseInLine(1);
			for (let r = 0; r < root.cursorRow; ++r){
				root.screenGrid[r] = root.makeBlankRow(root.gridCols, root.currentGridColor());
			}
		}
	}

	function insertLines(n){
		for (let i = 0; i < n; ++i){
			let blank = root.makeBlankRow(root.gridCols, root.currentGridColor());
			root.screenGrid.splice(root.scrollBottom, 1);
			root.screenGrid.splice(root.cursorRow, 0, blank);
		}
	}

	function deleteLines(n){
		for (let i = 0; i < n; ++i){
			let blank = root.makeBlankRow(root.gridCols, root.currentGridColor());
			root.screenGrid.splice(root.cursorRow, 1);
			root.screenGrid.splice(root.scrollBottom, 0, blank);
		}
	}

	function deleteChars(n){
		let row = root.screenGrid[root.cursorRow];
		if (!row){
			return;
		}
		for (let i = 0; i < n; ++i){
			row.splice(root.cursorCol, 1);
			row.push({ch: " ", color: root.currentGridColor(), bold: false});
		}
	}

	function insertChars(n){
		let row = root.screenGrid[root.cursorRow];
		if (!row){
			return;
		}
		for (let i = 0; i < n; ++i){
			row.splice(root.cursorCol, 0, {ch: " ", color: root.currentGridColor(), bold: false});
			row.pop();
		}
	}

	// Applies one parsed CSI sequence (params before the final byte, and the final
	// byte itself) to the grid/cursor state. Sequences with no handling below
	// (mouse reporting, bracketed paste, etc.) are recognized-and-ignored, same
	// policy as ansiToHtml uses for the scrollback log.
	function handleCsiSequence(paramsStr, finalByte){
		let isPrivate = paramsStr.indexOf("?") === 0;
		let cleanParams = isPrivate ? paramsStr.substring(1) : paramsStr;
		let parts = cleanParams.length > 0 ? cleanParams.split(";") : [];
		let nums = parts.map(function(p){ let v = parseInt(p, 10); return isNaN(v) ? 0 : v; });

		let p = function(idx, def){
			return (nums.length > idx && nums[idx] > 0) ? nums[idx] : def;
		}

		if (isPrivate){
			if (finalByte === "h" || finalByte === "l"){
				let enable = finalByte === "h";
				for (let k = 0; k < nums.length; ++k){
					if (nums[k] === 1049 || nums[k] === 47){
						if (enable){ root.enterAltScreen(); } else { root.exitAltScreen(); }
					}
					else if (nums[k] === 25){
						root.cursorVisible = enable;
					}
				}
			}
			return;
		}

		switch (finalByte){
		case "H":
		case "f":
			root.setCursorPosition(p(0, 1), p(1, 1));
			break;
		case "A":
			root.moveCursor(-p(0, 1), 0);
			break;
		case "B":
			root.moveCursor(p(0, 1), 0);
			break;
		case "C":
			root.moveCursor(0, p(0, 1));
			break;
		case "D":
			root.moveCursor(0, -p(0, 1));
			break;
		case "G":
			root.cursorCol = Math.max(0, Math.min(root.gridCols - 1, p(0, 1) - 1));
			break;
		case "d":
			root.cursorRow = Math.max(0, Math.min(root.gridRows - 1, p(0, 1) - 1));
			break;
		case "J":
			root.eraseInDisplay(nums.length > 0 ? nums[0] : 0);
			break;
		case "K":
			root.eraseInLine(nums.length > 0 ? nums[0] : 0);
			break;
		case "S":
			for (let s1 = 0; s1 < p(0, 1); ++s1){ root.scrollGridUp(); }
			break;
		case "T":
			for (let s2 = 0; s2 < p(0, 1); ++s2){ root.scrollGridDown(); }
			break;
		case "L":
			root.insertLines(p(0, 1));
			break;
		case "M":
			root.deleteLines(p(0, 1));
			break;
		case "P":
			root.deleteChars(p(0, 1));
			break;
		case "@":
			root.insertChars(p(0, 1));
			break;
		case "r":
			root.scrollTop = Math.max(0, p(0, 1) - 1);
			root.scrollBottom = Math.min(root.gridRows - 1, p(1, root.gridRows) - 1);
			root.cursorRow = 0;
			root.cursorCol = 0;
			break;
		case "m":
			root.applySgrToState(nums, root.altSgrState);
			break;
		default:
			break;
		}
	}

	// Feeds one chunk of raw pty bytes (already known to belong to the alternate
	// screen) through a small VT state machine: cursor movement, erase, scroll
	// region, insert/delete line/char and SGR are applied to screenGrid; anything
	// else recognized (OSC, DECSC/DECRC, RIS, reverse line feed) is consumed
	// without visible effect. An incomplete trailing escape is carried over to the
	// next chunk via altPendingPartial, mirroring ansiToHtml's own carry-over.
	function feedAltScreenData(text){
		let src = root.altPendingPartial + text;
		root.altPendingPartial = "";

		let i = 0;
		let n = src.length;
		while (i < n){
			let ch = src[i];

			if (ch === "\x1b"){
				if (i + 1 >= n){
					root.altPendingPartial = src.substring(i);
					break;
				}
				let next = src[i + 1];
				if (next === "["){
					let j = i + 2;
					while (j < n && /[0-9;?]/.test(src[j])){
						j++;
					}
					if (j >= n){
						root.altPendingPartial = src.substring(i);
						break;
					}
					root.handleCsiSequence(src.substring(i + 2, j), src[j]);
					i = j + 1;
					continue;
				}
				else if (next === "]"){
					let bel = src.indexOf("\x07", i);
					let st = src.indexOf("\x1b\\", i);
					let endIdx = -1;
					if (bel >= 0 && (st < 0 || bel < st)){
						endIdx = bel + 1;
					}
					else if (st >= 0){
						endIdx = st + 2;
					}
					if (endIdx < 0){
						root.altPendingPartial = src.substring(i);
						break;
					}
					i = endIdx;
					continue;
				}
				else if (next === "7"){
					root.savedCursorForDECSC = {row: root.cursorRow, col: root.cursorCol};
					i += 2;
					continue;
				}
				else if (next === "8"){
					if (root.savedCursorForDECSC){
						root.cursorRow = root.savedCursorForDECSC.row;
						root.cursorCol = root.savedCursorForDECSC.col;
					}
					i += 2;
					continue;
				}
				else if (next === "c"){
					root.resetGrid(root.gridCols, root.gridRows);
					i += 2;
					continue;
				}
				else if (next === "M"){
					if (root.cursorRow <= root.scrollTop){
						root.scrollGridDown();
					}
					else{
						root.cursorRow--;
					}
					i += 2;
					continue;
				}
				else{
					i += 2;
					continue;
				}
			}
			else if (ch === "\r"){
				root.cursorCol = 0;
				i++;
			}
			else if (ch === "\n"){
				root.moveCursorDownWithScroll();
				i++;
			}
			else if (ch === "\b"){
				if (root.cursorCol > 0){
					root.cursorCol--;
				}
				i++;
			}
			else if (ch === "\t"){
				root.cursorCol = Math.min(root.gridCols - 1, (Math.floor(root.cursorCol / 8) + 1) * 8);
				i++;
			}
			else{
				let code = ch.charCodeAt(0);
				if (code >= 32){
					root.gridPutChar(ch);
				}
				i++;
			}
		}

		root.screenVersion++;
	}

	// Rich-text for one grid row, including a highlighted cursor cell. Reads
	// screenVersion purely so this binding re-evaluates after each processed chunk
	// (screenGrid itself is mutated in place, not reassigned).
	function gridRowHtml(rowIndex){
		let dependency = root.screenVersion;
		let row = root.screenGrid[rowIndex];
		if (!row){
			return "";
		}

		let inner = "";
		let curColor = null;
		let curBold = null;
		let spanOpen = false;
		for (let c = 0; c < row.length; ++c){
			let cell = row[c];
			let isCursorCell = root.cursorVisible && rowIndex === root.cursorRow && c === root.cursorCol;
			if (cell.color !== curColor || cell.bold !== curBold || isCursorCell){
				if (spanOpen){
					inner += "</span>";
				}
				let style = "color:" + cell.color;
				if (cell.bold){
					style += ";font-weight:bold";
				}
				if (isCursorCell){
					style += ";background-color:" + Style.textSelectedColor + ";color:#ffffff";
				}
				inner += "<span style=\"" + style + "\">";
				spanOpen = true;
				curColor = cell.color;
				curBold = cell.bold;
			}
			inner += root.htmlEscape(cell.ch);
		}
		if (spanOpen){
			inner += "</span>";
		}
		return "<span style=\"white-space:pre\">" + inner + "</span>";
	}

	// If the very end of text could be an in-progress prefix of one of the four
	// alt-screen transition sequences (\x1b[?1049h/l, \x1b[?47h/l), returns the
	// index where that possible prefix starts; otherwise -1. Used only for the
	// "about to enter alt screen while still in scrollback mode" direction - once
	// altScreenActive is true, feedAltScreenData's own altPendingPartial already
	// carries over an incomplete exit sequence correctly.
	function findAltTransitionPartialStart(text){
		let idx = text.lastIndexOf("\x1b");
		if (idx < 0){
			return -1;
		}
		let candidate = text.substring(idx);
		if (candidate.length >= 9){
			return -1;
		}
		let targets = ["\x1b[?1049h", "\x1b[?1049l", "\x1b[?47h", "\x1b[?47l"];
		for (let t = 0; t < targets.length; ++t){
			if (targets[t].indexOf(candidate) === 0){
				return idx;
			}
		}
		return -1;
	}

	// Splits raw pty bytes at DEC alternate-screen-buffer transitions
	// (\x1b[?1049h/l, \x1b[?47h/l) and routes each segment to the scrollback log
	// (primary screen) or the character-grid emulator (alternate screen), toggling
	// altScreenActive as each transition is seen. A transition sequence split
	// across two chunks is held back in routerPendingPartial (see appendOutput)
	// and completed once the rest arrives.
	function routeTerminalBytes(text, streamId){
		let re = /\x1b\[\??(1049|47)[hl]/g;
		let last = 0;
		let match = re.exec(text);
		while (match){
			let segment = text.substring(last, match.index);
			if (segment.length > 0){
				if (root.altScreenActive){
					root.feedAltScreenData(segment);
				}
				else{
					root.feedPrimary(segment, streamId);
				}
			}

			if (match[0].charAt(match[0].length - 1) === "h"){
				root.enterAltScreen();
			}
			else{
				root.exitAltScreen();
			}

			last = match.index + match[0].length;
			match = re.exec(text);
		}

		let tail = text.substring(last);
		if (tail.length === 0){
			return;
		}

		if (root.altScreenActive){
			root.feedAltScreenData(tail);
			return;
		}

		let partialIdx = root.findAltTransitionPartialStart(tail);
		if (partialIdx >= 0){
			let visible = tail.substring(0, partialIdx);
			if (visible.length > 0){
				root.feedPrimary(visible, streamId);
			}
			root.routerPendingPartial = tail.substring(partialIdx);
		}
		else{
			root.feedPrimary(tail, streamId);
		}
	}

	function streamColor(stream){
		if (stream === "STDERR"){
			return Style.errorColor;
		}
		if (stream === "SYSTEM"){
			return Style.subtitleColor;
		}
		return Style.textColor;
	}

	function showActionHint(message){
		root.actionHint = message;
		actionHintTimer.restart();
	}

	function clearOutput(){
		outputModel.clear();
		root.plainLog = "";
		root.richLog = "";
		root.primaryBuffer = "";
		root.currentLineHtml = "";
		root.startupOutputPhase = true;
		root.altScreenActive = false;
		root.routerPendingPartial = "";
		root.altPendingPartial = "";
		root.altSgrState = {color: null, bold: false};
		root.resetGrid(root.gridCols, root.gridRows);
	}

	// Entry point for every real process chunk: SYSTEM messages (our own synthetic
	// text, never containing real escape codes) always go to the scrollback log;
	// STDOUT/STDERR bytes are routed to the scrollback log or the alt-screen grid
	// depending on whether the shell is currently in the alternate screen buffer.
	function appendOutput(text, stream){
		if (text.length === 0){
			return;
		}

		let streamId = stream !== undefined ? stream : "STDOUT";

		if (streamId === "SYSTEM"){
			root.appendScrollbackText(text, streamId);
			return;
		}

		root.plainLog += text;
		if (root.plainLog.length > root.maxOutputLength){
			root.plainLog = root.plainLog.substring(root.plainLog.length - root.maxOutputLength);
		}

		root.routeTerminalBytes(root.routerPendingPartial + text, streamId);
		root.routerPendingPartial = "";
	}

	// Primary-screen VT stream: accumulate until a newline terminates one or more lines,
	// commit those (resolved) to the scrollback, and keep the trailing unterminated line as
	// the live line. Command output (lines ending in \n) still renders exactly as before;
	// only the interactive prompt+typing line is now shown live.
	function feedPrimary(text, streamId){
		root.primaryBuffer += text;

		let nl = root.primaryBuffer.lastIndexOf("\n");
		if (nl >= 0){
			let completed = root.primaryBuffer.substring(0, nl);
			root.primaryBuffer = root.primaryBuffer.substring(nl + 1);

			let lines = completed.split("\n");
			let out = "";
			for (let k = 0; k < lines.length; ++k){
				out += root.resolvePrimaryLine(lines[k]) + "\n";
			}
			root.appendScrollbackText(out, streamId);
		}

		root.currentLineHtml = root.renderLiveLine(root.primaryBuffer, streamId);
		scrollToEndTimer.restart();
	}

	// Collapses one raw line's in-place edits to the text finally visible on it: take only
	// what follows the last carriage return (cmd rewrites a line as "\r<new content>"), drop
	// non-SGR escape sequences (cursor moves, erase-line, OSC titles) while keeping SGR
	// colour codes, then fold backspaces. Good enough for typing echo, backspace and
	// Tab/history line-rewrites without a full cell-grid emulator.
	function resolvePrimaryLine(raw){
		let s = String(raw);
		let cr = s.lastIndexOf("\r");
		if (cr >= 0){
			s = s.substring(cr + 1);
		}
		s = s.replace(/\x1b\][^\x07]*(?:\x07|\x1b\\)/g, "");
		s = s.replace(/\x1b\[[0-9;?]*[A-Za-ln-z]/g, "");

		let result = "";
		for (let i = 0; i < s.length; ++i){
			if (s[i] === "\b"){
				result = result.slice(0, -1);
			}
			else{
				result += s[i];
			}
		}
		return result;
	}

	function renderLiveLine(raw, streamId){
		let resolved = root.resolvePrimaryLine(raw);
		let html = resolved.length > 0 ? root.ansiToHtml(resolved, root.streamColor(streamId)) : "";
		if (!controller.running || controller.connectionLost){
			return html;
		}
		// Trailing block cursor so the caret is visible where the shell expects input.
		return html + "<span style=\"background-color:" + Style.textSelectedColor + ";color:#ffffff\"> </span>";
	}

	function appendScrollbackText(text, streamId){
		// Keep a bounded line model so line count and rich-text selection stay in sync.
		let parts = String(text).split("\n");
		let endsWithNl = String(text).endsWith("\n");
		let lineCount = endsWithNl ? parts.length - 1 : parts.length;
		let color = root.streamColor(streamId);

		for (let i = 0; i < lineCount; ++i){
			let line = parts[i];
			let visibleLine = line
						.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, "")
						.replace(/\x1b\][^\x07]*(?:\x07|\x1b\\)/g, "")
						.replace(/[\x00-\x20\x7f]/g, "");
			if (root.startupOutputPhase && streamId !== "SYSTEM" && visibleLine.length === 0){
				continue;
			}
			// Re-add newline for all but last incomplete line unless source ended with \n.
			let display = line + ((i < lineCount - 1 || endsWithNl) ? "\n" : "");
			if (display.length === 0){
				continue;
			}
			let html = root.ansiToHtml(display, color);
			outputModel.append({"html": html});
			root.richLog += html;
		}

		let trimmed = false;
		while (outputModel.count > root.maxOutputLines){
			outputModel.remove(0);
			trimmed = true;
		}
		if (trimmed){
			root.richLog = "";
			for (let index = 0; index < outputModel.count; ++index){
				root.richLog += outputModel.get(index).html;
			}
		}

		scrollToEndTimer.restart();
	}

	function startSession(){
		if (root.agentId.length === 0 || root.shellTypeId.length === 0){
			return;
		}
		if (controller.running || controller.busy || controller.closeInFlight){
			return;
		}

		root.errorText = "";
		root.actionHint = "";
		root.clearOutput();
		controller.agentId = root.agentId;
		controller.openSession(root.shellTypeId);
	}

	// Kick CloseTerminalSession without waiting for the response.
	function closeSessionImmediate(){
		controller.closeSession();
	}

	function requestCloseSession(){
		// Immediate UI close — no confirm, no wait for the server.
		// Enqueue remote close first (while the pane is still alive), then drop the tab.
		root.closeSessionImmediate();
		root.requestRemoveTab();
	}

	function pushCommandHistory(command){
		if (command.length === 0){
			return;
		}

		if (root.commandHistory.length > 0
					&& root.commandHistory[root.commandHistory.length - 1] === command){
			root.historyIndex = -1;
			root.historyDraft = "";
			return;
		}

		let next = root.commandHistory.slice();
		next.push(command);
		while (next.length > root.maxCommandHistory){
			next.shift();
		}
		root.commandHistory = next;
		root.historyIndex = -1;
		root.historyDraft = "";
		root.persistCommandHistory();
	}

	function historyUp(){
		if (root.commandHistory.length === 0){
			return;
		}

		if (root.historyIndex < 0){
			root.historyDraft = inputField.text;
			root.historyIndex = root.commandHistory.length - 1;
		}
		else if (root.historyIndex > 0){
			root.historyIndex = root.historyIndex - 1;
		}

		inputField.text = root.commandHistory[root.historyIndex];
	}

	function historyDown(){
		if (root.historyIndex < 0){
			return;
		}

		if (root.historyIndex < root.commandHistory.length - 1){
			root.historyIndex = root.historyIndex + 1;
			inputField.text = root.commandHistory[root.historyIndex];
		}
		else{
			root.historyIndex = -1;
			inputField.text = root.historyDraft;
		}
	}

	function submitCommand(){
		// commandInFlight: refuse a new line until the previous SendTerminalInput is acked,
		// so commands are delivered to the shell strictly one at a time (see the input
		// field's readOnly / the Send button's enabled bindings, which mirror this).
		if (!controller.running || controller.connectionLost || controller.commandInFlight){
			return;
		}

		// Single-line input: strip CR/LF from paste, ignore empty.
		let command = String(inputField.text).replace(/\r/g, "").replace(/\n/g, "").trim();
		if (command.length === 0){
			return;
		}

		root.pushCommandHistory(command);
		root.startupOutputPhase = false;
		// Backend appends the platform-specific Enter sequence so the shell executes the line.
		controller.sendInput(command);
		inputField.text = "";
		inputField.forceActiveFocus();
	}

	function copyAllOutput(){
		if (root.plainLog.length === 0 || root.copyFeedbackActive){
			return;
		}

		clipboardProxy.text = root.plainLog;
		clipboardProxy.selectAll();
		clipboardProxy.copy();
		clipboardProxy.select(0, 0);

		// Anchor the toast under the Copy button (coordinates of this page).
		let point = copyAllButton.mapToItem(root, 0, copyAllButton.height + Style.spacingS);
		copiedPopup.x = point.x;
		copiedPopup.y = point.y;

		root.copyFeedbackActive = true;
		root.showActionHint(qsTr("Output copied to clipboard"));
		copyFeedbackTimer.restart();
	}

	function downloadLog(){
		if (root.plainLog.length === 0){
			root.showActionHint(qsTr("Nothing to save"));
			return;
		}

		let stamp = Qt.formatDateTime(new Date(), "yyyyMMdd-HHmmss");
		let agentPart = root.agentId.length > 0 ? root.agentId.substring(0, 8) : "session";
		saveLogDialog.currentFile = "terminal-" + agentPart + "-" + stamp + ".txt";
		saveLogDialog.open();
	}

	function saveLogToPath(fileUrl){
		let filePath = String(fileUrl);
		if (Qt.platform.os === "web"){
			logFileIO.source = saveLogDialog.currentFile;
		}
		else{
			filePath = filePath.replace("file:///", "");
			if (Qt.platform.os === "linux" || Qt.platform.os === "osx" || Qt.platform.os === "unix"){
				if (!filePath.startsWith("/")){
					filePath = "/" + filePath;
				}
			}
			logFileIO.source = filePath;
		}

		// Prefix a UTF-8 BOM (U+FEFF). The log holds decoded Unicode (Cyrillic, box-drawing,
		// etc.); FileIO.write emits the string's UTF-8 bytes as-is, and without the BOM a
		// Windows editor opens it in the local ANSI code page and shows mojibake. The BOM
		// makes the UTF-8 encoding explicit so the saved file reads exactly as on screen.
		logFileIO.write("\uFEFF" + root.plainLog);
		root.showActionHint(qsTr("Log saved"));
	}

	function loadCommandHistory(){
		try {
			let raw = historySettings.commandHistoryJson;
			if (!raw || raw.length === 0){
				return;
			}
			let parsed = JSON.parse(raw);
			if (Array.isArray(parsed)){
				root.commandHistory = parsed.slice(-root.maxCommandHistory);
			}
		}
		catch (e){
			root.commandHistory = [];
		}
	}

	function persistCommandHistory(){
		try {
			historySettings.commandHistoryJson = JSON.stringify(root.commandHistory);
		}
		catch (e){
		}
	}

	function sessionLabel(){
		if (controller.connectionLost){
			return qsTr("Connection lost");
		}
		if (controller.running){
			return qsTr("%1 session running").arg(root.shellName);
		}
		if (controller.busy){
			return qsTr("Working…");
		}
		return qsTr("%1 — session ended").arg(root.shellName);
	}

	function statusLabel(){
		if (root.errorText !== ""){
			return root.errorText;
		}
		if (root.actionHint !== ""){
			return root.actionHint;
		}
		if (controller.connectionLost){
			return qsTr("Output channel lost — press Reconnect or re-open the session");
		}
		if (controller.busy){
			return qsTr("Working…");
		}
		if (controller.requestInFlight){
			return qsTr("Sending request…");
		}
		if (controller.running){
			return qsTr("Session active · idle warning ~1 min before 15 min close · ↑/↓ history");
		}
		return qsTr("Session ended — close the tab or open a new one");
	}

	function statusColor(){
		if (root.errorText !== ""){
			return Style.errorColor;
		}
		if (controller.connectionLost){
			return Style.errorColor;
		}
		if (root.actionHint !== ""){
			return Style.imaginToolsAccentColor;
		}
		if (controller.busy){
			return Style.highlightColor;
		}
		if (controller.requestInFlight){
			return Style.highlightColor;
		}
		if (controller.running){
			return Style.imaginToolsAccentColor;
		}
		return Style.subtitleColor;
	}

	function inputPlaceHolder(){
		if (controller.connectionLost){
			return qsTr("Reconnect to send commands");
		}
		if (controller.running){
			return qsTr("Type a command and press Enter (↑/↓ history · Ctrl+C interrupt)");
		}
		if (controller.busy){
			return qsTr("Please wait…");
		}
		return qsTr("Session ended");
	}

	function lineCountLabel(){
		let n = outputModel.count;
		return n === 1 ? qsTr("1 line") : qsTr("%1 lines").arg(n);
	}

	// Label for the top-right activity popup, describing the request currently in flight.
	function requestPopupLabel(){
		if (controller.busy && !controller.running){
			return qsTr("Opening session…");
		}
		if (controller.closeInFlight){
			return qsTr("Closing session…");
		}
		if (controller.commandInFlight){
			return qsTr("Sending command…");
		}
		return qsTr("Working…");
	}

	// Tells the agent's pty the GUI's visible character grid so full-screen/curses
	// programs (vim, mc, top) draw at the right size instead of assuming 80x24.
	function sendTerminalSize(){
		if (!controller.running || root.charWidth <= 0 || root.charHeight <= 0){
			return;
		}
		let columns = Math.max(1, Math.floor(outputFlickable.width / root.charWidth));
		let rows = Math.max(1, Math.floor(outputFlickable.height / root.charHeight));
		root.applyLocalGridSize(columns, rows);
		controller.resizeSession(columns, rows);
	}

	onAgentIdChanged: {
		controller.agentId = root.agentId;
	}

	Component.onCompleted: {
		controller.agentId = root.agentId;
		root.loadCommandHistory();
		if (root.shellTypeId.length > 0){
			root.startSession();
		}
	}

	Component.onDestruction: {
		// Idempotent: forgetSession / closeInFlight guards double Close.
		controller.closeSession();
	}

	// Metrics of the output font, used to convert the panel's pixel size into a
	// character grid for ResizeTerminalSession. The QML FontMetrics type has no
	// JQML web-build implementation, so width/height are measured instead via a
	// hidden monospace Text probe and its contentWidth/contentHeight - both are
	// backed by JQApplication.TextController.measureTextFast in the web build,
	// so this works identically there and in the native QML build.
	readonly property int sizeMetricsProbeLength: 20;
	readonly property real charWidth: sizeMetricsProbe.contentWidth / root.sizeMetricsProbeLength;
	readonly property real charHeight: sizeMetricsProbe.contentHeight;

	Text {
		id: sizeMetricsProbe;
		visible: false;
		text: "MMMMMMMMMMMMMMMMMMMM";
		wrapMode: Text.NoWrap;
		font.family: Style.fontFamily
		font.pixelSize: Style.fontSizeM;
	}

	Timer {
		id: actionHintTimer;
		interval: 2500;
		repeat: false;
		onTriggered: root.actionHint = "";
	}

	// Debounced so a live window/panel drag does not spam ResizeTerminalSession.
	Timer {
		id: resizeDebounceTimer;
		interval: 150;
		repeat: false;
		onTriggered: root.sendTerminalSize();
	}

	// 0-interval Timer instead of Qt.callLater: scrolling to the end only needs to
	// happen after the current model updates are laid out, and this idiom is known
	// to behave the same under the JQML web build (see project QML conventions).
	Timer {
		id: scrollToEndTimer;
		interval: 0;
		repeat: false;
		onTriggered: {
			if (outputFlickable.contentHeight > outputFlickable.height){
				outputFlickable.contentY = outputFlickable.contentHeight - outputFlickable.height;
			}
		}
	}

	Timer {
		id: copyFeedbackTimer;
		interval: root.copyFeedbackMs;
		repeat: false;
		onTriggered: root.copyFeedbackActive = false;
	}

	// Toast under the Copy toolbar button (not via ModalDialogManager — that layer is
	// full-window and would place the toast far from the control).
	PopupView {
		id: copiedPopup;

		z: 100;
		visible: root.copyFeedbackActive;
		width: copiedPopupLabel.implicitWidth + 2 * Style.marginL;
		height: Style.controlHeightM;
		forceFocus: false;
		noMouseArea: true;
		escapeEnabled: false;
		hiddenBackground: true;

		Rectangle {
			anchors.fill: parent;
			color: Style.baseColor;
			border.color: Style.borderColor;
			border.width: 1;
			radius: Style.radiusM;

			Text {
				id: copiedPopupLabel;
				anchors.centerIn: parent;
				text: qsTr("Copied to clipboard");
				color: Style.textColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeM;
			}
		}
	}

	Settings {
		id: historySettings;
		category: "Agentino.Terminal";
		property string commandHistoryJson: "[]";
	}

	TextEdit {
		id: clipboardProxy;
		width: 0;
		height: 0;
		visible: false;
		readOnly: true;
	}

	FileDialog {
		id: saveLogDialog;
		title: qsTr("Save terminal log");
		fileMode: FileDialog.SaveFile;
		nameFilters: [qsTr("Text files (*.txt)"), qsTr("All files (*)")];
		currentFile: "terminal-log.txt";
		onAccepted: root.saveLogToPath(saveLogDialog.file);
	}

	FileIO {
		id: logFileIO;
	}

	ListModel {
		id: outputModel;
	}

	SubscriptionClient {
		id: outputSubscription;

		gqlCommandId: "OnTerminalOutputChanged";
		autoSubscribe: false;

		function getHeaders(){
			return controller.getHeaders();
		}

		function getGqlQuery(){
			var query = Gql.GqlRequest("subscription", gqlCommandId);
			var inputParams = Gql.GqlObject("input");
			inputParams.InsertField("sessionId", controller.sessionId);
			// Re-read at each registerSubscription() call (initial open, reconnect, or the
			// resubscribe-after-drop path) - the publisher pushes every buffered chunk from
			// here onward as soon as it registers, so this is what makes the subscribe call
			// itself double as catch-up, with no separate query needed to close the gap.
			inputParams.InsertField("fromSequence", controller.nextSequence);
			query.AddParam(inputParams);

			query.AddField(Gql.GqlObject("sessionId"));

			var chunks = Gql.GqlObject("chunks");
			chunks.InsertField("sequence");
			chunks.InsertField("stream");
			chunks.InsertField("data");
			query.AddField(chunks);

			query.AddField(Gql.GqlObject("nextSequence"));
			query.AddField(Gql.GqlObject("running"));
			query.AddField(Gql.GqlObject("exitCode"));
			query.AddField(Gql.GqlObject("upstreamHealthy"));

			return query;
		}

		onMessageReceived: {
			controller.applyOutputPayload(data);
		}

		onStateChanged: {
			controller.handleSubscriptionHealth(state);
		}
	}

	TerminalController {
		id: controller;

		outputSubscription: outputSubscription;

		onOutputReceived: {
			root.appendOutput(data, stream);
		}

		onSessionOpened: {
			root.errorText = "";
			root.appendOutput(qsTr("[session opened]\n"), "SYSTEM");
			root.sessionStateChanged();
			// Focus the output, not the compose box: typing goes straight to the shell
			// (full cmd-like keyboard). The box stays available for pasting/composing.
			outputText.forceActiveFocus();
			resizeDebounceTimer.restart();
		}

		onSessionClosed: {
			if (exitCode === root.idleCloseExitCode){
				root.appendOutput(qsTr("\n[session closed: idle timeout]\n"), "SYSTEM");
			}
			else if (exitCode === root.manualCloseExitCode){
				root.appendOutput(qsTr("\n[session closed]\n"), "SYSTEM");
			}
			else{
				root.appendOutput(qsTr("\n[session closed, exit code %1]\n").arg(exitCode), "SYSTEM");
			}
			root.sessionStateChanged();
			// Tab strip closes immediately; only idle/server-driven end may still ask to drop the tab.
			if (root.removeTabWhenClosed){
				root.requestRemoveTab();
			}
		}

		onErrorOccurred: {
			root.errorText = message;
			root.appendOutput(qsTr("\n[error] %1\n").arg(message), "STDERR");
		}

		onConnectionLostChangedSignal: {
			root.sessionStateChanged();
		}
	}

	// ─── Toolbar ────────────────────────────────────────────────────────────
	Item {
		id: toolbar;

		anchors.top: parent.top;
		anchors.left: parent.left;
		anchors.right: parent.right;
		anchors.leftMargin: Style.marginL;
		anchors.rightMargin: Style.marginL;
		anchors.topMargin: Style.marginM;

		height: Style.controlHeightM;

		Row {
			id: navButtons;

			anchors.left: parent.left;
			anchors.verticalCenter: parent.verticalCenter;

			spacing: Style.spacingXS;

			Button {
				id: closeButton;
				widthFromDecorator: true;
				enabled: !controller.closeInFlight;
				tooltipText: qsTr("Close this tab (and terminate the session if running)");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Close", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: root.requestCloseSession();
			}

			Button {
				id: interruptButton;
				widthFromDecorator: true;
				enabled: controller.running && !controller.connectionLost && !controller.busy;
				tooltipText: qsTr("Interrupt current command (Ctrl+C)");
				// No dedicated icon: reuse Clear styling intent with Stop-like affordance via text.
				text: "Ctrl+C";
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: controller.interruptSession();
			}

			Button {
				id: reconnectButton;
				widthFromDecorator: true;
				visible: controller.connectionLost;
				enabled: controller.connectionLost && controller.sessionId.length > 0;
				tooltipText: qsTr("Re-establish the output subscription");
				text: qsTr("Reconnect");
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: controller.reconnectSubscription();
			}

			Button {
				id: clearButton;
				widthFromDecorator: true;
				enabled: root.plainLog.length > 0;
				tooltipText: qsTr("Clear the output");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Clear", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: root.clearOutput();
			}

			Button {
				id: copyAllButton;
				widthFromDecorator: true;
				enabled: root.plainLog.length > 0 && !root.copyFeedbackActive;
				tooltipText: root.copyFeedbackActive
							? qsTr("Copied")
							: qsTr("Copy all output to clipboard");
				iconSource: "qrc:/" + Style.getIconPath(
					root.copyFeedbackActive ? "Icons/Ok" : "Icons/Copy",
					Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: root.copyAllOutput();
			}

			Button {
				id: downloadLogButton;
				widthFromDecorator: true;
				enabled: root.plainLog.length > 0;
				tooltipText: qsTr("Download log as a text file");
				iconSource: "qrc:/" + Style.getIconPath("Icons/DocumentExport", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: root.downloadLog();
			}
		}

		Rectangle {
			id: sessionField;
			anchors.left: navButtons.right;
			anchors.leftMargin: Style.marginM;
			anchors.right: parent.right;
			anchors.verticalCenter: parent.verticalCenter;
			height: parent.height;
			radius: height / 2;
			color: Style.backgroundColor2;

			Rectangle {
				anchors.fill: parent;
				radius: parent.radius;
				color: controller.connectionLost
							? Style.errorColor
							: (controller.running ? Style.imaginToolsAccentColor : "transparent");
				opacity: (controller.running || controller.connectionLost) ? 0.12 : 0;
			}

			Text {
				anchors.fill: parent;
				anchors.leftMargin: Style.marginL;
				anchors.rightMargin: Style.marginL;
				verticalAlignment: Text.AlignVCenter;
				elide: Text.ElideMiddle;
				text: root.sessionLabel();
				color: Style.textColor;
				opacity: 0.9;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeM;
			}
		}
	}

	// ─── Output panel ───────────────────────────────────────────────────────
	Rectangle {
		id: outputPanel;

		anchors.top: toolbar.bottom;
		anchors.topMargin: Style.marginL;
		anchors.left: parent.left;
		anchors.leftMargin: Style.marginL;
		anchors.right: parent.right;
		anchors.rightMargin: Style.marginL;
		anchors.bottom: inputRow.top;
		anchors.bottomMargin: Style.marginM;

		radius: Style.radiusM;
		color: Style.baseColor;
		border.color: Style.borderColor;
		border.width: 1;
		clip: true;

		Flickable {
			id: outputFlickable;

			anchors.fill: parent;
			anchors.margins: Style.marginS;
			anchors.rightMargin: Style.marginS + Style.marginM;

			visible: !root.altScreenActive;
			clip: true;
			boundsBehavior: Flickable.StopAtBounds;
			contentWidth: width;
			contentHeight: outputText.contentHeight;

			onWidthChanged: resizeDebounceTimer.restart();
			onHeightChanged: resizeDebounceTimer.restart();

			TextEdit {
				id: outputText;
				width: outputFlickable.width;
				height: contentHeight;
				readOnly: true;
				selectByMouse: true;
				persistentSelection: true;
				textFormat: TextEdit.RichText;
				wrapMode: TextEdit.WrapAnywhere;
				font.family: Style.fontFamily
				font.pixelSize: Style.fontSizeM;
				color: Style.textColor;
				selectionColor: Style.textSelectedColor;
				selectedTextColor: "#ffffff";
				// The committed scrollback plus the live (still-being-typed) line, so raw
				// keyboard echo and in-place line editing show up as one continuous line.
				text: root.richLog + root.currentLineHtml;

				Keys.onPressed: {
					// Ctrl+C copies the selection when there is one (otherwise it falls
					// through to the shell below as a real interrupt).
					if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C
								&& outputText.selectedText.length > 0){
						clipboardProxy.text = outputText.selectedText;
						clipboardProxy.selectAll();
						clipboardProxy.copy();
						clipboardProxy.select(0, 0);
						event.accepted = true;
						return;
					}

					// Full cmd-like keyboard: every other key is sent straight to the shell,
					// which echoes it and runs its own line editing / history / Tab-completion.
					// The compose box below remains for pasting or composing a long line.
					if (controller.running && !controller.connectionLost){
						let data = root.vtInputForKey(event);
						if (data.length > 0){
							root.startupOutputPhase = false;
							controller.sendRawInput(data);
							event.accepted = true;
						}
					}
				}
			}
		}

		CustomScrollbar {
			visible: !root.altScreenActive;
			z: outputFlickable.z + 1;
			anchors.right: parent.right;
			anchors.rightMargin: Style.marginXS;
			anchors.top: outputFlickable.top;
			anchors.bottom: outputFlickable.bottom;
			secondSize: Style.marginS;
			radius: Style.radiusS;
			targetItem: outputFlickable;
		}

		// ─── Full-screen app view (alt screen buffer: vim, mc, top, …) ──────────
		Item {
			id: gridView;

			anchors.fill: parent;
			anchors.margins: Style.marginS;

			visible: root.altScreenActive;
			focus: root.altScreenActive;
			clip: true;

			Keys.onPressed: {
				root.sendAltScreenKey(event);
			}

			Repeater {
				model: root.gridRows;

				delegate: Text {
					x: 0;
					y: index * root.charHeight;
					width: gridView.width;
					height: root.charHeight;
					textFormat: Text.RichText;
					font.family: Style.fontFamily
					font.pixelSize: Style.fontSizeM;
					text: root.gridRowHtml(index);
				}
			}
		}

		Column {
			id: emptyState;
			anchors.centerIn: parent;
			width: Math.min(parent.width - 2 * Style.marginXL, Style.sizeHintM);
			spacing: Style.spacingM;
			visible: outputModel.count === 0 && !controller.busy && !controller.running;

			Text {
				width: parent.width;
				horizontalAlignment: Text.AlignHCenter;
				text: qsTr("Starting %1…").arg(root.shellName);
				color: Style.textColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeL;
				opacity: 0.8;
				elide: Text.ElideRight;
			}
		}

		Rectangle {
			id: busyOverlay;
			anchors.fill: parent;
			radius: Style.radiusM;
			color: Style.baseColor;
			opacity: controller.busy ? 0.85 : 0;
			visible: opacity > 0.01;
			z: 10;

			Loading {
				anchors.centerIn: parent;
				width: Style.iconSizeL;
				height: Style.iconSizeL;
				indicatorSize: Style.iconSizeM;
				visible: controller.busy;
				background.color: "transparent";
			}
		}

		// Top-right activity pill: a spinner + label shown while any terminal request is
		// round-tripping to the agent. Sits above the output (z:30) but is naturally hidden
		// behind the full busy overlay during open/close since that is opaque over the panel.
		Rectangle {
			id: requestPopup;

			anchors.top: parent.top;
			anchors.right: parent.right;
			anchors.topMargin: Style.marginM;
			anchors.rightMargin: Style.marginM + Style.marginL;
			z: 30;

			visible: controller.requestInFlight;
			height: Style.controlHeightM;
			width: requestPopupRow.implicitWidth + 2 * Style.marginL;
			radius: height / 2;
			color: Style.baseColor;
			border.color: Style.imaginToolsAccentColor;
			border.width: 1;
			opacity: 0.97;

			Row {
				id: requestPopupRow;
				anchors.centerIn: parent;
				spacing: Style.spacingS;

				Loading {
					anchors.verticalCenter: parent.verticalCenter;
					width: Style.iconSizeM;
					height: Style.iconSizeM;
					indicatorSize: Style.iconSizeS;
					visible: true;
					background.color: "transparent";
				}

				Text {
					anchors.verticalCenter: parent.verticalCenter;
					text: root.requestPopupLabel();
					color: Style.textColor;
					font.family: Style.fontFamily;
					font.pixelSize: Style.fontSizeS;
				}
			}
		}

		// Connection-lost banner over the log.
		Rectangle {
			anchors.left: parent.left;
			anchors.right: parent.right;
			anchors.top: parent.top;
			height: Style.controlHeightM;
			color: Style.errorColor;
			opacity: 0.9;
			visible: controller.connectionLost;
			z: 5;

			Text {
				anchors.centerIn: parent;
				text: qsTr("Output subscription lost — commands may still run on the agent");
				color: "#ffffff";
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeS;
			}
		}
	}

	// ─── Command input (single-line TextInput) ──────────────────────────────
	Item {
		id: inputRow;

		anchors.left: parent.left;
		anchors.leftMargin: Style.marginL;
		anchors.right: parent.right;
		anchors.rightMargin: Style.marginL;
		anchors.bottom: statusBar.top;
		anchors.bottomMargin: Style.marginM;

		height: root.altScreenActive ? 0 : Style.controlHeightM;
		visible: !root.altScreenActive;

		Rectangle {
			id: inputBackground;

			anchors.left: parent.left;
			anchors.right: sendButton.left;
			anchors.rightMargin: Style.marginM;
			anchors.verticalCenter: parent.verticalCenter;
			height: parent.height;
			radius: height / 2;
			color: controller.running && !controller.connectionLost
						? Style.backgroundColor2
						: Style.alternateBaseColor;
			border.width: inputField.activeFocus && controller.running ? 1 : 0;
			border.color: Style.textSelectedColor;
			clip: true;

			// Placeholder (TextInput has no placeholderText in Qt 5.12).
			Text {
				anchors.left: parent.left;
				anchors.leftMargin: Style.marginM;
				anchors.right: parent.right;
				anchors.rightMargin: Style.marginM;
				anchors.verticalCenter: parent.verticalCenter;
				visible: inputField.text.length === 0;
				text: root.inputPlaceHolder();
				color: Style.placeHolderTextColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeM;
				elide: Text.ElideRight;
				z: 0;
			}

			TextInput {
				id: inputField;

				anchors.fill: parent;
				anchors.leftMargin: Style.marginM;
				anchors.rightMargin: Style.marginM;
				verticalAlignment: TextInput.AlignVCenter;
				font.family: Style.fontFamily
				font.pixelSize: Style.fontSizeM;
				color: Style.textColor;
				selectionColor: Style.textSelectedColor;
				selectedTextColor: "#ffffff";
				selectByMouse: true;
				clip: true;
				// Single-line only — Enter submits, does not insert a newline.
				echoMode: TextInput.Normal;
				readOnly: !controller.running || controller.connectionLost || controller.commandInFlight;
				activeFocusOnPress: !readOnly;

				// Primary path: TextInput emits accepted on Return/Enter.
				onAccepted: {
					root.submitCommand();
				}

				Keys.onUpPressed: {
					if (controller.running){
						root.historyUp();
						event.accepted = true;
					}
				}

				Keys.onDownPressed: {
					if (controller.running){
						root.historyDown();
						event.accepted = true;
					}
				}

				Keys.onPressed: {
					// Ctrl+C with no selection → interrupt remote (toolbar "Copy all" for log).
					if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C){
						if (controller.running && !controller.connectionLost
									&& inputField.selectedText.length === 0){
							controller.interruptSession();
							event.accepted = true;
						}
					}
					// Safety net if onAccepted is swallowed (some platforms/web builds).
					// Second call is harmless: submitCommand no-ops on empty text.
					if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter){
						if (controller.running && !controller.connectionLost){
							root.submitCommand();
							event.accepted = true;
						}
					}
				}
			}
		}

		Button {
			id: sendButton;
			anchors.right: parent.right;
			anchors.verticalCenter: parent.verticalCenter;
			widthFromDecorator: true;
			height: parent.height;
			text: qsTr("Send");
			enabled: controller.running && !controller.connectionLost
						&& !controller.commandInFlight && inputField.text.length > 0;
			tooltipText: qsTr("Send command (Enter)");
			onClicked: root.submitCommand();
		}
	}

	// ─── Status bar ─────────────────────────────────────────────────────────
	Item {
		id: statusBar;

		anchors.left: parent.left;
		anchors.leftMargin: Style.marginL;
		anchors.right: parent.right;
		anchors.rightMargin: Style.marginL;
		anchors.bottom: parent.bottom;
		anchors.bottomMargin: Style.marginM;
		height: Style.controlHeightM;

		Rectangle {
			anchors.left: parent.left;
			anchors.verticalCenter: parent.verticalCenter;
			width: Style.spacingXS;
			height: parent.height - Style.marginXS;
			radius: width / 2;
			color: root.statusColor();
		}

		Text {
			anchors.left: parent.left;
			anchors.leftMargin: Style.marginM;
			anchors.right: requestIndicator.left;
			anchors.rightMargin: Style.marginM;
			anchors.verticalCenter: parent.verticalCenter;
			elide: Text.ElideMiddle;
			text: root.statusLabel();
			color: (root.errorText !== "" || controller.connectionLost) ? Style.errorColor : Style.textColor;
			opacity: (root.errorText !== "" || root.actionHint !== "" || controller.connectionLost) ? 1 : 0.85;
			font.family: Style.fontFamily;
			font.pixelSize: Style.fontSizeM;
		}

		BusyIndicator {
			id: requestIndicator;
			anchors.right: lineCountText.left;
			anchors.rightMargin: Style.marginM;
			anchors.verticalCenter: parent.verticalCenter;
			width: Style.iconSizeS;
			height: Style.iconSizeS;
			visible: controller.requestInFlight && !controller.busy;
		}

		Text {
			id: lineCountText;
			anchors.right: parent.right;
			anchors.verticalCenter: parent.verticalCenter;
			elide: Text.ElideLeft;
			text: root.lineCountLabel();
			color: Style.textColor;
			opacity: 0.6;
			font.family: Style.fontFamily;
			font.pixelSize: Style.fontSizeS;
		}
	}
}
