import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0

/**
	Windows Terminal-style multi-tab remote terminal host.

	Each tab is an independent shell session (TerminalSessionPane + TerminalController).

	Tab strip:
	  [ tab | tab | tab ][ + ][ ▾ ]
	  · "+" opens a new tab with the preferred shell
	  · "▾" opens a shell list; pick one → new tab and remember as preferred
	  · default preferred shell: Command Prompt on Windows agents, Bash on Linux agents
	  · tabs compress evenly when the page shrinks; overflow uses TabPanel scroll arrows
	  · closing a tab removes it immediately (remote close is fire-and-forget on destroy)
*/
Item {
	id: root;

	// Agent the terminals are attached to (clientid header on every request).
	property string agentId: "";

	property int availableShellCount: 0;
	property int tabCounter: 0;

	// Preferred shell for "+" / first auto-open (set by shell menu or platform default).
	property string preferredShellTypeId: "";
	property string preferredShellName: "";

	// Map tabId -> { shellTypeId, shellName, title } until the pane Loader finishes.
	property var pendingTabMeta: ({});

	// Tab label width budget (driven by strip width and tab count).
	// Total tab ≈ label + chrome (icon, close slot, paddings, text-to-X gap).
	readonly property int tabIdealMaxWidth: 200;
	readonly property int tabHardMinWidth: 80;
	readonly property int tabChromeWidth: 72;
	property int tabMaxWidth: 200;

	TreeItemModel {
		id: shellModel;
	}

	function refreshShellTypes(){
		shellModel.clear();
		root.availableShellCount = 0;
		root.preferredShellTypeId = "";
		root.preferredShellName = "";
		if (root.agentId.length > 0){
			shellTypesController.agentId = root.agentId;
			shellTypesController.listShellTypes();
		}
	}

	// Normalize GraphQL / model shell type to a stable key (CMD / POWERSHELL / BASH / SH).
	// Handles bare names, dotted enum paths, and numeric enum values from the wire.
	function normalizeShellKey(typeId, name){
		let raw = typeId !== undefined && typeId !== null ? String(typeId).trim() : "";
		if (raw.length > 0){
			let parts = raw.split(".");
			let last = parts[parts.length - 1].toUpperCase();
			if (last === "0" || last === "CMD"){
				return "CMD";
			}
			if (last === "1" || last === "POWERSHELL"){
				return "POWERSHELL";
			}
			if (last === "2" || last === "BASH"){
				return "BASH";
			}
			if (last === "3" || last === "SH"){
				return "SH";
			}
			if (last === "CMD" || last === "POWERSHELL" || last === "BASH" || last === "SH"){
				return last;
			}
		}
		let n = name !== undefined && name !== null ? String(name).toLowerCase() : "";
		if (n.indexOf("command prompt") >= 0 || n === "cmd"){
			return "CMD";
		}
		if (n.indexOf("powershell") >= 0){
			return "POWERSHELL";
		}
		if (n.indexOf("bash") >= 0){
			return "BASH";
		}
		if (n === "shell" || n === "sh" || n.indexOf("bourne") >= 0){
			return "SH";
		}
		return raw.toUpperCase();
	}

	// Lower rank = higher preference for the first auto-open / "+" default.
	// Windows agents typically expose CMD (+ PowerShell); Linux agents expose Bash (+ sh).
	// Priority: Command Prompt → Bash → sh → PowerShell → anything else.
	function shellPreferenceRank(typeId, name){
		let key = root.normalizeShellKey(typeId, name);
		if (key === "CMD"){
			return 0;
		}
		if (key === "BASH"){
			return 1;
		}
		if (key === "SH"){
			return 2;
		}
		if (key === "POWERSHELL"){
			return 3;
		}
		return 10;
	}

	function isShellAvailableFlag(value){
		return value === true || value === "true" || value === 1 || value === "1";
	}

	function selectPreferredShellDefault(){
		let count = shellModel.getItemsCount();
		if (count <= 0){
			root.preferredShellTypeId = "";
			root.preferredShellName = "";
			return;
		}

		// Cross-platform: CMD if the agent has it (Windows), else Bash (Linux), else first.
		let pick = 0;
		let bestRank = 100;
		for (let i = 0; i < count; ++i){
			let typeId = shellModel.getData("id", i);
			let name = shellModel.getData("name", i);
			let rank = root.shellPreferenceRank(typeId, name);
			if (rank < bestRank){
				bestRank = rank;
				pick = i;
			}
		}
		root.preferredShellTypeId = String(shellModel.getData("id", pick));
		root.preferredShellName = String(shellModel.getData("name", pick));
	}

	function setPreferredShell(typeId, name){
		if (!typeId || String(typeId).length === 0){
			return;
		}
		root.preferredShellTypeId = String(typeId);
		root.preferredShellName = name && String(name).length > 0 ? String(name) : qsTr("Terminal");
	}

	function nextTabTitle(shellName){
		root.tabCounter = root.tabCounter + 1;
		// Windows Terminal style: "Command Prompt", "Command Prompt (2)", …
		let base = shellName.length > 0 ? shellName : qsTr("Terminal");
		let same = 0;
		for (let i = 0; i < tabView.tabModel.count; ++i){
			let n = tabView.tabModel.get(i).name;
			if (n === base || n.indexOf(base + " (") === 0){
				same = same + 1;
			}
		}
		if (same === 0){
			return base;
		}
		return base + " (" + (same + 1) + ")";
	}

	// Open a tab with the preferred shell, or with an explicit shell from the ▾ menu.
	function openNewTab(shellTypeId, shellName){
		let typeId = shellTypeId !== undefined && shellTypeId !== null && String(shellTypeId).length > 0
					? String(shellTypeId)
					: root.preferredShellTypeId;
		let name = shellName !== undefined && shellName !== null && String(shellName).length > 0
					? String(shellName)
					: root.preferredShellName;
		if (typeId.length === 0){
			return;
		}
		if (name.length === 0){
			name = qsTr("Terminal");
		}

		// Remember choice so further "+" uses the same shell.
		root.setPreferredShell(typeId, name);

		let tabId = "term-tab-" + root.tabCounter + "-" + UuidGenerator.generateUUID();
		let title = root.nextTabTitle(name);
		root.pendingTabMeta[tabId] = {
			"shellTypeId": typeId,
			"shellName": name,
			"title": title
		};
		root.pendingTabMeta = root.pendingTabMeta;

		tabView.addTab(tabId, title, sessionPaneComp, "Icons/Terminal", name, false, false);
		tabView.currentIndex = tabView.tabModel.count - 1;
		tabStrip.selectedIndex = tabView.currentIndex;
		root.updateTabMaxWidth();
	}

	function removeTabById(tabId){
		if (tabId.length === 0){
			return;
		}
		tabView.removeTab(tabId);
		if (root.pendingTabMeta[tabId] !== undefined){
			delete root.pendingTabMeta[tabId];
			root.pendingTabMeta = root.pendingTabMeta;
		}
		if (tabStrip.selectedIndex >= tabView.tabModel.count){
			tabStrip.selectedIndex = Math.max(0, tabView.tabModel.count - 1);
		}
		tabView.currentIndex = tabStrip.selectedIndex;
		root.updateTabMaxWidth();
	}

	// Immediate UI close — no confirm, no wait for server.
	// Enqueue remote close first, then drop the tab (destruction is a second safety net).
	function requestCloseTab(index){
		if (index < 0 || index >= tabView.tabModel.count){
			return;
		}
		let pane = tabView.getTabByIndex(index);
		if (pane && pane.closeSessionImmediate){
			pane.closeSessionImmediate();
		}
		root.removeTabById(tabView.getTabIdByIndex(index));
	}

	// Equal-share max label width so tabs shrink together before scroll arrows appear.
	function updateTabMaxWidth(){
		let n = tabView.tabModel.count;
		if (n <= 0){
			root.tabMaxWidth = root.tabIdealMaxWidth;
			return;
		}
		let trailing = trailingTools.width > 0
					? trailingTools.width
					: (2 * Style.controlHeightM + Style.spacingXS);
		let available = Math.max(0, stripRow.width - trailing - Style.spacingXS);
		let idealTotal = root.tabIdealMaxWidth + root.tabChromeWidth;
		if (n * idealTotal <= available){
			root.tabMaxWidth = root.tabIdealMaxWidth;
			return;
		}
		let shareTotal = Math.floor(available / n);
		let labelMax = shareTotal - root.tabChromeWidth;
		if (labelMax < root.tabHardMinWidth){
			root.tabMaxWidth = root.tabHardMinWidth;
			return;
		}
		root.tabMaxWidth = Math.min(root.tabIdealMaxWidth, labelMax);
	}

	function syncStripToBody(){
		if (tabStrip.selectedIndex !== tabView.currentIndex){
			tabStrip.selectedIndex = tabView.currentIndex;
		}
	}

	function syncBodyToStrip(){
		if (tabView.currentIndex !== tabStrip.selectedIndex){
			tabView.currentIndex = tabStrip.selectedIndex;
		}
	}

	function openShellMenu(){
		if (root.availableShellCount <= 0 || root.agentId.length === 0){
			return;
		}
		let point = shellMenuButton.mapToItem(null, 0, shellMenuButton.height);
		ModalDialogManager.openDialog(shellMenuComp, {
			"x": point.x,
			"model": shellModel,
			"width": Style.sizeHintXXS
		});
	}

	function preferredShellIndex(){
		let count = shellModel.getItemsCount();
		for (let i = 0; i < count; ++i){
			if (String(shellModel.getData("id", i)) === root.preferredShellTypeId){
				return i;
			}
		}
		return -1;
	}

	onAgentIdChanged: {
		while (tabView.tabModel.count > 0){
			let id = tabView.getTabIdByIndex(0);
			tabView.removeTab(id);
		}
		root.pendingTabMeta = ({});
		root.refreshShellTypes();
		root.updateTabMaxWidth();
	}

	onWidthChanged: {
		root.updateTabMaxWidth();
	}

	Component.onCompleted: {
		root.refreshShellTypes();
		root.updateTabMaxWidth();
	}

	Keys.onPressed: {
		// Windows Terminal: Ctrl+Shift+T — new tab; Ctrl+W / Ctrl+Shift+W — close tab.
		if ((event.modifiers & Qt.ControlModifier) && (event.modifiers & Qt.ShiftModifier)
					&& event.key === Qt.Key_T){
			if (root.availableShellCount > 0 && root.preferredShellTypeId.length > 0){
				root.openNewTab();
			}
			event.accepted = true;
		}
		else if ((event.modifiers & Qt.ControlModifier)
					&& (event.key === Qt.Key_W || ((event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_W))){
			if (tabView.tabModel.count > 0){
				root.requestCloseTab(tabView.currentIndex);
			}
			event.accepted = true;
		}
	}

	// Lightweight controller only for ListShellTypes (no session).
	TerminalController {
		id: shellTypesController;

		onShellTypesReceived: {
			shellModel.clear();
			let count = items ? items.getItemsCount() : 0;
			// Collect available shells first, then insert sorted by platform preference
			// so the ▾ menu lists native shells first (CMD… on Windows, Bash… on Linux).
			let collected = [];
			for (let i = 0; i < count; ++i){
				if (!root.isShellAvailableFlag(items.getData("available", i))){
					continue;
				}
				let typeId = items.getData("type", i);
				let name = items.getData("name", i);
				collected.push({
					"typeId": typeId,
					"name": name,
					"rank": root.shellPreferenceRank(typeId, name)
				});
			}
			collected.sort(function(a, b){
				if (a.rank !== b.rank){
					return a.rank - b.rank;
				}
				return String(a.name).localeCompare(String(b.name));
			});
			for (let j = 0; j < collected.length; ++j){
				let index = shellModel.insertNewItem();
				// Keep wire type as-is for OpenTerminalSession; do not re-encode.
				shellModel.setData("id", collected[j].typeId, index);
				shellModel.setData("name", collected[j].name, index);
			}
			root.availableShellCount = shellModel.getItemsCount();
			root.selectPreferredShellDefault();
			// Open one tab by default (Command Prompt on Windows agents, Bash on Linux).
			if (tabView.tabModel.count === 0 && root.availableShellCount > 0
						&& root.agentId.length > 0 && root.preferredShellTypeId.length > 0){
				root.openNewTab();
			}
			root.updateTabMaxWidth();
		}

		onErrorOccurred: {
		}
	}

	Component {
		id: sessionPaneComp;

		TerminalSessionPane {
			id: sessionPane;

			anchors.fill: parent;

			onRequestRemoveTab: {
				for (let i = 0; i < tabView.tabModel.count; ++i){
					if (tabView.getTabByIndex(i) === sessionPane){
						root.removeTabById(tabView.getTabIdByIndex(i));
						return;
					}
				}
			}

			onSessionStateChanged: {
			}
		}
	}

	// Shell list under the ▾ button (same PopupMenuDialog pattern as ComboBox).
	Component {
		id: shellMenuComp;

		PopupMenuDialog {
			id: shellPopup;

			opacity: 0;
			itemHeight: Style.controlHeightM;
			radius: Style.radiusM;
			hiddenBackground: true;
			shownItemsCount: 6;
			selectedIndex: root.preferredShellIndex();
			nameId: "name";

			onHeightChanged: {
				shellPopup.setY();
			}

			function setY(){
				if (shellPopup.height === 0){
					return;
				}
				let point = shellMenuButton.mapToItem(null, 0, shellMenuButton.height);
				let yPos = point.y;
				if (yPos + shellPopup.height > ModalDialogManager.activeView.height){
					shellPopup.y = point.y - shellPopup.height - shellMenuButton.height;
					shellPopup.isUpwards = true;
				}
				else{
					shellPopup.y = point.y;
				}
				shellPopup.opacity = 1;
			}

			onFinished: {
				// index < 0 → dismissed (outside click / Escape).
				if (index < 0){
					return;
				}
				let typeId = shellModel.getData("id", index);
				let name = shellModel.getData("name", index);
				root.openNewTab(typeId, name);
			}
		}
	}

	Component {
		id: terminalTabDecoratorComp;

		TerminalTabDecorator {
		}
	}

	// ─── Tab strip: tabs + "+" + shell menu (▾) ─────────────────────────────
	Item {
		id: stripRow;

		anchors.top: parent.top;
		anchors.left: parent.left;
		anchors.right: parent.right;
		anchors.leftMargin: Style.marginL;
		anchors.rightMargin: Style.marginL;
		anchors.topMargin: Style.marginM;

		height: Style.controlHeightL;

		Rectangle {
			anchors.fill: parent;
			color: Style.baseColor;
			radius: Style.radiusM;
		}

		Rectangle {
			anchors.left: parent.left;
			anchors.right: parent.right;
			anchors.bottom: parent.bottom;
			height: 1;
			color: Style.borderColor;
			opacity: 0.45;
		}

		TabPanel {
			id: tabStrip;

			anchors.left: parent.left;
			anchors.top: parent.top;
			anchors.bottom: parent.bottom;

			maxWidth: Math.max(0, stripRow.width - trailingTools.width);
			height: parent.height;
			color: "transparent";
			isCloseEnable: true;
			spacing: 0;
			visible: tabView.tabModel.count > 0;
			model: tabView.tabModel;
			tabDelegateDecorator: terminalTabDecoratorComp;

			tabDelegate: Component {
				TabDelegate {
					id: termTab;

					height: tabStrip.height;
					tabPanel: tabStrip;
					isCloseEnable: tabStrip.isCloseEnable;
					listView: tabStrip.tabsList;
					decorator: tabStrip.tabDelegateDecorator;
					maxWidth: root.tabMaxWidth;
					minWidth: root.tabHardMinWidth;

					onCloseSignal: {
						tabStrip.closeItem(model.index);
					}
				}
			}

			onTabClicked: {
				root.syncBodyToStrip();
			}

			onCloseItem: {
				root.requestCloseTab(index);
			}

			onLeftClicked: {
				if (tabStrip.selectedIndex > 0){
					tabStrip.selectedIndex = tabStrip.selectedIndex - 1;
					root.syncBodyToStrip();
				}
			}

			onRightClicked: {
				if (tabStrip.selectedIndex < tabView.tabModel.count - 1){
					tabStrip.selectedIndex = tabStrip.selectedIndex + 1;
					root.syncBodyToStrip();
				}
			}
		}

		// Trailing: "+" then shell-menu "▾" (Windows Terminal-style split new-tab).
		Row {
			id: trailingTools;

			anchors.left: tabStrip.visible ? tabStrip.right : parent.left;
			anchors.verticalCenter: parent.verticalCenter;
			height: parent.height;
			spacing: 0;

			Button {
				id: newTabButton;

				anchors.verticalCenter: parent.verticalCenter;
				width: Style.controlHeightM;
				height: Style.controlHeightM;
				enabled: root.availableShellCount > 0 && root.agentId.length > 0
						&& root.preferredShellTypeId.length > 0;
				tooltipText: root.preferredShellName.length > 0
							? qsTr("New tab — %1 (Ctrl+Shift+T)").arg(root.preferredShellName)
							: qsTr("New tab (Ctrl+Shift+T)");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Add", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.alternateBaseColor : "transparent";
						radius: Style.radiusM;
						border.width: 0;
					}
				}
				onClicked: root.openNewTab();
			}

			Button {
				id: shellMenuButton;

				anchors.verticalCenter: parent.verticalCenter;
				width: Style.controlHeightM;
				height: Style.controlHeightM;
				enabled: root.availableShellCount > 0 && root.agentId.length > 0;
				tooltipText: qsTr("New tab with shell…");
				iconSource: "qrc:/" + Style.getIconPath("Icons/Down", Icon.State.On,
					enabled ? Icon.Mode.Normal : Icon.Mode.Disabled);
				decorator: Component {
					ButtonDecorator {
						color: parent.hovered ? Style.alternateBaseColor : "transparent";
						radius: Style.radiusM;
						border.width: 0;
					}
				}
				onClicked: root.openShellMenu();
			}
		}

		onWidthChanged: {
			root.updateTabMaxWidth();
		}
	}

	Connections {
		target: trailingTools;

		function onWidthChanged(){
			root.updateTabMaxWidth();
		}
	}

	Connections {
		target: tabView;

		function onCurrentIndexChanged(){
			root.syncStripToBody();
		}
	}

	// ─── Session bodies (TabView chrome hidden — strip is custom above) ──────
	TabView {
		id: tabView;

		anchors.top: stripRow.bottom;
		anchors.topMargin: Style.spacingXS;
		anchors.left: parent.left;
		anchors.right: parent.right;
		anchors.bottom: parent.bottom;
		anchors.leftMargin: Style.marginL;
		anchors.rightMargin: Style.marginL;
		anchors.bottomMargin: Style.marginM;

		tabVisible: false;
		closable: true;
		contentColor: Style.baseColor;
		tabPanelColor: Style.baseColor;

		function onCloseTab(index){
			root.requestCloseTab(index);
		}

		onTabLoaded: {
			let meta = root.pendingTabMeta[tabId];
			if (!meta || !tabItem){
				return;
			}
			tabItem.agentId = root.agentId;
			tabItem.shellTypeId = meta.shellTypeId;
			tabItem.shellName = meta.shellName;
			if (!tabItem.running && !tabItem.busy){
				tabItem.startSession();
			}
			delete root.pendingTabMeta[tabId];
			root.pendingTabMeta = root.pendingTabMeta;
			root.updateTabMaxWidth();
		}
	}

	// Empty state when there are no tabs.
	Column {
		anchors.centerIn: parent;
		width: Math.min(parent.width - 2 * Style.marginXL, Style.sizeHintM);
		spacing: Style.spacingM;
		visible: tabView.tabModel.count === 0;
		z: 2;

		Text {
			width: parent.width;
			horizontalAlignment: Text.AlignHCenter;
			text: root.availableShellCount === 0
						? qsTr("No shell available")
						: qsTr("No terminal tabs");
			color: Style.textColor;
			font.family: Style.fontFamily;
			font.pixelSize: Style.fontSizeL;
			opacity: 0.85;
		}

		Text {
			width: parent.width;
			horizontalAlignment: Text.AlignHCenter;
			wrapMode: Text.WordWrap;
			text: root.availableShellCount === 0
						? qsTr("This agent reports no shell that can be started (Command Prompt / PowerShell on Windows, Bash / sh on Linux).")
						: qsTr("Press + for a new tab with the preferred shell, or ▾ to pick another. Default: Command Prompt on Windows agents, Bash on Linux.");
			color: Style.textColor;
			font.family: Style.fontFamily;
			font.pixelSize: Style.fontSizeS;
			opacity: 0.5;
		}

		Button {
			anchors.horizontalCenter: parent.horizontalCenter;
			widthFromDecorator: true;
			text: qsTr("New tab");
			visible: root.availableShellCount > 0 && root.agentId.length > 0
					&& root.preferredShellTypeId.length > 0;
			onClicked: root.openNewTab();
		}
	}
}
