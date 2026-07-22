import QtQuick 2.15
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentino 1.0
import agentinoServicesSdl 1.0

DocumentViewBase {
	id: serviceEditorContainer
	
	anchors.fill: parent
	contentColor: Style.baseColor
	
	property ServiceData serviceData: model

	// Agent id used for agent-scoped GQL routing (header clientid). Overridden by ServiceEditorWrap.
	property string clientId: ""

	// Server UI hosts the Agents document service → path browse is remote and needs clientId.
	// Agent app has no Agents page → path browse hits local FileSystemController, clientId not required.
	property bool remoteAgentBrowse: false

	// Browse enabled when local (agent app) or when a target agent id is known (server app).
	property bool pathBrowseEnabled: !serviceEditorContainer.remoteAgentBrowse
		|| serviceEditorContainer.clientId !== ""
	
	property bool pluginLoaded: false
	property bool pluginLoading: false
	property bool pluginLoadFailed: false
	property string pluginServicePath: ""

	function getHeaders(){
		// Prefer typed DataScope when set; fall back to editor properties (QG1 transition).
		if (dataScope !== null && (dataScope.agentId.length > 0 || dataScope.serviceId.length > 0)) {
			let scoped = {}
			if (dataScope.agentId.length > 0)
				scoped["clientid"] = dataScope.agentId
			if (dataScope.serviceId.length > 0)
				scoped["serviceid"] = dataScope.serviceId
			return scoped
		}
		let headers = {}
		if (serviceEditorContainer.clientId !== ""){
			headers["clientid"] = serviceEditorContainer.clientId
			if (dataScope !== null)
				dataScope.agentId = serviceEditorContainer.clientId
		}
		if (serviceEditorContainer.serviceData){
			headers["serviceid"] = serviceEditorContainer.serviceData.m_id
			if (dataScope !== null)
				dataScope.serviceId = serviceEditorContainer.serviceData.m_id
		}
		return headers
	}

	function pathBrowsePlaceHolder(forRemoteOnAgent){
		if (!serviceEditorContainer.remoteAgentBrowse){
			return forRemoteOnAgent
				? qsTr("Browse for the file")
				: qsTr("Browse for the executable")
		}
		if (serviceEditorContainer.clientId !== ""){
			return forRemoteOnAgent
				? qsTr("Browse for the file on the agent")
				: qsTr("Browse for the executable on the agent")
		}
		return qsTr("Select an agent first to browse")
	}
	
	function requestLoadPlugin() {
		pluginLoaded = false
		pluginLoading = true
		pluginLoadFailed = false
		loadPlugin()
	}
	
	function setPluginLoaded(path) {
		pluginServicePath = path
		pluginLoaded = true
		pluginLoadFailed = false
	}
	
	function handlePluginPathChange(newPath) {
		if (newPath !== pluginServicePath) {
			if (pluginLoaded) {
				pluginLoaded = false
				pluginLoadFailed = false
			}
		} else {
			pluginLoaded = true
		}
	}

	Component.onCompleted: {
		// Detect server vs agent app (same rule as TopologyPage new-service flow).
		serviceEditorContainer.remoteAgentBrowse = !!MainDocumentService.getDocumentService("Agents")

		// Agent: no permission gating for the editor. Server: keep ChangeService lock.
		if (serviceEditorContainer.remoteAgentBrowse){
			let ok = PermissionsController.checkPermission("ChangeService");
			serviceEditorContainer.readOnly = !ok;
		}
		else{
			serviceEditorContainer.readOnly = false;
		}
		refreshIsNewService()
		multiPageView.addServicePages()
	}
	
	signal loadPlugin()
	signal pluginLoadingFailed()
	signal loadSettings()
	signal saveSettings(string content)

	// Candidate producers are fetched on demand (they are a fleet-wide scan), never packed
	// into GetService. The editor asks once the output slots are known; the L3 adapter
	// answers with setAvailableConnections().
	signal requestAvailableConnections(var connectionUsageIds)

	// Picking a producer for one Output Connection applies immediately via its own
	// mutation (SetOutputConnection) - it is deliberately NOT part of serviceData /
	// doUpdateModel(), so choosing a connection never requires saving the whole service
	// record. dependantConnectionId === "" clears the selection. The L3 adapter answers
	// with outputConnectionApplied() once the agent confirms (or rejects) the change.
	signal setOutputConnection(string connectionId, string dependantConnectionId)
	signal outputConnectionApplied(string connectionId, bool successful, var connectionParam)

	// usage id -> list of candidate endpoints for that output slot.
	property var availableConnectionsByUsage: ({})
	property bool availableConnectionsLoading: false
	signal availableConnectionsUpdated()

	function requestAvailableConnectionsForOutputs(){
		if (!serviceEditorContainer.serviceData || !serviceEditorContainer.serviceData.hasOutputConnections()){
			return
		}

		let outputs = serviceEditorContainer.serviceData.m_outputConnections
		let usageIds = []
		for (let i = 0; i < outputs.count; i++){
			let entry = outputs.get(i)
			let outputConnection = entry ? entry.item : null
			if (outputConnection && outputConnection.m_id){
				usageIds.push("" + outputConnection.m_id)
			}
		}

		if (usageIds.length === 0){
			return
		}

		serviceEditorContainer.availableConnectionsLoading = true
		serviceEditorContainer.requestAvailableConnections(usageIds)
	}

	function setAvailableConnections(payload){
		serviceEditorContainer.availableConnectionsLoading = false

		let byUsage = ({})
		if (payload && payload.hasOutputConnections && payload.hasOutputConnections()){
			let groups = payload.m_outputConnections
			for (let i = 0; i < groups.count; i++){
				let entry = groups.get(i)
				let group = entry ? entry.item : null
				if (!group || !group.m_connectionUsageId){
					continue
				}
				// Copy: the payload belongs to the request sender and does not outlive it.
				if (group.hasCandidates && group.hasCandidates()){
					byUsage["" + group.m_connectionUsageId] = group.m_candidates.copyMe()
				}
			}
		}

		// Reassign wholesale so the delegates' bindings re-evaluate.
		serviceEditorContainer.availableConnectionsByUsage = byUsage
		serviceEditorContainer.availableConnectionsUpdated()
	}

	function getAvailableConnectionsFor(connectionUsageId){
		if (!connectionUsageId){
			return null
		}
		let map = serviceEditorContainer.availableConnectionsByUsage
		if (!map){
			return null
		}
		return map["" + connectionUsageId] || null
	}

	Connections {
		target: serviceEditorContainer
		function onPluginLoadedChanged() {
			if (serviceEditorContainer.pluginLoaded) {
				serviceEditorContainer.pluginLoading = false
				serviceEditorContainer.pluginLoadFailed = false
			}
		}
		function onPluginLoadingFailed() {
			serviceEditorContainer.pluginLoading = false
			serviceEditorContainer.pluginLoadFailed = true
		}
	}
	
	property bool isNewService: true
	property string serviceStatus: ""
	readonly property bool serviceRunning: serviceStatus === ServiceStatus.s_Running
	property bool operationInProgress: false
	property string operationDescription: ""
	property bool serviceIsDirty: false
	property int editorControlWidth: Style.sizeHintXL

	function getEditorControlWidth(element){
		return Math.min(editorControlWidth,
			Math.max(0, element.width - 2 * element.contentMargin - element.nameMargin - Style.sizeHintBXS))
	}

	// Normalize status strings into agentino.ServiceStatus tokens
	// (ServiceStatus.s_Running === "running", s_NotRunning === "notRunning", ...).
	// Sources differ:
	//   - GetService / ProcessState: "running" / "notRunning"
	//   - GraphQL ServiceStatus enum / Start-Stop: "RUNNING" / "NOT_RUNNING"
	//   - I_DECLARE_ENUM ToString on subscriptions: "SS_RUNNING"
	//   - Display labels: "Running" / "Not running"
	// Must return ServiceStatus.s_* values — statusPopup compares against those.
	property var serviceStatusModel: ServiceStatusModel {}
	property var dataScope: DataScope { agentId: ""; serviceId: "" }

	function normalizeServiceStatus(status){
		if (serviceStatusModel !== null) {
			// unknown/failed/crashed → s_Undefined (Start/Stop disabled when agent offline).
			let n = serviceStatusModel.normalize(status)
			if (n === serviceStatusModel.running) return ServiceStatus.s_Running
			if (n === serviceStatusModel.starting) return ServiceStatus.s_Starting
			if (n === serviceStatusModel.stopping) return ServiceStatus.s_Stopping
			if (n === serviceStatusModel.stopped) return ServiceStatus.s_NotRunning
			if (n === serviceStatusModel.unknown
						|| n === serviceStatusModel.failed
						|| n === serviceStatusModel.crashed)
				return ServiceStatus.s_Undefined
		}
		return _legacyNormalizeServiceStatus(status)
	}

	function _legacyNormalizeServiceStatus(status){
		if (status === undefined || status === null || status === "")
			return ServiceStatus.s_NotRunning
		let value = String(status)
		switch (value){
			case ServiceStatus.s_Running:
			case "RUNNING":
			case "SS_RUNNING":
			case "Running":
				return ServiceStatus.s_Running
			case ServiceStatus.s_NotRunning:
			case "NOT_RUNNING":
			case "SS_NOT_RUNNING":
			case "NotRunning":
			case "Not running":
			case "notRunning":
				return ServiceStatus.s_NotRunning
			case ServiceStatus.s_Starting:
			case "STARTING":
			case "SS_STARTING":
			case "Starting":
				return ServiceStatus.s_Starting
			case ServiceStatus.s_Stopping:
			case "STOPPING":
			case "SS_STOPPING":
			case "Stopping":
				return ServiceStatus.s_Stopping
			case ServiceStatus.s_Undefined:
			case "UNDEFINED":
			case "SS_UNDEFINED":
			case "RUNNING_IMPOSSIBLE":
			case "Undefined":
			case "undefined":
				return ServiceStatus.s_Undefined
		}
		return ServiceStatus.s_NotRunning
	}

	function setServiceStatus(status){
		serviceStatus = normalizeServiceStatus(status)
	}

	// isNewService must reflect whether the document has actually been persisted via
	// Save, not whether serviceData.m_id is empty: the document framework
	// (DocumentService.qml) allocates a real object id for a new service as soon as
	// the "New" command creates the draft document, well before the user clicks Save -
	// so m_id is never actually empty once the editor is open, and the old check was
	// always false. documentManager.documentIsNew(documentId) tracks the real
	// saved/unsaved state (flips to false only after a real Save - see
	// DocumentService.qml's onDocumentSaved), so use that instead.
	function refreshIsNewService(){
		if (!documentManager || documentId === "")
			return
		if (typeof documentManager.documentIsNew !== "function")
			return
		isNewService = documentManager.documentIsNew(documentId)
	}

	function ensureLogPage(){
		if (isNewService || multiPageView.getIndexById("Log") >= 0){
			return
		}
		multiPageView.addPage("Log", qsTr("Service Log"), serviceLogPageComp, "Icons/EventLog")
	}

	onDocumentIdChanged: refreshIsNewService()
	onDocumentManagerChanged: refreshIsNewService()
	onIsNewServiceChanged: {
		if (!isNewService){
			ensureLogPage()
		}
	}

	Connections {
		target: serviceEditorContainer.documentManager
		function onDocumentSaved(savedDocumentId){
			if (savedDocumentId === serviceEditorContainer.documentId){
				serviceEditorContainer.refreshIsNewService()
				serviceEditorContainer.ensureLogPage()
			}
		}
		// openDocument marks isNew=false before the view gets documentId; once the
		// model is bound and documentOpened fires, re-sync so "Not saved yet" does
		// not stick on an existing service opened at first start.
		function onDocumentOpened(openedDocumentId){
			if (openedDocumentId === serviceEditorContainer.documentId){
				serviceEditorContainer.refreshIsNewService()
				serviceEditorContainer.ensureLogPage()
			}
		}
		function onDocumentAdded(addedDocumentId){
			if (addedDocumentId === serviceEditorContainer.documentId){
				serviceEditorContainer.refreshIsNewService()
			}
		}
	}
	
	property bool settingsFileExists: false
	property string settingsPath: ""
	property string settingsContent: ""
	
	function setSettings(content, exists, path){
		settingsFileExists = exists;
		settingsPath = path;
		settingsContent = content;
		let index = multiPageView.getIndexById("Settings");
		if (index < 0){
			return;
		}
		multiPageView.ensurePageLoaded(index);
		let item = multiPageView.getPageByIndex(index);
		if (item){
			item.updateGui();
		}
	}
	
	property string serviceTypeId: serviceData ? serviceData.m_serviceTypeId : ""
	onServiceTypeIdChanged: {
		if (serviceTypeId !== "" && multiPageView.getIndexById("Administration") < 0) {
			multiPageView.addPage("Administration", qsTr("Administration"), administrationViewComp, "Icons/AdminPanel")
		}
	}
	
	onServiceDataChanged: {
		if (serviceData){
			refreshIsNewService()
			ensureLogPage()
			setServiceStatus(serviceData.m_status)
			operationInProgress = false
			if (pluginServicePath === "" && serviceData.hasPath()){
				pluginServicePath = serviceData.m_path
			}
			if (serviceData.hasInputConnections() && serviceData.m_inputConnections.count > 0){
				pluginLoading = false
				pluginLoadFailed = false
				setPluginLoaded(serviceData.m_path)
				// setPluginLoaded() only flips the flag; subpage ListViews are populated via updateGui()
				updateGui()
			}
			if (serviceData.m_settingsPath !== undefined && serviceData.m_settingsPath !== ""){
				settingsPath = serviceData.m_settingsPath
			}
		}
	}
	
	function setOperationInProgress(inProgress, description) {
		operationInProgress = inProgress
		operationDescription = inProgress ? description : ""
	}
	
	function updateGui(){
		let idx = multiPageView.getIndexById("Information");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Options");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Server");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("OutputConnections");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
		idx = multiPageView.getIndexById("Settings");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateGui();
		}
	}
	
	function updateModel(){
		let idx = multiPageView.getIndexById("Information");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Options");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Server");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("OutputConnections");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
		idx = multiPageView.getIndexById("Settings");
		if (idx >= 0) {
			multiPageView.ensurePageLoaded(idx);
			let item = multiPageView.getPageByIndex(idx);
			if (item) item.updateModel();
		}
	}
	
	Rectangle {
		id: statusPopup
		property int statusIndicatorWidth: serviceEditorContainer.operationInProgress ? 18 :
			(serviceEditorContainer.isNewService ? 0 : 10)
		property string statusText: serviceEditorContainer.operationInProgress ? serviceEditorContainer.operationDescription :
			(serviceEditorContainer.isNewService ? qsTr("Not saved yet") :
				(serviceEditorContainer.serviceStatus === ServiceStatus.s_Running ? qsTr("Running") :
					(serviceEditorContainer.serviceStatus === ServiceStatus.s_Starting ? qsTr("Starting") :
						(serviceEditorContainer.serviceStatus === ServiceStatus.s_Stopping ? qsTr("Stopping") :
							(serviceEditorContainer.serviceStatus === ServiceStatus.s_NotRunning ? qsTr("Stopped") : qsTr("Unknown"))))))
		anchors.top: parent.top
		anchors.right: parent.right
		anchors.topMargin: Style.marginXL
		anchors.rightMargin: Style.marginXL
		width: Math.min(statusTextItem.implicitWidth + statusIndicatorWidth +
			(statusIndicatorWidth > 0 ? statusPopupRow.spacing : 0) + 2 * Style.marginL,
			Math.max(0, parent.width - 2 * Style.marginXL))
		height: 36
		clip: true
		radius: height / 2
		color: Style.backgroundColor
		border.color: serviceEditorContainer.operationInProgress ? Style.lightBlueColor :
			(serviceEditorContainer.isNewService ? Style.borderColor :
				(serviceEditorContainer.serviceRunning ? Style.greenColor :
					(serviceEditorContainer.serviceStatus === ServiceStatus.s_Starting ||
					serviceEditorContainer.serviceStatus === ServiceStatus.s_Stopping ? Style.lightBlueColor : Style.errorColor)))
		border.width: 1
		z: 100
		
		Row {
			id: statusPopupRow
			anchors.centerIn: parent
			spacing: Style.spacingS
			
			Loading {
				width: 18
				height: 18
				indicatorSize: 14
				anchors.verticalCenter: parent.verticalCenter
				background.color: "transparent"
				visible: serviceEditorContainer.operationInProgress
			}
			
			Rectangle {
				width: 10
				height: 10
				radius: 5
				anchors.verticalCenter: parent.verticalCenter
				color: serviceEditorContainer.serviceRunning ? Style.greenColor : Style.errorColor
				visible: !serviceEditorContainer.isNewService && !serviceEditorContainer.operationInProgress
			}
			
			BaseText {
				id: statusTextItem
				anchors.verticalCenter: parent.verticalCenter
				width: Math.max(0, statusPopup.width - 2 * Style.marginL -
					statusPopup.statusIndicatorWidth - (statusPopup.statusIndicatorWidth > 0 ? statusPopupRow.spacing : 0))
				text: statusPopup.statusText
				font.pixelSize: Style.fontSizeM
				elide: Text.ElideRight
			}
		}
	}
	
	MultiPageView {
		id: multiPageView
		anchors.fill: parent
		panelWidth: Style.sizeHintXXS

		function addServicePages() {
			multiPageView.clear()
			multiPageView.addPage("General", qsTr("General"), null, "Icons/Settings")
			multiPageView.addSubPage("General", "Information", qsTr("Information"), generalPageComp)
			multiPageView.addSubPage("General", "Options", qsTr("Options"), optionsPageComp)
			multiPageView.addPage("Connections", qsTr("Connections"), null, "Icons/Link")
			multiPageView.addSubPage("Connections", "Server", qsTr("Server"), serverPageComp)
			multiPageView.addSubPage("Connections", "OutputConnections", qsTr("Output Connections"), outputConnectionsPageComp)
			// Settings page is hidden for now - not safe yet.
			if (serviceEditorContainer.serviceTypeId !== "") {
				multiPageView.addPage("Administration", qsTr("Administration"), administrationViewComp, "Icons/AdminPanel")
			}
			serviceEditorContainer.ensureLogPage()
			// Categories are collapsed by default (MultiPageView.addPage()/addSubPage() always
			// start with expanded=false) - expand them so their subpages are visible right away.
			multiPageView.toggleExpanded("General")
			multiPageView.toggleExpanded("Connections")
			multiPageView.currentIndex = multiPageView.getIndexById("Information")
		}
	}
	
	
	
	Component {
		id: generalPageComp
		
		Flickable {
			id: generalFlickable
			
			anchors.fill: parent
			anchors.leftMargin: Style.marginXL
			anchors.topMargin: Style.marginXL
			anchors.rightMargin: Style.marginXL
			anchors.bottomMargin: Style.marginXL
			
			contentWidth: generalColumn.width
			contentHeight: generalColumn.height + Style.marginXL
			
			boundsBehavior: Flickable.StopAtBounds
			clip: true
			
			function updateGui(){
				if (!serviceEditorContainer.serviceData){
					return
				}

				nameInput.text = serviceEditorContainer.serviceData.m_name;
				descriptionInput.text = serviceEditorContainer.serviceData.m_description; 
				pathInput.path = serviceEditorContainer.serviceData.m_path;

				argumentsInput.text = serviceEditorContainer.serviceData.m_arguments;
			}
			
			function updateModel(){
				if (!serviceEditorContainer.serviceData){
					return
				}

				serviceEditorContainer.serviceData.m_name = nameInput.text;
				serviceEditorContainer.serviceData.m_description = descriptionInput.text;
				serviceEditorContainer.serviceData.m_path = pathInput.path;
				serviceEditorContainer.serviceData.m_arguments = argumentsInput.text;
			}
			
			Column {
				id: generalColumn
				x: Math.max(0, (generalFlickable.width - width) / 2)
				width: Math.max(0, Math.min(900, generalFlickable.width - 2 * Style.marginXL))
				spacing: Style.marginXL
				
				Column {
					width: parent.width
					spacing: Style.marginL
					
					GroupHeaderView {
						title: qsTr("Information")
						width: parent.width
						groupView: generalGroup
					}
					
					GroupElementView {
						id: generalGroup
						width: parent.width
						
						TextInputElementView {
							id: nameInput
							controlWidth: serviceEditorContainer.getEditorControlWidth(nameInput)
							name: qsTr("Name")
							description: qsTr("Identifies the service in lists, topology, and editor tabs")
							placeHolderText: qsTr("Enter the name");
							textInputValidator: nameRequiredRegexp;
							showErrorWhenInvalid: true;
							errorText: qsTr("Please enter the name");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: descriptionInput;
							
							RegularExpressionValidator {
								id: nameRequiredRegexp;
								regularExpression: /^(?!\s*$).+/;
							}
						}
						
						TextInputElementView {
							id: descriptionInput
							controlWidth: serviceEditorContainer.getEditorControlWidth(descriptionInput)
							name: qsTr("Description")
							description: qsTr("Explains the service purpose and is shown in service views")
							placeHolderText: qsTr("Enter the description");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: pathInput;
						}
						
						ServerPathPickerElementView {
							id: pathInput
							controlWidth: serviceEditorContainer.getEditorControlWidth(pathInput)
							name: qsTr("Path")
							description: qsTr("Executable path used to start the service and detect its running process")
							placeHolderText: serviceEditorContainer.pathBrowsePlaceHolder(false)
							pathKind: Enums.pathKindFile
							// Windows executables + common binary suffixes; "*" = extensionless
							// Linux/Unix binaries (no suffix). Other file types cannot be picked.
							extensions: ["exe", "com", "bin", "run", "appimage", "out", "*"]
							// Server: need clientId (remote agent FS). Agent app: local FS always.
							browseEnabled: serviceEditorContainer.pathBrowseEnabled
							// Path can only come from the browse dialog - manual typing is
							// error-prone (wrong separators, no existence check).
							readOnlyPath: true
							textInputValidator: pathRequiredRegexp;
							showErrorWhenInvalid: true;
							errorText: qsTr("Please enter the path");

							function getHeaders(){
								return serviceEditorContainer.getHeaders()
							}
							
							onPathEdited: {
								if (serviceEditorContainer.serviceData){
									serviceEditorContainer.serviceData.m_path = pathInput.path
								}
								serviceEditorContainer.doUpdateModel();
								serviceEditorContainer.handlePluginPathChange(pathInput.path)
							}
							
							KeyNavigation.tab: argumentsInput;
							
							RegularExpressionValidator {
								id: pathRequiredRegexp;
								regularExpression: /^(?!\s*$).+/;
							}
						}
						
						TextInputElementView {
							id: argumentsInput
							controlWidth: serviceEditorContainer.getEditorControlWidth(argumentsInput)
							name: qsTr("Arguments")
							description: qsTr("Command-line arguments passed to the service executable")
							placeHolderText: qsTr("Enter the arguments");
							
							onEditingFinished: {
								serviceEditorContainer.doUpdateModel();
							}
							
							KeyNavigation.tab: nameInput;
						}
					}
				}
			}
		}
	}
	
	Component {
		id: serverPageComp

		Item {
			id: serverPageRoot
			anchors.fill: parent

			function updateGui(){
				if (!serviceEditorContainer.serviceData){
					return
				}

				if (!serviceEditorContainer.pluginLoaded) {
					return
				}
				if (serviceEditorContainer.serviceData && serviceEditorContainer.serviceData.m_inputConnections){
					inputListView.model = 0
					inputListView.model = serviceEditorContainer.serviceData.m_inputConnections

					for (let i = 0; i < inputListView.count; i++){
						let item = inputListView.itemAtIndex(i)
						if (item){
							item.updateGui()
						}
					}
				}
			}

			function updateModel(){
				if (!serviceEditorContainer.serviceData){
					return
				}

				if (!serviceEditorContainer.pluginLoaded) {
					return
				}
				if (serviceEditorContainer.serviceData && serviceEditorContainer.serviceData.m_inputConnections){
					for (let i = 0; i < inputListView.count; i++){
						let item = inputListView.itemAtIndex(i)
						if (item){
							item.updateModel()
						}
					}
				}
			}

			// Placeholder when plugin not loaded
			Column {
				anchors.centerIn: parent
				spacing: Style.marginL
				visible: !serviceEditorContainer.pluginLoaded
				width: Math.max(0, Math.min(400, parent.width - 2 * Style.marginXL))

				BusyIndicator {
					anchors.horizontalCenter: parent.horizontalCenter
					width: 40
					height: 40
					visible: serviceEditorContainer.pluginLoading
				}

				Image {
					anchors.horizontalCenter: parent.horizontalCenter
					width: 48
					height: 48
					sourceSize.width: 48
					sourceSize.height: 48
					source: "../../../../" + Style.getIconPath("Icons/Link", Icon.State.Off, Icon.Mode.Normal)
					opacity: 0.5
					visible: !serviceEditorContainer.pluginLoading
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.pluginLoading ? qsTr("Loading plugin data...") :
					      (serviceEditorContainer.pluginLoadFailed ? qsTr("Failed to load plugin") : qsTr("Plugin data not loaded"))
					font.pixelSize: Style.fontSizeXL
					horizontalAlignment: Text.AlignHCenter
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.pluginServicePath ? qsTr("Path: ") + serviceEditorContainer.pluginServicePath : ""
					horizontalAlignment: Text.AlignHCenter
					opacity: 0.6
					elide: Text.ElideMiddle
					font.pixelSize: Style.fontSizeS
					visible: !serviceEditorContainer.pluginLoading && serviceEditorContainer.pluginServicePath !== ""
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.isNewService
						? qsTr("Save the service to automatically load plugin connection settings")
						: (serviceEditorContainer.pluginLoadFailed
							? qsTr("Check the path and try reloading")
							: qsTr("Will be loaded automatically after saving"))
					horizontalAlignment: Text.AlignHCenter
					opacity: 0.7
					wrapMode: Text.WordWrap
					visible: !serviceEditorContainer.pluginLoading
				}

				BaseText {
					text: qsTr("Reload")
					font.pixelSize: Style.fontSizeM
							font.underline: serverEmptyReloadMouseArea.containsMouse
					color: Style.linkColor
					width: parent.width
					horizontalAlignment: Text.AlignHCenter
					visible: !serviceEditorContainer.isNewService && !serviceEditorContainer.pluginLoading
					MouseArea {
								id: serverEmptyReloadMouseArea
						anchors.fill: parent
								hoverEnabled: true
						cursorShape: Qt.PointingHandCursor
						onClicked: serviceEditorContainer.requestLoadPlugin()
					}
				}
			}

			Flickable {
				id: serverFlickable
				anchors.fill: parent
				anchors.leftMargin: Style.marginXL
				anchors.topMargin: Style.marginXL
				anchors.rightMargin: Style.marginXL
				anchors.bottomMargin: Style.marginXL

				visible: serviceEditorContainer.pluginLoaded

				contentWidth: serverBody.width
				contentHeight: serverBody.height + Style.marginXL

				boundsBehavior: Flickable.StopAtBounds
				clip: true

				Column {
					id: serverBody
					x: Math.max(0, (serverFlickable.width - width) / 2)
					width: Math.max(0, Math.min(900, serverFlickable.width - 2 * Style.marginXL))
					spacing: Style.marginXL

					Row {
						width: parent.width
						spacing: Style.marginS
						visible: serviceEditorContainer.pluginLoaded

						BaseText {
							text: "\u2713"
							color: Style.imaginToolsAccentColor
							font.bold: true
							font.pixelSize: Style.fontSizeM
							anchors.verticalCenter: parent.verticalCenter
						}

						BaseText {
							text: qsTr("Plugin loaded")
							font.pixelSize: Style.fontSizeS
							opacity: 0.6
							anchors.verticalCenter: parent.verticalCenter
						}

						BaseText {
							text: qsTr("Reload")
							font.pixelSize: Style.fontSizeS
							font.underline: serverLoadedReloadMouseArea.containsMouse
							color: Style.linkColor
							anchors.verticalCenter: parent.verticalCenter
							MouseArea {
								id: serverLoadedReloadMouseArea
								anchors.fill: parent
								hoverEnabled: true
								cursorShape: Qt.PointingHandCursor
								onClicked: serviceEditorContainer.requestLoadPlugin()
							}
						}
					}

					BaseText {
						visible: !inputListView || inputListView.count === 0
						text: qsTr("No connections")
						font.pixelSize: Style.fontSizeXL
						horizontalAlignment: Text.AlignHCenter
						width: parent.width
					}

					ListView {
						id: inputListView
						width: parent.width
						height: contentHeight
						cacheBuffer: 1000
						boundsBehavior: Flickable.StopAtBounds
						spacing: Style.marginXL
						delegate: Column {
							width: parent.width
							spacing: Style.marginL

							GroupHeaderView {
								title: model.item.m_connectionName + qsTr(" (input)")
								width: parent.width
								visible: inputListView.count > 0
							}

							ServerConnectionParamElementView {
								id: inputConnectionEditor
								width: inputListView.width
								readOnly: serviceEditorContainer.readOnly
								controlWidth: serviceEditorContainer.getEditorControlWidth(inputConnectionEditor.hostInput)

								onParamsChanged: {
									serviceEditorContainer.doUpdateModel()
								}

								ButtonElementView {
									id: externConnectionsView
									controlWidth: serviceEditorContainer.getEditorControlWidth(externConnectionsView)
									name: qsTr("Extern Connections")
									description: qsTr("Configures externally accessible endpoints for this input connection")
									text: qsTr("Edit")
									onClicked: {
										ModalDialogManager.openDialog(externPortsDialogComp, {inputConnection: model.item})
									}
								}
							}

							function updateGui(){
								if (!model.item){
									return
								}
								// Setting these properties one by one below fires onXxxChanged ->
								// paramsChanged() -> onParamsChanged -> serviceEditorContainer.doUpdateModel()
								// synchronously for *each* property. Without this guard, that
								// mid-refresh doUpdateModel() call writes the not-yet-updated fields
								// (still holding their previous/default values) straight back into
								// model.item.m_connectionParam, clobbering the very port values this
								// same updateGui() is about to read for the next property.
								serviceEditorContainer.setBlockingUpdateModel(true)
								if (model.item.m_connectionParam){
									inputConnectionEditor.host = model.item.m_connectionParam.m_host
									inputConnectionEditor.httpPort = model.item.m_connectionParam.m_httpPort
									inputConnectionEditor.wsPort = model.item.m_connectionParam.m_wsPort
									inputConnectionEditor.isSecure = model.item.m_connectionParam.m_isSecure
								}
								serviceEditorContainer.setBlockingUpdateModel(false)
								if (model.item.m_externConnectionList){
									let values = []
									for (let i = 0; i < model.item.m_externConnectionList.count; i++){
										let externConnection = model.item.m_externConnectionList.get(i).item
										if (externConnection){
											values.push((externConnection.m_connectionParam.m_isSecure == true ? "https://" : "http://") + externConnection.m_connectionParam.m_host + ":" + externConnection.m_connectionParam.m_httpPort + externConnection.m_connectionParam.m_httpPath)
										}
									}
									if (values.length > 0){
										externConnectionsView.description = values.join('\n')
									}
									else{
										externConnectionsView.description = ""
									}
								}
							}

							function updateModel(){
								if (!model.item){
									return
								}
								if (model.item.m_connectionParam){
									model.item.m_connectionParam.m_host = inputConnectionEditor.host
									model.item.m_connectionParam.m_httpPort = inputConnectionEditor.httpPort
									model.item.m_connectionParam.m_wsPort = inputConnectionEditor.wsPort
									model.item.m_connectionParam.m_isSecure = inputConnectionEditor.isSecure
								}
							}
						}
					}
				}
			}

			Component {
				id: externPortsDialogComp

				ExternPortsDialog {
					property var inputConnection: null

					width: serviceEditorContainer.width - Style.marginS < 1200 ? serviceEditorContainer.width - Style.marginS : 1200

					onStarted: {
						if (inputConnection){
							if (inputConnection.m_externConnectionList){
								connectionListModel = inputConnection.m_externConnectionList.copyMe()
							}
						}
					}

					onFinished: {
						if (buttonId == Enums.save){
							if (inputConnection){
								inputConnection.m_externConnectionList = connectionListModel.copyMe()
								serviceEditorContainer.doUpdateModel()
								serviceEditorContainer.doUpdateGui()
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: outputConnectionsPageComp

		Item {
			id: outputPageRoot
			anchors.fill: parent

			// The pick-list is a fleet-wide scan, so it is fetched when this page is
			// actually opened — never packed into every read of the service.
			Component.onCompleted: {
				serviceEditorContainer.requestAvailableConnectionsForOutputs()
			}

			Connections {
				target: serviceEditorContainer
				// Plugin (re)loaded: the set of output slots may have changed.
				function onPluginLoadedChanged(){
					if (serviceEditorContainer.pluginLoaded){
						serviceEditorContainer.requestAvailableConnectionsForOutputs()
					}
				}
				// Candidates arrived: rebind delegates. Must not re-fetch (would loop).
				function onAvailableConnectionsUpdated(){
					outputPageRoot.updateGui()
				}
			}

			function updateGui(){
				if (!serviceEditorContainer.serviceData){
					return
				}

				if (!serviceEditorContainer.pluginLoaded) {
					return
				}
				if (serviceEditorContainer.serviceData && serviceEditorContainer.serviceData.m_outputConnections){
					// Keep the same ListView model instance when possible so delegates stay alive
					// and can re-bind Available Connections after undo/redo (createFromJson).
					if (outputListView.model !== serviceEditorContainer.serviceData.m_outputConnections){
						outputListView.model = serviceEditorContainer.serviceData.m_outputConnections
					}

					for (let i = 0; i < outputListView.count; i++){
						let item = outputListView.itemAtIndex(i)
						if (item){
							item.updateGui()
						}
					}
				}
			}

			function updateModel(){
				if (!serviceEditorContainer.pluginLoaded) {
					return
				}
				if (serviceEditorContainer.serviceData && serviceEditorContainer.serviceData.m_outputConnections){
					for (let i = 0; i < outputListView.count; i++){
						let item = outputListView.itemAtIndex(i)
						if (item){
							item.updateModel()
						}
					}
				}
			}

			// Placeholder when plugin not loaded
			Column {
				anchors.centerIn: parent
				spacing: Style.marginL
				visible: !serviceEditorContainer.pluginLoaded
				width: Math.max(0, Math.min(400, parent.width - 2 * Style.marginXL))

				BusyIndicator {
					anchors.horizontalCenter: parent.horizontalCenter
					width: 40
					height: 40
					visible: serviceEditorContainer.pluginLoading
				}

				Image {
					anchors.horizontalCenter: parent.horizontalCenter
					width: 48
					height: 48
					sourceSize.width: 48
					sourceSize.height: 48
					source: "../../../../" + Style.getIconPath("Icons/Link", Icon.State.Off, Icon.Mode.Normal)
					opacity: 0.5
					visible: !serviceEditorContainer.pluginLoading
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.pluginLoading ? qsTr("Loading plugin data...") :
					      (serviceEditorContainer.pluginLoadFailed ? qsTr("Failed to load plugin") : qsTr("Plugin data not loaded"))
					font.pixelSize: Style.fontSizeXL
					horizontalAlignment: Text.AlignHCenter
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.pluginServicePath ? qsTr("Path: ") + serviceEditorContainer.pluginServicePath : ""
					horizontalAlignment: Text.AlignHCenter
					opacity: 0.6
					elide: Text.ElideMiddle
					font.pixelSize: Style.fontSizeS
					visible: !serviceEditorContainer.pluginLoading && serviceEditorContainer.pluginServicePath !== ""
				}

				BaseText {
					width: parent.width
					text: serviceEditorContainer.isNewService
						? qsTr("Save the service to automatically load plugin connection settings")
						: (serviceEditorContainer.pluginLoadFailed
							? qsTr("Check the path and try reloading")
							: qsTr("Will be loaded automatically after saving"))
					horizontalAlignment: Text.AlignHCenter
					opacity: 0.7
					wrapMode: Text.WordWrap
					visible: !serviceEditorContainer.pluginLoading
				}

				BaseText {
					text: qsTr("Reload")
					font.pixelSize: Style.fontSizeM
							font.underline: outputEmptyReloadMouseArea.containsMouse
					color: Style.linkColor
					width: parent.width
					horizontalAlignment: Text.AlignHCenter
					visible: !serviceEditorContainer.isNewService && !serviceEditorContainer.pluginLoading
					MouseArea {
								id: outputEmptyReloadMouseArea
						anchors.fill: parent
								hoverEnabled: true
						cursorShape: Qt.PointingHandCursor
						onClicked: serviceEditorContainer.requestLoadPlugin()
					}
				}
			}

			Flickable {
				id: outputFlickable
				anchors.fill: parent
				anchors.leftMargin: Style.marginXL
				anchors.topMargin: Style.marginXL
				anchors.rightMargin: Style.marginXL
				anchors.bottomMargin: Style.marginXL

				visible: serviceEditorContainer.pluginLoaded

				contentWidth: outputBody.width
				contentHeight: outputBody.height + Style.marginXL

				boundsBehavior: Flickable.StopAtBounds
				clip: true

				Column {
					id: outputBody
					x: Math.max(0, (outputFlickable.width - width) / 2)
					width: Math.max(0, Math.min(900, outputFlickable.width - 2 * Style.marginXL))
					spacing: Style.marginXL

					Row {
						width: parent.width
						spacing: Style.marginS
						visible: serviceEditorContainer.pluginLoaded

						BaseText {
							text: "\u2713"
							color: Style.imaginToolsAccentColor
							font.bold: true
							font.pixelSize: Style.fontSizeM
							anchors.verticalCenter: parent.verticalCenter
						}

						BaseText {
							text: qsTr("Plugin loaded")
							font.pixelSize: Style.fontSizeS
							opacity: 0.6
							anchors.verticalCenter: parent.verticalCenter
						}

						BaseText {
							text: qsTr("Reload")
							font.pixelSize: Style.fontSizeS
							font.underline: outputLoadedReloadMouseArea.containsMouse
							color: Style.linkColor
							anchors.verticalCenter: parent.verticalCenter
							MouseArea {
								id: outputLoadedReloadMouseArea
								anchors.fill: parent
								hoverEnabled: true
								cursorShape: Qt.PointingHandCursor
								onClicked: serviceEditorContainer.requestLoadPlugin()
							}
						}
					}

					BaseText {
						visible: serviceEditorContainer.pluginLoaded && outputListView.count === 0
						text: qsTr("No output connections")
						font.pixelSize: Style.fontSizeXL
						horizontalAlignment: Text.AlignHCenter
						width: parent.width
					}

					GroupHeaderView {
						title: qsTr("Output Connections")
						width: parent.width
						visible: outputListView.count > 0
					}

					TreeItemModel {
						id: outputHeadersModel
						Component.onCompleted: {
							let index = outputHeadersModel.insertNewItem()
							outputHeadersModel.setData("id", "host", index)
							outputHeadersModel.setData("name", qsTr("Host"), index)

							index = outputHeadersModel.insertNewItem()
							outputHeadersModel.setData("id", "httpPort", index)
							outputHeadersModel.setData("name", qsTr("HTTP Port"), index)

							index = outputHeadersModel.insertNewItem()
							outputHeadersModel.setData("id", "wsPort", index)
							outputHeadersModel.setData("name", qsTr("Web Socket Port"), index)
						}
					}

					ListView {
						id: outputListView
						width: parent.width
						height: contentHeight
						cacheBuffer: 1000
						boundsBehavior: Flickable.StopAtBounds
						spacing: Style.marginXL
						delegate: GroupElementView {
							id: outputDelegate
							width: outputListView.width

							property var connectionParam: model && model.item ? model.item.m_connectionParam : null
							property var modelData: model ? model.item : null

							// Candidates for this slot, fetched lazily by the editor. Re-evaluated
							// whenever availableConnectionsByUsage is reassigned after a fetch.
							property var availableConnections: serviceEditorContainer.getAvailableConnectionsFor(
										model && model.item ? model.item.m_id : "")

							// Reactive resync: undo/redo mutates model.item in place (createFromJson)
							// without necessarily routing through this row's updateGui() — this binding
							// re-checks the correct row whenever the stored link itself changes, so the
							// checkbox never depends on an imperative refresh reaching this delegate.
							property string currentDependantConnectionId: (model && model.item
										&& model.item.m_dependantConnectionId !== undefined
										&& model.item.m_dependantConnectionId !== null)
										? "" + model.item.m_dependantConnectionId : ""
							onCurrentDependantConnectionIdChanged: outputDelegate.syncAvailableConnectionsCheck()

							// Picking a connection applies immediately (SetOutputConnection), independent
							// of doUpdateModel()/the rest of the service record.
							property bool applying: false
							property string applyError: ""
							// The id this row asked to apply — echoed back onto model.item once the
							// agent confirms, since the response itself only carries connectionParam.
							property string pendingDependantConnectionId: ""

							// When ListView creates/recreates the row (model reassigned after undo/redo),
							// sync Available Connections from the current model without deferred calls.
							Component.onCompleted: {
								outputDelegate.updateGui()
							}

							Connections {
								target: serviceEditorContainer
								function onOutputConnectionApplied(connectionId, successful, connectionParam){
									if (!model.item || ("" + model.item.m_id) !== connectionId){
										return
									}

									outputDelegate.applying = false

									// This mutates serviceData to reflect a server-confirmed fact, not a
									// pending user edit — SetOutputConnection already applied it remotely,
									// so it must not re-arm Save/Undo (which would offer to "save" something
									// already saved, and let Undo revert local display out of sync with the
									// agent). Same guard onPluginLoaded uses for the same reason.
									let documentManager = serviceEditorContainer.documentManager
									if (documentManager){
										documentManager.setBlockUndoManager(serviceEditorContainer.documentId, true)
									}

									if (successful){
										outputDelegate.applyError = ""
										model.item.m_dependantConnectionId = outputDelegate.pendingDependantConnectionId
										if (model.item.m_connectionParam && connectionParam){
											model.item.m_connectionParam.m_host = connectionParam.m_host
											model.item.m_connectionParam.m_httpPort = connectionParam.m_httpPort
											model.item.m_connectionParam.m_wsPort = connectionParam.m_wsPort
										}
									}
									else{
										outputDelegate.applyError = qsTr("Failed to apply connection")
									}

									// Reflects the (possibly unchanged, on failure) confirmed state.
									outputDelegate.syncAvailableConnectionsCheck()

									if (documentManager){
										documentManager.setBlockUndoManager(serviceEditorContainer.documentId, false)
									}
								}
							}

							TextInputElementView {
								id: serviceTypeInput
								controlWidth: serviceEditorContainer.getEditorControlWidth(serviceTypeInput)
								name: qsTr("Service Type")
								description: qsTr("Required service type provided by the selected connection")
								readOnly: true
								text: model.item.m_serviceTypeId
							}

							TextInputElementView {
								id: connectionNameInput
								controlWidth: serviceEditorContainer.getEditorControlWidth(connectionNameInput)
								name: qsTr("Connection Name")
								description: qsTr("Identifies this output connection in the service configuration")
								readOnly: true
								text: model.item.m_connectionName
							}

							TableElementView {
								id: outTable
								controlWidth: serviceEditorContainer.getEditorControlWidth(outTable)
								name: qsTr("Available Connections")
								// Tell an empty list apart from a failed match: a service type nobody
								// has added yet used to look exactly like "still loading". A candidate
								// does NOT need to be running (Start/Stop status is irrelevant here) —
								// it only needs to have been added on some agent.
								description: outputDelegate.getAvailableConnectionsDescription()
								onTableChanged: {
									if (!model.item){
										return
									}
									if (table){
										table.isMultiCheckable = false
										table.checkable = true
										table.headers = outputHeadersModel

										table.setColumnContentById("host", outHostCell)
										table.setColumnContentById("httpPort", outHttpCell)
										table.setColumnContentById("wsPort", outWsCell)

										// Route through updateGui() (not the two calls directly) so the
										// blocking-guard covers table.elements= too — see updateGui() below.
										outputDelegate.updateGui()
									}
								}

								Connections {
									target: outTable.table
									function onCheckedItemsChanged(){
										// While restoring checks from model (or from a just-confirmed
										// response), do not re-fire from the table's own check/uncheck.
										if (serviceEditorContainer.internal__ && serviceEditorContainer.internal__.blockingUpdateModel){
											return
										}
										if (!model.item){
											return
										}

										let indexes = outTable.table.getCheckedItems()
										let dependantConnectionId = ""
										if (indexes.length > 0){
											let connectionInfo = outputDelegate.getCandidate(indexes[0])
											if (connectionInfo && connectionInfo.m_id !== undefined && connectionInfo.m_id !== null){
												dependantConnectionId = "" + connectionInfo.m_id
											}
										}

										outputDelegate.pendingDependantConnectionId = dependantConnectionId
										outputDelegate.applying = true
										outputDelegate.applyError = ""
										serviceEditorContainer.setOutputConnection("" + model.item.m_id, dependantConnectionId)
									}
								}

								Component {
									id: outHostCell
									TableCellTextDelegate{
										function getValue(){
											let connectionItem = outputDelegate.getCandidateParam(rowIndex)
											return connectionItem ? connectionItem.m_host : ""
										}
									}
								}

								Component {
									id: outHttpCell
									TableCellTextDelegate{
										function getValue(){
											let connectionItem = outputDelegate.getCandidateParam(rowIndex)
											return connectionItem ? connectionItem.m_httpPort : ""
										}
									}
								}

								Component {
									id: outWsCell
									TableCellTextDelegate{
										function getValue(){
											let connectionItem = outputDelegate.getCandidateParam(rowIndex)
											return connectionItem ? connectionItem.m_wsPort : ""
										}
									}
								}
							}

							// One candidate row, or null when the list is not loaded / index is stale.
							function getCandidate(rowIndex){
								let candidates = outputDelegate.availableConnections
								if (!candidates || rowIndex < 0 || rowIndex >= candidates.count){
									return null
								}
								let entry = candidates.get(rowIndex)
								return entry ? entry.item : null
							}

							function getCandidateParam(rowIndex){
								let candidate = outputDelegate.getCandidate(rowIndex)
								return candidate ? candidate.m_connectionParam : null
							}

							function getAvailableConnectionsDescription(){
								if (outputDelegate.applying){
									return qsTr("Applying...")
								}
								if (outputDelegate.applyError !== ""){
									return outputDelegate.applyError
								}
								if (serviceEditorContainer.availableConnectionsLoading){
									return qsTr("Looking for services providing this connection...")
								}
								if (outTable.table && outTable.table.elementsCount > 0){
									return qsTr("Select one of the available connections")
								}
								let connectionName = outputDelegate.modelData ? outputDelegate.modelData.m_connectionName : ""
								let serviceTypeId = outputDelegate.modelData ? outputDelegate.modelData.m_serviceTypeId : ""
								return qsTr("No service provides the connection '%1' yet. Add a service of type '%2' on any agent — it does not need to be running to appear here.")
									.arg(connectionName)
									.arg(serviceTypeId)
							}

							// Table.elements must point at the freshly fetched candidate list;
							// TableBase.onElementsChanged clears checkmarks, so the selection is
							// restored afterwards from the stored m_dependantConnectionId.
							// Guarded (save/restore, not unconditional true/false) because
							// TableBase.onElementsChanged() fires checkedItemsChanged() synchronously,
							// which would otherwise mark the document dirty as soon as the editor opens
							// or the table (re)appears, before the user has touched anything.
							function bindAvailableConnectionsTable(){
								if (!outTable.table){
									return
								}
								let wasBlocking = serviceEditorContainer.internal__
											? serviceEditorContainer.internal__.blockingUpdateModel : false
								serviceEditorContainer.setBlockingUpdateModel(true)
								outTable.table.elements = outputDelegate.availableConnections
								serviceEditorContainer.setBlockingUpdateModel(wasBlocking)
							}

							function syncAvailableConnectionsCheck(){
								if (!outTable.table || !model.item){
									return
								}

								// Save/restore rather than unconditional true/false: this can run
								// nested inside an outer guarded refresh (e.g. doUpdateGui()'s own
								// blockingUpdateModel=true) — clearing to false unconditionally would
								// re-open that outer guard before it is done.
								let wasBlocking = serviceEditorContainer.internal__
											? serviceEditorContainer.internal__.blockingUpdateModel : false
								serviceEditorContainer.setBlockingUpdateModel(true)

								let dependantId = ""
								if (model.item.m_dependantConnectionId !== undefined && model.item.m_dependantConnectionId !== null){
									dependantId = "" + model.item.m_dependantConnectionId
								}

								outTable.table.uncheckAll()

								let candidates = outputDelegate.availableConnections
								if (dependantId !== "" && candidates){
									for (let i = 0; i < candidates.count; i++){
										let connectionInfo = outputDelegate.getCandidate(i)
										if (!connectionInfo){
											continue
										}
										let rowId = "" + connectionInfo.m_id
										if (rowId === dependantId){
											outTable.table.checkItem(i)
											break
										}
									}
								}

								serviceEditorContainer.setBlockingUpdateModel(wasBlocking)
							}

							function updateGui(){
								bindAvailableConnectionsTable()
								syncAvailableConnectionsCheck()
							}

							// The producer pick applies immediately via SetOutputConnection (see
							// onCheckedItemsChanged above) - nothing to contribute to the bundled
							// doUpdateModel()/UpdateService save.
							function updateModel(){
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: optionsPageComp
			
			Flickable {
				id: optionsFlickable
				
				anchors.fill: parent
				anchors.leftMargin: Style.marginXL
				anchors.topMargin: Style.marginXL
				anchors.rightMargin: Style.marginXL
				anchors.bottomMargin: Style.marginXL
				
				contentWidth: optionsColumn.width
				contentHeight: optionsColumn.height + Style.marginXL
				
				boundsBehavior: Flickable.StopAtBounds
				clip: true
				
				function updateGui(){
					if (!serviceEditorContainer.serviceData){
						return
					}

					switchAutoStart.checked = serviceEditorContainer.serviceData.m_isAutoStart
					
					if (serviceEditorContainer.serviceData.m_tracingLevel > -1){
						switchVerboseMessage.checked = true
					}
					else{
						switchVerboseMessage.checked = false
					}
					
					tracingLevelInput.currentIndex = serviceEditorContainer.serviceData.m_tracingLevel
					
					if (serviceEditorContainer.serviceData.m_startScript !== ""){
						startScriptChecked.checked = true
						startScriptInput.path = serviceEditorContainer.serviceData.m_startScript
					}
					else{
						startScriptChecked.checked = false
						startScriptInput.path = ""
					}

					if (serviceEditorContainer.serviceData.m_stopScript !== ""){
						stopScriptChecked.checked = true
						stopScriptInput.path = serviceEditorContainer.serviceData.m_stopScript
					}
					else{
						stopScriptChecked.checked = false
					}
				}
				
				function updateModel(){
					if (!serviceEditorContainer.serviceData){
						return
					}

					serviceEditorContainer.serviceData.m_isAutoStart = switchAutoStart.checked
					
					if (switchVerboseMessage.checked){
						if (tracingLevelInput.currentIndex == -1){
							tracingLevelInput.currentIndex = 0
						}
						serviceEditorContainer.serviceData.m_tracingLevel = tracingLevelInput.currentIndex
					}
					else{
						serviceEditorContainer.serviceData.m_tracingLevel = -1
					}
					
					if (startScriptChecked.checked){
						serviceEditorContainer.serviceData.m_startScript = startScriptInput.path
					}
					else{
						serviceEditorContainer.serviceData.m_startScript = ""
					}

					if (stopScriptChecked.checked){
						serviceEditorContainer.serviceData.m_stopScript = stopScriptInput.path
					}
					else{
						serviceEditorContainer.serviceData.m_stopScript = ""
					}
				}
				
				Column {
					id: optionsColumn
					x: Math.max(0, (optionsFlickable.width - width) / 2)
					width: Math.max(0, Math.min(900, optionsFlickable.width - 2 * Style.marginXL))
					spacing: Style.marginXL
					
					Column {
						width: parent.width
						spacing: Style.marginL
						
						GroupHeaderView {
							title: qsTr("Options")
							width: parent.width
							groupView: additionalGroup
						}
						
						GroupElementView {
							id: additionalGroup
							width: parent.width
							
							SwitchElementView {
								id: switchAutoStart
								controlWidth: serviceEditorContainer.getEditorControlWidth(switchAutoStart)
								name: qsTr("Autostart (") + (switchAutoStart.checked ? qsTr("on") : qsTr("off")) + ")"
								description: qsTr("Starts the service automatically when the Agent starts")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: switchVerboseMessage
								controlWidth: serviceEditorContainer.getEditorControlWidth(switchVerboseMessage)
								name: qsTr("Verbose Message")
								description: qsTr("Enables service tracing messages at the selected level")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							ComboBoxElementView {
								id: tracingLevelInput
								controlWidth: serviceEditorContainer.getEditorControlWidth(tracingLevelInput)
								name: qsTr("Tracing level")
								description: qsTr("Controls the amount of diagnostic information produced by the service")
								visible: switchVerboseMessage.checked
								model: TreeItemModel {
								}
								Component.onCompleted: {
									let index = model.insertNewItem()
									model.setData("id", "0", index)
									model.setData("name", "0", index)
									
									index = model.insertNewItem()
									model.setData("id", "1", index)
									model.setData("name", "1", index)
									
									index = model.insertNewItem()
									model.setData("id", "2", index)
									model.setData("name", "2", index)
									
									index = model.insertNewItem()
									model.setData("id", "3", index)
									model.setData("name", "3", index)
									
									index = model.insertNewItem()
									model.setData("id", "4", index)
									model.setData("name", "4", index)
									
									index = model.insertNewItem()
									model.setData("id", "5", index)
									model.setData("name", "5", index)
								}
								onCurrentIndexChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: startScriptChecked
								controlWidth: serviceEditorContainer.getEditorControlWidth(startScriptChecked)
								name: qsTr("Start script")
								description: qsTr("Uses a custom script when starting the service")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							ServerPathPickerElementView {
								id: startScriptInput
								controlWidth: serviceEditorContainer.getEditorControlWidth(startScriptInput)
								visible: startScriptChecked.checked
								placeHolderText: serviceEditorContainer.pathBrowsePlaceHolder(true)
								name: qsTr("Start Script Path")
								description: qsTr("Path to the script executed to start the service")
								pathKind: Enums.pathKindFile
								// Script formats only (Windows shell / PowerShell / Unix shells / common interpreters).
								extensions: ["sh", "bash", "bat", "cmd", "ps1", "py", "pl", "rb", "vbs"]
								browseEnabled: serviceEditorContainer.pathBrowseEnabled
								readOnlyPath: true

								function getHeaders(){
									return serviceEditorContainer.getHeaders()
								}

								onPathEdited: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							SwitchElementView {
								id: stopScriptChecked
								controlWidth: serviceEditorContainer.getEditorControlWidth(stopScriptChecked)
								name: qsTr("Stop script")
								description: qsTr("Uses a custom script when stopping the service")
								onCheckedChanged: {
									serviceEditorContainer.doUpdateModel()
								}
							}
							
							ServerPathPickerElementView {
								id: stopScriptInput
								controlWidth: serviceEditorContainer.getEditorControlWidth(stopScriptInput)
								visible: stopScriptChecked.checked
								placeHolderText: serviceEditorContainer.pathBrowsePlaceHolder(true)
								name: qsTr("Stop Script Path")
								description: qsTr("Path to the script executed to stop the service")
								pathKind: Enums.pathKindFile
								// Same script whitelist as Start Script Path.
								extensions: ["sh", "bash", "bat", "cmd", "ps1", "py", "pl", "rb", "vbs"]
								browseEnabled: serviceEditorContainer.pathBrowseEnabled
								readOnlyPath: true

								function getHeaders(){
									return serviceEditorContainer.getHeaders()
								}

								onPathEdited: {
									serviceEditorContainer.doUpdateModel()
								}
							}
						}
					}
				}
			}
	}
	Component {
		id: settingsPageComp
		
		Item {
			id: settingsViewItem
			anchors.fill: parent
			
			property bool canEditSettings: !serviceEditorContainer.isNewService && serviceEditorContainer.settingsPath !== "" && !serviceEditorContainer.serviceIsDirty
			
			function updateGui(){
				settingsPathInput.text = serviceEditorContainer.settingsPath
				settingsTextEdit.text = serviceEditorContainer.settingsContent
			}
			
			function updateModel(){
				if (serviceEditorContainer.serviceData){
					serviceEditorContainer.settingsPath = settingsPathInput.text
					serviceEditorContainer.serviceData.m_settingsPath = settingsPathInput.text
				}
			}
			
			function setContent(content){
				settingsTextEdit.text = content
			}
			
			function getContent(){
				return settingsTextEdit.text
			}
			
			Component.onCompleted: {
				updateGui()
			}
			
			Column {
				id: settingsHeaderColumn
				anchors.left: parent.left
				anchors.leftMargin: Style.marginXL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginXL
				anchors.top: parent.top
				anchors.topMargin: Style.marginXL
				spacing: Style.marginM
				
				GroupHeaderView {
					title: qsTr("Settings")
					width: parent.width
					groupView: settingsGroup
				}
				
				GroupElementView {
					id: settingsGroup
					width: parent.width
					
					TextInputElementView {
						id: settingsPathInput
						controlWidth: serviceEditorContainer.getEditorControlWidth(settingsPathInput)
						name: qsTr("Settings Path")
						description: qsTr("Path to the service settings file loaded and saved by this editor")
						placeHolderText: qsTr("Enter the path to settings file")
						text: serviceEditorContainer.settingsPath
						
						onEditingFinished: {
							serviceEditorContainer.settingsPath = settingsPathInput.text
							serviceEditorContainer.doUpdateModel()
						}
					}
				}
				
				BaseText {
					width: parent.width
					wrapMode: Text.WordWrap
					visible: serviceEditorContainer.isNewService
					text: qsTr("Save the service first to configure settings")
					color: Style.warningColor
				}
				
				BaseText {
					width: parent.width
					wrapMode: Text.WordWrap
					visible: !serviceEditorContainer.isNewService && serviceEditorContainer.serviceIsDirty
					text: qsTr("Service has unsaved changes. Save the service before loading or editing settings")
					color: Style.warningColor
				}
				
				BaseText {
					width: parent.width
					wrapMode: Text.WordWrap
					visible: !serviceEditorContainer.isNewService && !serviceEditorContainer.serviceIsDirty && serviceEditorContainer.settingsPath === ""
					text: qsTr("Settings path is not configured. Enter a path and save the service to enable settings")
					opacity: 0.7
				}
				
				BaseText {
					width: parent.width
					wrapMode: Text.WordWrap
					visible: settingsViewItem.canEditSettings
					text: serviceEditorContainer.settingsFileExists
						? qsTr("Settings file: ") + serviceEditorContainer.settingsPath
						: qsTr("Settings file does not exist yet. It will be created on first save")
				}
				
				Row {
					spacing: Style.marginM
					visible: !serviceEditorContainer.isNewService && serviceEditorContainer.settingsPath !== ""
					
					Button {
						id: loadSettingsButton
						width: Style.sizeHintBXS
						height: Style.controlHeightM
						text: qsTr("Load")
						enabled: settingsViewItem.canEditSettings
						onClicked: {
							serviceEditorContainer.loadSettings()
						}
					}
					
					Button {
						id: saveSettingsButton
						width: Style.sizeHintBXS
						height: Style.controlHeightM
						text: qsTr("Save")
						enabled: settingsViewItem.canEditSettings && !serviceEditorContainer.readOnly
						onClicked: {
							serviceEditorContainer.saveSettings(settingsViewItem.getContent())
						}
					}
				}
			}
			
			Rectangle {
				anchors.left: parent.left
				anchors.leftMargin: Style.marginXL
				anchors.right: parent.right
				anchors.rightMargin: Style.marginXL
				anchors.top: settingsHeaderColumn.bottom
				anchors.topMargin: Style.marginM
				anchors.bottom: parent.bottom
				anchors.bottomMargin: Style.marginXL
				
				visible: settingsViewItem.canEditSettings
				
				color: Style.backgroundColor2
				border.color: Style.borderColor
				border.width: 1
				clip: true
				
				Flickable {
					id: settingsFlickable
					anchors.fill: parent
					anchors.margins: Style.marginS
					contentWidth: width
					contentHeight: settingsTextEdit.contentHeight
					boundsBehavior: Flickable.StopAtBounds
					clip: true
					
					TextEdit {
						id: settingsTextEdit
						width: settingsFlickable.width
						readOnly: serviceEditorContainer.readOnly
						selectByMouse: true
						wrapMode: TextEdit.NoWrap
						font.family: "Courier New"
						font.pixelSize: Style.fontSizeM
						color: Style.textColor
						
						onCursorRectangleChanged: {
							if (cursorRectangle.y + cursorRectangle.height > settingsFlickable.contentY + settingsFlickable.height){
								settingsFlickable.contentY = cursorRectangle.y + cursorRectangle.height - settingsFlickable.height
							}
							else if (cursorRectangle.y < settingsFlickable.contentY){
								settingsFlickable.contentY = cursorRectangle.y
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: serviceLogPageComp

		MessageCollectionView {
			anchors.fill: parent
			filterRightMargin: statusPopup.width + Style.marginXL
			collectionId: "ServiceLog"
			gqlGetListCommandId: "GetServiceLog"
			subscriptionCommandId: "OnServiceLogCollectionChanged"

			function getHeaders(){
				return serviceEditorContainer.getHeaders()
			}

			function handleSubscription(dataModel){
				if (!dataModel || !dataModel.containsKey("serviceid")){
					return
				}
				if (dataModel.getData("serviceid") === serviceEditorContainer.serviceData.m_id){
					doUpdateGui()
				}
			}
		}
	}
	
	Component {
		id: administrationViewComp;
			
			Item {
				id: administrationViewItem
				anchors.fill: parent;
				
				function getHeaders(){
					if (serviceEditorContainer.serviceTypeId === ""){
						console.error("Unable to get additional parameters. Product-ID is empty");
						return null;
					}
					
					let obj = serviceEditorContainer.getHeaders();
					obj["productId"] = serviceEditorContainer.serviceTypeId;
					obj["token"] = userTokenProvider.accessToken;
					
					return obj;
				}
				
				UserTokenProvider {
					id: userTokenProvider
					productId: serviceEditorContainer.serviceTypeId;
					isTokenGlobal: false
					
					function getHeaders(){
						return administrationViewItem.getHeaders();
					}
					
					onAccepted: {
						authorizationPage.visible = false;
						loader.sourceComponent = administrationViewDocument
					}
				}
				
				AuthorizationPage {
					id: authorizationPage
					anchors.fill: parent;
					appName: serviceEditorContainer.serviceTypeId
					canRegisterUser: false
					canRecoveryPassword: false
					onLogin: {
						userTokenProvider.authorization(login, password)
					}
				}
				
				Loader {
					id: loader
					anchors.fill: parent;
				}
				
				Rectangle {
					anchors.fill: parent
					color: Style.backgroundColor2
					visible: !serviceEditorContainer.serviceRunning
					
					Row {
						anchors.centerIn: parent
						spacing: Style.marginM
						
						Image {
							id: image
							width: 30
							height: width
							
							sourceSize.width: width
							sourceSize.height: height
							
							source: "../../../../" + Style.getIconPath("Icons/Warning", Icon.State.On, Icon.Mode.Normal)
						}
						
						BaseText {
							text: qsTr("Service not running")
							font.pixelSize: Style.fontSizeXXL
						}
					}
				}
				
				Component {
					id: administrationViewDocument
					SingleDocumentWorkspaceView {
						id: singleDocumentWorkspaceView
						anchors.fill: administrationViewItem
						documentManager: DocumentService {}
						
						visualStatusProvider: GqlBasedObjectVisualStatusProvider {
							function getHeaders(){
								return administrationViewItem.getHeaders();
							}
						}
						
						Component.onCompleted: {
							addInitialItem(administrationView, "Administration")
						}
						
						Component {
							id: administrationView;
							AdministrationView {
								anchors.fill: parent;
								productId: serviceEditorContainer.serviceTypeId;
								documentManager: singleDocumentWorkspaceView.documentManager;
								
								function getHeaders(){
									return administrationViewItem.getHeaders();
								}
							}
						}
					}
				}
				
	}
}
}
