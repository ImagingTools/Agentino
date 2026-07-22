import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import Qt.labs.platform 1.1 as QLP
import Qt.labs.settings 1.0

/**
	Remote terminal page for AgentEditor.

	Output is a virtualised ListView of line items (not one growing RichText document).
	ANSI SGR colour codes in chunks are translated to HTML spans at render time;
	cursor/TUI sequences are intentionally not supported (documented empty-state note).
*/
Item {
	id: root;

	property alias agentId: controller.agentId;

	property int availableShellCount: 0;
	property string sessionShellName: "";
	property string errorText: "";
	property string actionHint: "";

	// Copy-all feedback: disable the button and show PopupView for a short time.
	property bool copyFeedbackActive: false;
	readonly property int copyFeedbackMs: 2000;

	// Plain text for copy/download (no HTML).
	property string plainLog: "";

	property var commandHistory: [];
	property int historyIndex: -1;
	property string historyDraft: "";
	readonly property int maxCommandHistory: 100;
	readonly property int maxOutputLines: 5000;
	readonly property int maxOutputLength: 200000;

	readonly property int idleCloseExitCode: -2;
	readonly property int manualCloseExitCode: -1;

	readonly property string monoFontFamily: "Courier New, Courier, monospace";

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
	}

	function appendOutput(text, stream){
		if (text.length === 0){
			return;
		}

		let streamId = stream !== undefined ? stream : "STDOUT";
		root.plainLog += text;
		if (root.plainLog.length > root.maxOutputLength){
			root.plainLog = root.plainLog.substring(root.plainLog.length - root.maxOutputLength);
		}

		// Split into lines for ListView virtualisation (keep trailing empty only if ends with \n).
		let parts = String(text).split("\n");
		let endsWithNl = String(text).endsWith("\n");
		let lineCount = endsWithNl ? parts.length - 1 : parts.length;
		let color = root.streamColor(streamId);

		for (let i = 0; i < lineCount; ++i){
			let line = parts[i];
			// Re-add newline for all but last incomplete line unless source ended with \n.
			let display = line + ((i < lineCount - 1 || endsWithNl) ? "\n" : "");
			if (display.length === 0){
				continue;
			}
			let html = root.ansiToHtml(display, color);
			outputModel.append({"plain": display, "html": html, "stream": streamId});
		}

		while (outputModel.count > root.maxOutputLines){
			outputModel.remove(0);
		}

		Qt.callLater(function(){
			if (outputListView.contentHeight > outputListView.height){
				outputListView.positionViewAtEnd();
			}
		});
	}

	function refreshShellTypes(){
		shellModel.clear();
		root.availableShellCount = 0;
		if (root.agentId.length > 0){
			controller.listShellTypes();
		}
	}

	function openSession(){
		if (shellSelector.currentIndex < 0){
			return;
		}

		root.errorText = "";
		root.actionHint = "";
		root.sessionShellName = shellModel.getData("name", shellSelector.currentIndex);
		root.clearOutput();
		controller.openSession(shellModel.getData("id", shellSelector.currentIndex));
	}

	function requestCloseSession(){
		if (!controller.running || controller.busy || controller.closeInFlight){
			return;
		}
		ModalDialogManager.openDialog(closeConfirmDialogComp, {});
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
		if (!controller.running || controller.connectionLost || inputField.text.length === 0){
			return;
		}

		// Multi-line paste: normalise CRLF and send as one block (shell gets newlines).
		let command = String(inputField.text).replace(/\r\n/g, "\n").replace(/\r/g, "\n");
		// Drop a single trailing newline from paste so we do not double-submit empty.
		if (command.endsWith("\n")){
			command = command.substring(0, command.length - 1);
		}
		if (command.length === 0){
			return;
		}

		root.pushCommandHistory(command.split("\n")[0]);
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

		logFileIO.write(root.plainLog);
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
			return qsTr("%1 session running").arg(root.sessionShellName);
		}
		if (controller.busy){
			return qsTr("Working…");
		}
		return qsTr("No session");
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
		if (controller.running){
			return qsTr("Session active · idle warning ~1 min before 15 min close · ↑/↓ history");
		}
		if (root.availableShellCount === 0){
			return qsTr("No shell is available on this agent");
		}
		return qsTr("No session — pick a shell and press Play");
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
			return qsTr("Type a command and press Enter (↑/↓ history, Ctrl+C interrupt)");
		}
		if (controller.busy){
			return qsTr("Please wait…");
		}
		if (root.availableShellCount === 0){
			return qsTr("No shell available on this agent");
		}
		return qsTr("Open a session to run commands");
	}

	function lineCountLabel(){
		let n = outputModel.count;
		return n === 1 ? qsTr("1 line") : qsTr("%1 lines").arg(n);
	}

	onAgentIdChanged: root.refreshShellTypes();

	Component.onCompleted: {
		root.loadCommandHistory();
		root.refreshShellTypes();
	}

	Component.onDestruction: {
		// Idempotent: forgetSession / closeInFlight guards double Close.
		controller.closeSession();
	}

	Timer {
		id: actionHintTimer;
		interval: 2500;
		repeat: false;
		onTriggered: root.actionHint = "";
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

	QLP.FileDialog {
		id: saveLogDialog;
		title: qsTr("Save terminal log");
		fileMode: QLP.FileDialog.SaveFile;
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

	Component {
		id: closeConfirmDialogComp;

		MessageDialog {
			width: Style.sizeHintM;
			title: qsTr("Close terminal session");
			message: qsTr("Terminate the shell process on the agent? Unsaved work in the session will be lost.");
			onFinished: {
				if (buttonId == Enums.yes){
					controller.closeSession();
				}
			}
		}
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
			inputEdit.forceActiveFocus();
		}

		onSessionClosed: {
			root.sessionShellName = "";
			if (exitCode === root.idleCloseExitCode){
				root.appendOutput(qsTr("\n[session closed: idle timeout]\n"), "SYSTEM");
			}
			else if (exitCode === root.manualCloseExitCode){
				root.appendOutput(qsTr("\n[session closed]\n"), "SYSTEM");
			}
			else{
				root.appendOutput(qsTr("\n[session closed, exit code %1]\n").arg(exitCode), "SYSTEM");
			}
		}

		onShellTypesReceived: {
			shellModel.clear();

			let count = items ? items.getItemsCount() : 0;
			for (let i = 0; i < count; ++i){
				if (items.getData("available", i) !== true){
					continue;
				}

				let index = shellModel.insertNewItem();
				shellModel.setData("id", items.getData("type", i), index);
				shellModel.setData("name", items.getData("name", i), index);
			}

			root.availableShellCount = shellModel.getItemsCount();
			shellSelector.currentIndex = root.availableShellCount > 0 ? 0 : -1;
		}

		onErrorOccurred: {
			root.errorText = message;
			root.appendOutput(qsTr("\n[error] %1\n").arg(message), "STDERR");
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
				id: openButton;
				widthFromDecorator: true;
				enabled: !controller.running && !controller.busy && shellSelector.currentIndex >= 0;
				tooltipText: qsTr("Open a session with the selected shell");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Play", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.buttonHoverColor : "transparent";
						radius: Style.radiusS;
						border.width: 0;
					}
				}
				onClicked: root.openSession();
			}

			Button {
				id: closeButton;
				widthFromDecorator: true;
				enabled: controller.running && !controller.busy && !controller.closeInFlight;
				tooltipText: qsTr("Terminate the session");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Stop", Icon.State.On,
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
			anchors.right: shellSelector.left;
			anchors.rightMargin: Style.marginM;
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

		ComboBox {
			id: shellSelector;
			anchors.right: parent.right;
			anchors.verticalCenter: parent.verticalCenter;
			width: Style.sizeHintXXS;
			height: parent.height;
			radius: height / 2;
			backgroundColor: Style.backgroundColor2;
			changeable: !controller.running && !controller.busy && root.availableShellCount > 0;
			placeHolderText: qsTr("Shell");
			model: TreeItemModel {
				id: shellModel;
			}
		}
	}

	// ─── Output panel (virtualised) ─────────────────────────────────────────
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

		ListView {
			id: outputListView;

			anchors.fill: parent;
			anchors.margins: Style.marginS;
			anchors.rightMargin: Style.marginS + Style.marginM;

			clip: true;
			boundsBehavior: Flickable.StopAtBounds;
			// Virtualisation: only visible delegates are created.
			cacheBuffer: Style.controlHeightM * 20;
			model: outputModel;

			delegate: Text {
				width: outputListView.width;
				textFormat: Text.RichText;
				wrapMode: Text.WrapAnywhere;
				font.family: root.monoFontFamily;
				font.pixelSize: Style.fontSizeM;
				text: model.html;
				// Accessible plain text for selection is limited in ListView; Copy all uses plainLog.
			}
		}

		CustomScrollbar {
			z: outputListView.z + 1;
			anchors.right: parent.right;
			anchors.rightMargin: Style.marginXS;
			anchors.top: outputListView.top;
			anchors.bottom: outputListView.bottom;
			secondSize: Style.marginS;
			radius: Style.radiusS;
			targetItem: outputListView;
		}

		Column {
			id: emptyState;
			anchors.centerIn: parent;
			width: Math.min(parent.width - 2 * Style.marginXL, Style.sizeHintM);
			spacing: Style.spacingM;
			visible: outputModel.count === 0 && !controller.busy;

			Text {
				width: parent.width;
				horizontalAlignment: Text.AlignHCenter;
				text: root.availableShellCount === 0
					? qsTr("No shell available")
					: qsTr("No terminal session");
				color: Style.textColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeL;
				opacity: 0.8;
				elide: Text.ElideRight;
			}

			Text {
				width: parent.width;
				horizontalAlignment: Text.AlignHCenter;
				wrapMode: Text.WordWrap;
				text: root.availableShellCount === 0
					? qsTr("This agent reports no shell that can be started (cmd/PowerShell on Windows, bash/sh on Linux).")
					: qsTr("Pick a shell and press Play. ANSI colours are supported; full TUI/cursor programs are not.");
				color: Style.textColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeS;
				opacity: 0.5;
			}

			Button {
				anchors.horizontalCenter: parent.horizontalCenter;
				widthFromDecorator: true;
				text: qsTr("Open session");
				visible: root.availableShellCount > 0 && !controller.running && !controller.busy;
				onClicked: root.openSession();
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

	// ─── Command input (TextEdit: multi-line paste) ─────────────────────────
	Item {
		id: inputRow;

		anchors.left: parent.left;
		anchors.leftMargin: Style.marginL;
		anchors.right: parent.right;
		anchors.rightMargin: Style.marginL;
		anchors.bottom: statusBar.top;
		anchors.bottomMargin: Style.marginM;

		height: Style.controlHeightM;

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
			border.width: inputEdit.activeFocus && controller.running ? 1 : 0;
			border.color: Style.textSelectedColor;
			clip: true;

			Flickable {
				id: inputFlick;
				anchors.fill: parent;
				anchors.leftMargin: Style.marginM;
				anchors.rightMargin: Style.marginM;
				contentWidth: inputEdit.paintedWidth;
				contentHeight: height;
				clip: true;
				flickableDirection: Flickable.HorizontalFlick;
				boundsBehavior: Flickable.StopAtBounds;

				TextEdit {
					id: inputEdit;

					height: inputFlick.height;
					width: Math.max(inputFlick.width, paintedWidth);
					verticalAlignment: TextEdit.AlignVCenter;
					font.family: root.monoFontFamily;
					font.pixelSize: Style.fontSizeM;
					color: Style.textColor;
					selectByMouse: true;
					readOnly: !controller.running || controller.connectionLost;
					wrapMode: TextEdit.NoWrap;
					textFormat: TextEdit.PlainText;

					Keys.onReturnPressed: {
						if (event.modifiers & Qt.ShiftModifier){
							// Allow Shift+Enter to insert a newline for multi-line paste editing.
							event.accepted = false;
							return;
						}
						if (controller.running && !controller.connectionLost){
							root.submitCommand();
							event.accepted = true;
						}
					}

					Keys.onEnterPressed: {
						if (event.modifiers & Qt.ShiftModifier){
							event.accepted = false;
							return;
						}
						if (controller.running && !controller.connectionLost){
							root.submitCommand();
							event.accepted = true;
						}
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
						// Ctrl+C while focus is in the input: interrupt remote command
						// (not local copy — use toolbar Copy all for log).
						if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C){
							if (controller.running && !controller.connectionLost
										&& inputEdit.selectedText.length === 0){
								controller.interruptSession();
								event.accepted = true;
							}
						}
					}
				}
			}

			Text {
				anchors.left: parent.left;
				anchors.leftMargin: Style.marginM;
				anchors.right: parent.right;
				anchors.rightMargin: Style.marginM;
				anchors.verticalCenter: parent.verticalCenter;
				visible: inputEdit.text.length === 0;
				text: root.inputPlaceHolder();
				color: Style.placeHolderTextColor;
				font.family: Style.fontFamily;
				font.pixelSize: Style.fontSizeM;
				elide: Text.ElideRight;
				z: 1;
			}
		}

		// Compatibility alias for history helpers that use inputField.text.
		QtObject {
			id: inputField;
			property alias text: inputEdit.text;
			function setFocus(v){
				if (v){
					inputEdit.forceActiveFocus();
				}
			}
			function forceActiveFocus(){
				inputEdit.forceActiveFocus();
			}
		}

		Button {
			id: sendButton;
			anchors.right: parent.right;
			anchors.verticalCenter: parent.verticalCenter;
			widthFromDecorator: true;
			height: parent.height;
			text: qsTr("Send");
			enabled: controller.running && !controller.connectionLost && inputEdit.text.length > 0;
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
			anchors.right: lineCountText.left;
			anchors.rightMargin: Style.marginM;
			anchors.verticalCenter: parent.verticalCenter;
			elide: Text.ElideMiddle;
			text: root.statusLabel();
			color: (root.errorText !== "" || controller.connectionLost) ? Style.errorColor : Style.textColor;
			opacity: (root.errorText !== "" || root.actionHint !== "" || controller.connectionLost) ? 1 : 0.85;
			font.family: Style.fontFamily;
			font.pixelSize: Style.fontSizeM;
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
