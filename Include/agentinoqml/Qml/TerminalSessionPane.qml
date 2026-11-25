import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import Qt.labs.platform 1.1
import Qt.labs.settings 1.0

import "TerminalEmulator.js" as Emulator

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

	readonly property int maxOutputLines: 5000;
	readonly property int maxOutputLength: 200000;

	readonly property int idleCloseExitCode: -2;
	readonly property int manualCloseExitCode: -1;

	// ── Terminal emulator state ─────────────────────────────────────────────
	// The VT emulation (the grid of cells with their colors and attributes, the cursor, the modes and the parser of the escape
	// sequences) is TerminalEmulator.js; see Docs/Architecture/TerminalFullScreenApps.md. The pane keeps mirrors of a few values of
	// the emulator that the views bind to; syncEmulatorState() refreshes them after every chunk of the output.
	property var emu: Emulator.create(80, 24);
	// A program that owns the screen owns the whole pane: it is drawn by the grid view, not by the scrollback. It is a program of the alternate screen
	// (CSI ?1049h) or one that draws the main screen (mc.exe under ConPTY, which has no alternate screen); see updateOwnership of the emulator.
	property bool gridOwnsPane: false;
	// The emulator is on its alternate buffer (the pane gives the screen back at once when it leaves it, and after a delay in the other case).
	property bool altBuffer: false;
	property int gridCols: 80;
	property int gridRows: 24;
	property int cursorRow: 0;
	property int cursorCol: 0;
	property bool cursorVisible: true;
	// The cursor of the grid view blinks while the view has the focus; any output or key shows it again (the scrollback view does not blink: its text is laid out as a whole).
	property bool cursorBlinkOn: true;
	// Bumped once per processed chunk so the grid delegates' text bindings (which read this property) know to re-evaluate;
	// the grid of the emulator is changed in place and does not fire property-change notifications on its own.
	property int screenVersion: 0;
	// The mouse: whether the program asked for it (?1000, ?1002, ?1003), and the selection of the grid view while it did not.
	property bool mouseReporting: false;
	property bool mouseTracking: false;
	property var selection: null;
	property bool selecting: false;
	property int mouseButtonDown: -1;
	property int lastMouseRow: 0;
	property int lastMouseCol: 0;
	// The modifiers of the last key event: the mouse events of the engine do not carry them (Shift lets the selection through while the program has the mouse).
	property int keyModifiers: 0;

	// ── Primary screen ──────────────────────────────────────────────────────
	// The pty (ConPTY on Windows) addresses the visible screen with cursor moves and repaints all of it after a resize, so the
	// primary screen is the same grid. Rows scrolled off its top are committed to the scrollback; the visible rows are rendered
	// below it (currentLineHtml). A full-screen program that does not use the alternate screen (mc.exe) is drawn here.
	property string currentLineHtml: "";
	property string committedPlain: "";

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

	// ── Terminal emulator glue ──────────────────────────────────────────────

	// The colors that the emulator uses for the default foreground and background and for the cursor cell.
	function emulatorOptions(){
		return {defaultFg: Style.textColor, defaultBg: Style.baseColor, cursorFg: "#ffffff", cursorBg: Style.textSelectedColor, selectionFg: "#ffffff", selectionBg: Style.textSelectedColor};
	}

	// The cursor is on and stays on for a whole interval after an output or a key, as in xterm.
	function showCursorSolid(){
		root.cursorBlinkOn = true;
		if (cursorBlinkTimer.running){
			cursorBlinkTimer.restart();
		}
	}

	// Copies what the views bind to from the emulator and moves the focus when a program enters or leaves the alternate screen.
	function syncEmulatorState(){
		let wasOwned = root.gridOwnsPane;
		let leftAltBuffer = root.altBuffer && !root.emu.altActive;
		root.altBuffer = root.emu.altActive;
		root.gridCols = root.emu.cols;
		root.gridRows = root.emu.rows;
		root.cursorRow = root.emu.cursorRow;
		root.cursorCol = root.emu.cursorCol;
		root.cursorVisible = root.emu.cursorVisible;
		root.showCursorSolid();
		root.mouseReporting = root.emu.mouseMode > 0;
		root.mouseTracking = root.emu.mouseMode === 1003;
		root.screenVersion++;

		if (root.emu.altActive || root.emu.owned){
			ownedOffTimer.stop();
			root.gridOwnsPane = true;
			if (!wasOwned){
				gridView.forceActiveFocus();
			}
		}
		else if (wasOwned){
			if (leftAltBuffer){
				root.releaseGrid();
			}
			else if (!ownedOffTimer.running){
				// Not at once: a program may clear its screen and draw it again in the next chunk.
				ownedOffTimer.start();
			}
		}
	}

	// The pane goes back to the scrollback view when the program does not own the screen any more.
	function releaseGrid(){
		ownedOffTimer.stop();
		if (root.emu.altActive || root.emu.owned || !root.gridOwnsPane){
			return;
		}
		root.gridOwnsPane = false;
		root.updatePrimaryScreen();
		if (controller.running && !controller.connectionLost){
			outputText.forceActiveFocus();
		}
	}

	// A new empty emulator of this size (the output is cleared).
	function createEmulator(cols, rows){
		root.emu = Emulator.create(cols, rows);
		// The rows that scroll off the top of the primary screen go to the scrollback.
		root.emu.onScrollOut = root.commitGridRow;
		root.syncEmulatorState();
	}

	// Called whenever the panel's computed character size changes (see sendTerminalSize). The pty repaints the screen after the
	// resize; keeping the old rows until then avoids a blank flash.
	function applyLocalGridSize(columns, rows){
		if (columns === root.gridCols && rows === root.gridRows){
			return;
		}
		Emulator.resize(root.emu, columns, rows);
		root.syncEmulatorState();
		if (!root.gridOwnsPane){
			root.updatePrimaryScreen();
		}
	}

	// Feeds one chunk of the raw output of the pty to the emulator and sends what the terminal answers to its queries
	// (device attributes, the position of the cursor) back to the pty.
	function feedTerminal(text){
		Emulator.feed(root.emu, text);
		let replies = Emulator.takeReplies(root.emu);
		if (replies.length > 0 && controller.running && !controller.connectionLost){
			controller.sendRawInput(replies);
		}
		root.syncEmulatorState();
		if (!root.gridOwnsPane){
			root.updatePrimaryScreen();
		}
	}

	// Rich-text for one grid row of the full-screen view, including the cursor cell. Reads screenVersion purely so this binding
	// re-evaluates after each processed chunk (the grid itself is changed in place, not reassigned).
	function gridRowHtml(rowIndex){
		let dependency = root.screenVersion;
		if (rowIndex >= root.emu.rows){
			return "";
		}
		let cursorColumn = (root.cursorVisible && root.cursorBlinkOn && rowIndex === root.emu.cursorRow) ? root.emu.cursorCol : -1;
		let options = root.emulatorOptions();
		if (root.selection !== null){
			let range = Emulator.selectionColumns(root.selection, rowIndex, root.emu.cols);
			if (range !== null){
				options.selectedFrom = range[0];
				options.selectedTo = range[1];
			}
		}
		return "<span style=\"white-space:pre\">" + Emulator.rowHtml(root.emu, rowIndex, options, cursorColumn, false) + "</span>";
	}

	// The bytes that a key sends to the pty: the table of xterm in the emulator (modifiers of the cursor keys and F-keys, F11 and F12,
	// Alt, the control characters, the application mode of the cursor keys that the program asks for).
	function vtInputForKey(event){
		return Emulator.encodeKey(root.emu, {
			key: event.key,
			text: event.text,
			ctrl: (event.modifiers & Qt.ControlModifier) !== 0,
			alt: (event.modifiers & Qt.AltModifier) !== 0,
			shift: (event.modifiers & Qt.ShiftModifier) !== 0
		});
	}

	function sendAltScreenKey(event){
		if (!root.gridOwnsPane || !controller.running || controller.connectionLost){
			return;
		}

		let data = root.vtInputForKey(event);
		if (data.length > 0){
			controller.sendRawInput(data);
			event.accepted = true;
		}
	}

	// The keys of the grid view: the copy of the selection and the paste are the terminal's, the rest goes to the program.
	function handleGridKey(event){
		root.keyModifiers = event.modifiers;
		root.showCursorSolid();
		let ctrl = (event.modifiers & Qt.ControlModifier) !== 0;
		let shift = (event.modifiers & Qt.ShiftModifier) !== 0;
		if (ctrl && event.key === Qt.Key_C && (root.selection !== null || shift)){
			root.copyGridSelection();
			event.accepted = true;
			return;
		}
		if ((ctrl && event.key === Qt.Key_V) || (shift && event.key === Qt.Key_Insert)){
			root.pasteFromClipboard();
			event.accepted = true;
			return;
		}
		root.selection = null;
		root.sendAltScreenKey(event);
	}

	function copyGridSelection(){
		if (root.selection === null){
			return;
		}
		let text = Emulator.selectionText(root.emu, root.selection);
		if (text.length === 0){
			return;
		}
		root.pasteArmed = false;
		clipboardProxy.text = text;
		clipboardProxy.selectAll();
		clipboardProxy.copy();
		clipboardProxy.select(0, 0);
		root.showActionHint(qsTr("Selection copied"));
	}

	// The cell of the screen under a point of the output panel. The screen of a program that has the alternate screen is the grid view;
	// the screen of a program that draws the main one (mc.exe) is the last lines of the scrollback view.
	function terminalCell(x, y){
		let col = Math.floor((x - Style.marginS) / root.charWidth);
		let row = 0;
		if (root.gridOwnsPane){
			row = Math.floor((y - Style.marginS) / root.charHeight);
		}
		else{
			let screenTop = outputText.contentHeight - (root.lastPrimaryRow() + 1) * root.charHeight;
			row = Math.floor((y - Style.marginS + outputFlickable.contentY - screenTop) / root.charHeight);
		}
		return {row: Math.max(0, Math.min(root.gridRows - 1, row)), col: Math.max(0, Math.min(root.gridCols - 1, col))};
	}

	function sendMouse(action, button, row, col){
		if (!controller.running || controller.connectionLost){
			return;
		}
		let data = Emulator.encodeMouse(root.emu, {
			action: action,
			button: button,
			row: row,
			col: col,
			shift: (root.keyModifiers & Qt.ShiftModifier) !== 0,
			alt: (root.keyModifiers & Qt.AltModifier) !== 0,
			ctrl: (root.keyModifiers & Qt.ControlModifier) !== 0
		});
		if (data.length > 0){
			controller.sendRawInput(data);
		}
	}

	function terminalMousePressed(mouse){
		let cell = root.terminalCell(mouse.x, mouse.y);
		let shift = (root.keyModifiers & Qt.ShiftModifier) !== 0;
		root.lastMouseRow = cell.row;
		root.lastMouseCol = cell.col;
		if (root.mouseReporting && !shift){
			root.selection = null;
			root.mouseButtonDown = mouse.button === Qt.RightButton ? 2 : 0;
			root.sendMouse("press", root.mouseButtonDown, cell.row, cell.col);
		}
		else if (root.gridOwnsPane){
			// The program has no mouse: the pointer selects the cells.
			root.selection = {r1: cell.row, c1: cell.col, r2: cell.row, c2: cell.col};
			root.selecting = true;
			root.screenVersion++;
		}
	}

	function terminalMouseMoved(mouse){
		let cell = root.terminalCell(mouse.x, mouse.y);
		let moved = cell.row !== root.lastMouseRow || cell.col !== root.lastMouseCol;
		root.lastMouseRow = cell.row;
		root.lastMouseCol = cell.col;
		if (!moved){
			return;
		}
		if (root.selecting){
			root.selection = {r1: root.selection.r1, c1: root.selection.c1, r2: cell.row, c2: cell.col};
			root.screenVersion++;
		}
		else if (root.mouseReporting && root.mouseButtonDown >= 0){
			// Only the motion with a button down (a drag, one report for each cell that the pointer enters) goes to the program. The motion without a button
			// (?1003, "any motion") is not reported: every report is a request to the agent, a program has nothing to do with the pointer that only passes
			// over it, and what a program does with a click (mc: the panels, the menu, the key bar) needs the click.
			root.sendMouse("move", root.mouseButtonDown, cell.row, cell.col);
		}
	}

	function terminalMouseReleased(mouse){
		let cell = root.terminalCell(mouse.x, mouse.y);
		if (root.selecting){
			root.selecting = false;
			return;
		}
		if (root.mouseButtonDown >= 0){
			root.sendMouse("release", root.mouseButtonDown, cell.row, cell.col);
			root.mouseButtonDown = -1;
		}
	}

	// The wheel goes to the program as the mouse when it asked for the mouse. A program that owns the screen and did not (or cannot: ConPTY does not give
	// the mouse to a console program) gets the arrow keys, three for a notch, as in the other terminals on the alternate screen.
	function terminalWheel(direction){
		if (root.mouseReporting){
			root.sendMouse(direction < 0 ? "wheelUp" : "wheelDown", 0, root.lastMouseRow, root.lastMouseCol);
			return;
		}
		if (!controller.running || controller.connectionLost){
			return;
		}
		let arrow = Emulator.encodeKey(root.emu, {key: direction < 0 ? Qt.Key_Up : Qt.Key_Down, text: "", ctrl: false, alt: false, shift: false});
		controller.sendRawInput(arrow + arrow + arrow);
	}

	// The program that asked for the focus reports (?1004) is told when the view gets or loses it.
	function sendFocusEvent(focused){
		let data = Emulator.encodeFocus(root.emu, focused);
		if (data.length > 0 && controller.running && !controller.connectionLost){
			controller.sendRawInput(data);
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
		root.committedPlain = "";
		root.currentLineHtml = "";
		root.startupOutputPhase = true;
		root.createEmulator(root.gridCols, root.gridRows);
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
			// Our own message goes below whatever the shell has drawn so far.
			if (!root.gridOwnsPane){
				root.flushPrimaryScreen();
			}
			root.appendScrollbackText(text.endsWith("\n") ? text : text + "\n", streamId);
			root.updatePrimaryScreen();
			return;
		}

		root.feedTerminal(text);
	}

	// Index of the last primary row worth showing: the cursor row or a later non-blank one.
	function lastPrimaryRow(){
		let last = root.cursorRow;
		for (let r = root.gridRows - 1; r > last; --r){
			if (root.lastUsedColumn(root.emu.grid[r]) >= 0){
				return r;
			}
		}
		return last;
	}

	function lastUsedColumn(row){
		return Emulator.lastUsedColumn(row);
	}

	// One grid row as rich text without trailing blanks; cursorCol >= 0 draws the block cursor.
	function primaryRowHtml(row, cursorCol){
		let inner = Emulator.rowHtml(root.emu, row, root.emulatorOptions(), cursorCol, true);
		if (inner.length === 0){
			return "";
		}
		return "<span style=\"white-space:pre\">" + inner + "</span>";
	}

	function primaryRowText(row){
		return Emulator.rowText(root.emu, row);
	}

	function updatePrimaryScreen(){
		let showCursor = root.cursorVisible && controller.running && !controller.connectionLost;
		let last = root.lastPrimaryRow();
		let html = "";
		let plain = "";
		for (let r = 0; r <= last; ++r){
			let row = root.emu.grid[r];
			if (r > 0){
				html += "<br/>";
				plain += "\n";
			}
			html += root.primaryRowHtml(row, showCursor && r === root.cursorRow ? root.cursorCol : -1);
			plain += root.primaryRowText(row);
		}
		if (plain.trim().length === 0 && !showCursor){
			html = "";
			plain = "";
		}
		root.currentLineHtml = html;
		root.plainLog = root.committedPlain + plain;
		scrollToEndTimer.restart();
	}

	function commitGridRow(row){
		let text = root.primaryRowText(row);
		if (root.startupOutputPhase && text.trim().length === 0){
			return;
		}
		root.appendScrollbackLine(root.primaryRowHtml(row, -1) + "<br/>", text + "\n");
	}

	// Moves everything drawn on the primary screen into the scrollback and homes the cursor.
	function flushPrimaryScreen(){
		let last = root.lastPrimaryRow();
		let hasContent = last > 0 || root.lastUsedColumn(root.emu.grid[0]) >= 0;
		if (!hasContent){
			return;
		}
		for (let r = 0; r <= last; ++r){
			root.commitGridRow(root.emu.grid[r]);
		}
		Emulator.clearScreen(root.emu);
		root.syncEmulatorState();
	}

	function appendScrollbackLine(html, plain){
		outputModel.append({"html": html});
		root.richLog += html;
		root.committedPlain += plain;
		if (root.committedPlain.length > root.maxOutputLength){
			root.committedPlain = root.committedPlain.substring(root.committedPlain.length - root.maxOutputLength);
		}

		if (outputModel.count > root.maxOutputLines){
			while (outputModel.count > root.maxOutputLines){
				outputModel.remove(0);
			}
			root.richLog = "";
			for (let index = 0; index < outputModel.count; ++index){
				root.richLog += outputModel.get(index).html;
			}
		}
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
			root.appendScrollbackLine(root.ansiToHtml(display, color), root.visibleText(display));
		}

		scrollToEndTimer.restart();
	}

	function visibleText(text){
		return String(text)
					.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, "")
					.replace(/\x1b\][^\x07]*(?:\x07|\x1b\\)/g, "");
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

	// Sends clipboard text to the shell as typed input (Ctrl+V / Shift+Insert in the output field).
	// TextEdit.paste() is asynchronous in the web build, so the text is picked up in
	// clipboardProxy.onTextChanged while pasteArmed is set (a timer disarms it if the paste never arrives).
	property bool pasteArmed: false;

	function pasteFromClipboard(){
		if (!controller.running || controller.connectionLost){
			return;
		}

		root.pasteArmed = false;
		clipboardProxy.text = "";
		root.pasteArmed = true;
		pasteArmTimer.restart();
		clipboardProxy.paste();
	}

	function sendPastedText(text){
		root.pasteArmed = false;
		pasteArmTimer.stop();
		// The new lines are Enter; between ESC [ 200 ~ and ESC [ 201 ~ when the program asked for the bracketed paste.
		let data = Emulator.encodePaste(root.emu, String(text));
		clipboardProxy.text = "";
		if (data.length > 0 && controller.running && !controller.connectionLost){
			root.startupOutputPhase = false;
			controller.sendRawInput(data);
		}
	}

	function copyAllOutput(){
		if (root.plainLog.length === 0 || root.copyFeedbackActive){
			return;
		}

		root.pasteArmed = false;
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
			return qsTr("Session active · ↑/↓ history");
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
			return Style.imagingToolsAccentColor;
		}
		if (controller.busy){
			return Style.highlightColor;
		}
		if (controller.requestInFlight){
			return Style.highlightColor;
		}
		if (controller.running){
			return Style.imagingToolsAccentColor;
		}
		return Style.subtitleColor;
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
		let columns = Math.floor(outputFlickable.width / root.charWidth);
		let rows = Math.floor(outputFlickable.height / root.charHeight);
		// Not laid out yet: a one-row pty would scroll everything through a single line.
		if (columns < 2 || rows < 2){
			return;
		}
		root.applyLocalGridSize(columns, rows);
		controller.resizeSession(columns, rows);
	}

	onAgentIdChanged: {
		controller.agentId = root.agentId;
	}

	Component.onCompleted: {
		root.createEmulator(root.gridCols, root.gridRows);
		controller.agentId = root.agentId;
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
	// hidden Text probe and its contentWidth/contentHeight - both are backed by
	// JQApplication.TextController.measureTextFast in the web build, so this works
	// identically there and in the native QML build. The output font is proportional:
	// the probe holds typical shell text, since a too narrow grid breaks lines early
	// while a too wide one only wraps them visually.
	// The terminal needs a monospace font: the cells of a grid are columns, and a frame of a full-screen program (mc) lines up only when
	// every character is as wide as the others. The generic family is what the web build draws with the font of the browser.
	readonly property string terminalFontFamily: Qt.platform.os === "windows" ? "Consolas" : "monospace";
	readonly property int sizeMetricsProbeLength: 20;
	readonly property real charWidth: sizeMetricsProbe.contentWidth / root.sizeMetricsProbeLength;
	readonly property real charHeight: sizeMetricsProbe.contentHeight;

	Text {
		id: sizeMetricsProbe;
		visible: false;
		text: "D:/Work/dir_name.txt";
		wrapMode: Text.NoWrap;
		font.family: root.terminalFontFamily;
		font.pixelSize: Style.fontSizeM;
	}

	Timer {
		id: ownedOffTimer;
		interval: 400;
		repeat: false;
		onTriggered: root.releaseGrid();
	}

	Timer {
		id: cursorBlinkTimer;
		interval: 530;
		repeat: true;
		running: root.gridOwnsPane && root.cursorVisible && controller.running && gridView.activeFocus;
		onTriggered: {
			root.cursorBlinkOn = !root.cursorBlinkOn;
			root.screenVersion++;
		}
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

	Timer {
		id: pasteArmTimer;
		interval: 2000;
		repeat: false;
		onTriggered: root.pasteArmed = false;
	}

	TextEdit {
		id: clipboardProxy;
		width: 0;
		height: 0;
		visible: false;
		readOnly: false;

		onTextChanged: {
			if (root.pasteArmed && clipboardProxy.text.length > 0){
				root.sendPastedText(clipboardProxy.text);
			}
		}
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
			// Focus the output field: typing goes straight to the shell there.
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
				objectName: "ClearOutputButton";
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
				objectName: "CopyOutputButton";
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
				objectName: "DownloadOutputButton";
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
							: (controller.running ? Style.imagingToolsAccentColor : "transparent");
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
		anchors.bottom: statusBar.top;
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

			visible: !root.gridOwnsPane;
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
				font.family: root.terminalFontFamily;
				font.pixelSize: Style.fontSizeM;
				color: Style.textColor;
				selectionColor: Style.textSelectedColor;
				selectedTextColor: "#ffffff";
				// The committed scrollback plus the live (still-being-typed) line, so raw
				// keyboard echo and in-place line editing show up as one continuous line.
				text: root.richLog + root.currentLineHtml;

				onActiveFocusChanged: root.sendFocusEvent(activeFocus);

				Keys.onReleased: {
					root.keyModifiers = event.modifiers;
				}

				Keys.onPressed: {
					root.keyModifiers = event.modifiers;
					// Ctrl+C copies the selection when there is one (otherwise it falls
					// through to the shell below as a real interrupt).
					if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C
								&& outputText.selectedText.length > 0){
						root.pasteArmed = false;
						clipboardProxy.text = outputText.selectedText;
						clipboardProxy.selectAll();
						clipboardProxy.copy();
						clipboardProxy.select(0, 0);
						event.accepted = true;
						return;
					}

					// Paste goes to the shell as typed input.
					if (((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_V)
								|| ((event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_Insert)){
						root.pasteFromClipboard();
						event.accepted = true;
						return;
					}

					// Typing happens right here: every other key is sent straight to the shell,
					// which echoes it into this field and runs its own line editing / history /
					// Tab-completion; Enter executes the line.
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
			visible: !root.gridOwnsPane;
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

			visible: root.gridOwnsPane;
			focus: root.gridOwnsPane;
			clip: true;

			onActiveFocusChanged: root.sendFocusEvent(activeFocus);

			Keys.onReleased: {
				root.keyModifiers = event.modifiers;
			}

			Keys.onPressed: {
				root.handleGridKey(event);
			}

			Repeater {
				model: root.gridRows;

				delegate: Text {
					x: 0;
					y: index * root.charHeight;
					width: gridView.width;
					height: root.charHeight;
					textFormat: Text.RichText;
					font.family: root.terminalFontFamily;
					font.pixelSize: Style.fontSizeM;
					text: root.gridRowHtml(index);
				}
			}
		}

		// The mouse over the screen: the program gets it when it asked for it (?1000/?1002/?1003), otherwise the grid view selects with it.
		// The area is off while neither is true, so the scrollback view keeps its own selection and wheel.
		MouseArea {
			id: terminalMouse;

			anchors.fill: parent;
			z: 10;

			enabled: root.gridOwnsPane || root.mouseReporting;
			hoverEnabled: true;
			acceptedButtons: Qt.LeftButton | Qt.RightButton;

			onPressed: {
				root.terminalMousePressed(mouse);
			}

			onPositionChanged: {
				root.terminalMouseMoved(mouse);
			}

			onReleased: {
				root.terminalMouseReleased(mouse);
			}

			TerminalWheelHandler {
				enabled: root.mouseReporting || root.gridOwnsPane;
				onRolled: root.terminalWheel(direction);
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

			visible: false//controller.requestInFlight;
			height: Style.controlHeightM;
			width: requestPopupRow.implicitWidth + 2 * Style.marginL;
			radius: height / 2;
			color: Style.baseColor;
			border.color: Style.imagingToolsAccentColor;
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
