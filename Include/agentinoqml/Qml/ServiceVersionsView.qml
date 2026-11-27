import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtbaseImtCollectionSdl 1.0
import agentinoUpdatesSdl 1.0

// Versions page of a service type, opened in its own tab by a double click on the "Update" page
// (document type "ServiceVersions" of the "Update" document service, see UpdatePage.qml).
// The table lists the versions uploaded to the server (collection "ServiceVersions",
// CServiceVersionCollectionControllerComp); the panel on the right lists the agents the selected
// version is installed on.
// Versions differ by the operating system family selected on the "Update" page: the page lists the
// versions of that family only (all of them for "All" / "Unknown", with "Add" unavailable).
// "Set service folder" chooses the folder on the server the versions of the service for the family are
// chosen in; the server keeps it (GetServiceVersionFolder / SetServiceVersionFolder).
// "Add" chooses the folder of a version inside it (FileSystemBrowserDialog, the file system of the
// server - no agent header), so it is unavailable until the folder is set: the name of the folder is
// the version; the operating system family, the path and the upload date are stored with it
// (AddServiceVersion).
// "Install" chooses the agents to install the selected version on (InstallServiceVersionDialog); the
// server sends the URLs of the archives (*.zip) of the folder of the version to them
// (InstallServiceVersion) and each agent updates its services of the type of the version: it downloads
// the archives over HTTP into the folder of the executable of the service, stops the service, unpacks
// the archives, starts the service, removes the archives and reports the result to the server. The
// popup of the installation (ServiceVersionInstallationPopup) follows it on the agents until it has
// ended on all of them (GetServiceVersionInstallation).
ViewBase {
	id: serviceVersionsView

	// "<operating system family>/<service type ID>" - the ID of the document, set by
	// DocumentService.setupDocumentView (see UpdatePage.openServiceVersions). The family is empty for all of them.
	property string documentId: ""

	// Parts of the document ID; an ID without the separator is a service type ID alone.
	readonly property int documentIdSeparatorIndex: documentId.indexOf("/")
	readonly property string operatingSystem: documentIdSeparatorIndex >= 0 ? documentId.substring(0, documentIdSeparatorIndex) : ""
	readonly property string serviceTypeId: documentIdSeparatorIndex >= 0 ? documentId.substring(documentIdSeparatorIndex + 1) : documentId

	// Folder on the server the versions of the service for the operating system family are chosen in
	// ("Set service folder", kept by the server); forward slashes. Empty while it is not set.
	property string serviceFolder: ""

	function updateGui(){
		versionsCollectionView.doUpdateGui()
	}

	onDocumentIdChanged: {
		versionsCollectionView.doUpdateGui()
		serviceVersionsView.loadServiceFolder()
	}

	// The folder belongs to one operating system family: none for "All" / "Unknown".
	function loadServiceFolder(){
		serviceVersionsView.serviceFolder = ""
		if (serviceVersionsView.operatingSystem === "" || serviceVersionsView.serviceTypeId === ""){
			return
		}

		getServiceFolderRequestSender.inputParams = Gql.GqlObject("input")
		getServiceFolderRequestSender.addInputParam("serviceTypeId", serviceVersionsView.serviceTypeId)
		getServiceFolderRequestSender.addInputParam("operatingSystem", serviceVersionsView.operatingSystem)
		getServiceFolderRequestSender.requestedDocumentId = serviceVersionsView.documentId
		getServiceFolderRequestSender.send()
	}

	function openSetServiceFolderDialog(){
		// The current folder (if any) is shown selected inside its parent; otherwise the dialog starts
		// at the root of the file system of the server.
		ModalDialogManager.openDialog(serviceFolderDialogComp, {
			"startPath": serviceVersionsView.serviceFolder,
			"pathKind": Enums.pathKindDir,
			"showOnlyFolders": true,
			"titleText": qsTr("Select the folder of the service versions")
		})
	}

	function setServiceFolder(path){
		let folderPath = serviceVersionsView.normalizeFolderPath(path)
		if (folderPath === "" || serviceVersionsView.operatingSystem === ""){
			return
		}

		setServiceFolderRequestSender.inputParams = Gql.GqlObject("input")
		setServiceFolderRequestSender.addInputParam("serviceTypeId", serviceVersionsView.serviceTypeId)
		setServiceFolderRequestSender.addInputParam("operatingSystem", serviceVersionsView.operatingSystem)
		setServiceFolderRequestSender.addInputParam("path", folderPath)
		setServiceFolderRequestSender.send()
	}

	function updateMetaInfo(){
		let ids = versionsCollectionView.getSelectedIds()

		versionsMetaInfo.contentVisible = ids.length === 1
		if (ids.length === 1 && versionsCollectionView.dataController){
			versionsMetaInfo.startLoading()
			versionsCollectionView.dataController.getObjectMetaInfo(ids[0])
		}
	}

	// Versions of the selected rows, e.g. ["1.0.0.513"]
	function getSelectedVersionNames(){
		let result = []
		let elements = versionsCollectionView.table.elements
		let indexes = versionsCollectionView.table.getSelectedIndexes()
		for (let i = 0; i < indexes.length; ++i){
			if (elements && elements.containsKey("version", indexes[i])){
				result.push(String(elements.getData("version", indexes[i])))
			}
		}

		return result
	}

	// Path with forward slashes and without a trailing one, e.g. "C:/Dogma/Puma/1.0.0.513"; a drive root keeps it ("C:/").
	function normalizeFolderPath(path){
		let result = path ? String(path).replace(/\\/g, "/") : ""
		while (result.length > 1 && result.charAt(result.length - 1) === "/" && !/^[A-Za-z]:\/$/.test(result)){
			result = result.substring(0, result.length - 1)
		}

		return result
	}

	// The name of the folder is the version: "C:/Dogma/Puma/1.0.0.513" -> "1.0.0.513". Empty for a drive root.
	function getVersionFromPath(path){
		let folderPath = serviceVersionsView.normalizeFolderPath(path)
		let index = folderPath.lastIndexOf("/")
		let name = index >= 0 ? folderPath.substring(index + 1) : folderPath

		return /^[A-Za-z]:$/.test(name) ? "" : name
	}

	function openAddVersionDialog(){
		// "Add" is disabled until the folder of the service versions is set.
		if (serviceVersionsView.serviceFolder === ""){
			return
		}

		// The trailing slash opens the dialog inside the folder: it browses the parent of the start path
		// (the folder itself here) and pre-selects the start path (the folder, the selection of a plain
		// navigation in a folder choice anyway).
		ModalDialogManager.openDialog(versionFolderDialogComp, {
			"startPath": serviceVersionsView.normalizeFolderPath(serviceVersionsView.serviceFolder) + "/",
			"pathKind": Enums.pathKindDir,
			// The versions are chosen inside the versions folder, among folders only.
			"restrictToStartFolder": true,
			"showOnlyFolders": true,
			"titleText": qsTr("Select the folder of a service version")
		})
	}

	function addVersion(path){
		// A version belongs to one operating system family (the "Add" command is disabled without it).
		if (serviceVersionsView.operatingSystem === ""){
			return
		}

		let folderPath = serviceVersionsView.normalizeFolderPath(path)
		let version = serviceVersionsView.getVersionFromPath(folderPath)
		if (version === ""){
			PopupManager.addWarningMessage(qsTr("The folder of a service version must be selected"), true)
			return
		}

		let inputParams = Gql.GqlObject("input")
		inputParams.InsertField("name", version)

		let itemParams = Gql.GqlObject("item")
		itemParams.InsertField("serviceTypeId", serviceVersionsView.serviceTypeId)
		itemParams.InsertField("operatingSystem", serviceVersionsView.operatingSystem)
		itemParams.InsertField("version", version)
		itemParams.InsertField("path", folderPath)
		inputParams.InsertFieldObject(itemParams)

		addVersionRequestSender.inputParams = inputParams
		addVersionRequestSender.send()
	}

	// The folder of a version on the server.
	Component {
		id: versionFolderDialogComp

		FileSystemBrowserDialog {
			onPathSelected: {
				serviceVersionsView.addVersion(selectedPath)
			}
		}
	}

	// The folder of the service versions on the server.
	Component {
		id: serviceFolderDialogComp

		FileSystemBrowserDialog {
			onPathSelected: {
				serviceVersionsView.setServiceFolder(selectedPath)
			}
		}
	}

	GqlSdlRequestSender {
		id: getServiceFolderRequestSender

		// The document ID the request is sent for: a response for another one is dropped.
		property string requestedDocumentId: ""

		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "GetServiceVersionFolder"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")
			payloadFields.InsertField("path")

			return payloadFields
		}

		sdlObjectComp: Component {
			ServiceVersionFolder {
				onFinished: {
					if (getServiceFolderRequestSender.requestedDocumentId === serviceVersionsView.documentId){
						serviceVersionsView.serviceFolder = serviceVersionsView.normalizeFolderPath(m_path ? String(m_path) : "")
					}
				}
			}
		}
	}

	GqlSdlRequestSender {
		id: setServiceFolderRequestSender

		requestType: 1
		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "SetServiceVersionFolder"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")
			payloadFields.InsertField("path")

			return payloadFields
		}

		sdlObjectComp: Component {
			ServiceVersionFolder {
				onFinished: {
					serviceVersionsView.serviceFolder = serviceVersionsView.normalizeFolderPath(m_path ? String(m_path) : "")
				}
			}
		}
	}

	GqlSdlRequestSender {
		id: addVersionRequestSender

		requestType: 1
		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "AddServiceVersion"

		function getRequestedFields(){
			let payloadFields = Gql.GqlObject("payload")
			payloadFields.InsertField("id")

			return payloadFields
		}

		sdlObjectComp: Component {
			AddedNotificationPayload {
				onFinished: {
					versionsCollectionView.doUpdateGui()
				}
			}
		}
	}

	// Install the version on the agents chosen in the dialog: the server asks every agent to update its
	// services of the type of the version. A single version is installed at once ("Install" needs one).
	function installVersion(versionIds, agentIds){
		if (!versionIds || versionIds.length !== 1 || !agentIds || agentIds.length === 0){
			return
		}

		installVersionInput.m_versionId = String(versionIds[0])
		installVersionInput.m_agentIds = agentIds
		installVersionRequestSender.send(installVersionInput)
	}

	// Why the installation was not started on some agents; the agents it was started on are followed in a
	// popup until the installation has ended on all of them (installationProgressComp).
	function showInstallationResult(installationId, version, installations){
		let startedCount = 0
		let count = installations && installations.count !== undefined ? installations.count : 0
		for (let i = 0; i < count; ++i){
			let installation = installations.get(i).item
			if (!installation){
				continue
			}

			if (installation.m_started === true){
				startedCount++
			}
			else{
				let agentName = installation.m_agentName ? String(installation.m_agentName) : String(installation.m_agentId)
				PopupManager.addWarningMessage(qsTr("The version %1 is not installed on %2: %3")
											   .arg(version).arg(agentName).arg(installation.m_message ? String(installation.m_message) : ""), true)
			}
		}

		if (startedCount > 0 && installationId !== ""){
			let progress = installationProgressComp.createObject(serviceVersionsView, {
				"installationId": installationId,
				"version": version
			})
			progress.update(installations)
		}
		else if (startedCount > 0){
			// A server without the installation tracker: the progress is not known.
			PopupManager.addSuccessMessage(qsTr("The installation of the version %1 has started").arg(version), true)
		}
		else if (count === 0){
			PopupManager.addWarningMessage(qsTr("The version %1 is not installed on any agent").arg(version), true)
		}
	}

	// An installation followed until it has ended on every agent: its popup (ServiceVersionInstallationPopup)
	// lists the agents with their states; the state is requested from the server every few seconds
	// (GetServiceVersionInstallation). When every agent has been updated, the popup says so and closes.
	// Created with a parent (see the smart objects of JQML), destroyed when its popup is closed.
	Component {
		id: installationProgressComp

		Item {
			id: installationProgress

			property string installationId: ""
			property string version: ""
			// Rows of the popup: {agentId, agentName, state, message} - an agent with several services of the
			// type has the state of all of them (failed if one has failed, running while one runs)
			property ListModel agents: ListModel {}
			property bool finished: false
			property string resultText: ""
			property bool requestPending: false
			// Failed requests in a row (the server is not reachable, the installation is unknown)
			property int failedRequestCount: 0

			readonly property string messageId: "ServiceVersionInstallation_" + installationProgress.installationId
			readonly property int maxFailedRequestCount: 10

			property ServiceVersionInstallationInput installationInput: ServiceVersionInstallationInput {}

			function showPopup(){
				PopupManager.addCustomMessage(installationProgress.messageId, installationPopupComp, {"progress": installationProgress})
			}

			// installations: the BaseModel of InstallServiceVersionPayload (m_installations)
			function update(installations){
				let states = {}
				let order = []
				let count = installations && installations.count !== undefined ? installations.count : 0
				for (let i = 0; i < count; ++i){
					let installation = installations.get(i).item
					if (!installation || installation.m_started !== true){
						continue
					}

					let agentId = String(installation.m_agentId)
					if (!states.hasOwnProperty(agentId)){
						states[agentId] = {
							"agentId": agentId,
							"agentName": installation.m_agentName ? String(installation.m_agentName) : agentId,
							"state": "Succeeded",
							"message": ""
						}
						order.push(agentId)
					}

					let agentState = states[agentId]
					let state = installation.m_state ? String(installation.m_state) : "Running"
					if (state === "Failed"){
						let message = installation.m_message ? String(installation.m_message) : ""
						if (count > 1 && installation.m_serviceName){
							message = String(installation.m_serviceName) + ": " + message
						}

						agentState.message = agentState.message !== "" ? agentState.message + "\n" + message : message
						agentState.state = "Failed"
					}
					else if (state === "Running" && agentState.state !== "Failed"){
						agentState.state = "Running"
					}
				}

				for (let j = 0; j < order.length; ++j){
					let row = states[order[j]]
					if (j < installationProgress.agents.count){
						installationProgress.agents.setProperty(j, "agentId", row.agentId)
						installationProgress.agents.setProperty(j, "agentName", row.agentName)
						installationProgress.agents.setProperty(j, "state", row.state)
						installationProgress.agents.setProperty(j, "message", row.message)
					}
					else{
						installationProgress.agents.append(row)
					}
				}

				let runningCount = 0
				let failedCount = 0
				for (let k = 0; k < order.length; ++k){
					if (states[order[k]].state === "Running"){
						runningCount++
					}
					else if (states[order[k]].state === "Failed"){
						failedCount++
					}
				}

				if (runningCount === 0){
					installationProgress.finish(order.length, failedCount)
				}
				else if (!PopupManager.messageIsOpened(installationProgress.messageId)){
					// Shown again if other messages have pushed it out of the popup container.
					installationProgress.showPopup()
				}
			}

			function finish(agentCount, failedCount){
				if (installationProgress.finished){
					return
				}

				if (failedCount === 0){
					installationProgress.resultText = qsTr("All agents have been updated to the version %1").arg(installationProgress.version)
				}
				else{
					installationProgress.resultText = qsTr("The version %1 is installed on %2 of %3 agents")
							.arg(installationProgress.version).arg(agentCount - failedCount).arg(agentCount)
				}

				installationProgress.finished = true

				if (!PopupManager.messageIsOpened(installationProgress.messageId)){
					installationProgress.showPopup()
				}

				// Every agent has been updated: the popup closes after the result was shown; otherwise it is
				// closed by its close button.
				if (failedCount === 0){
					closeTimer.start()
				}
				else{
					closedCheckTimer.start()
				}
			}

			// The state is not known any more: the popup can be closed.
			function stopFollowing(message){
				if (installationProgress.finished){
					return
				}

				installationProgress.resultText = message
				installationProgress.finished = true

				if (!PopupManager.messageIsOpened(installationProgress.messageId)){
					installationProgress.showPopup()
				}

				closedCheckTimer.start()
			}

			function requestState(){
				if (installationProgress.requestPending || installationProgress.finished){
					return
				}

				installationProgress.requestPending = true
				installationProgress.installationInput.m_installationId = installationProgress.installationId
				stateRequestSender.send(installationProgress.installationInput)
			}

			Component.onCompleted: {
				installationProgress.showPopup()
			}

			// The popup shows this object: it goes with it (e.g. the tab of the versions is closed).
			Component.onDestruction: {
				PopupManager.closeMessage(installationProgress.messageId)
			}

			Timer {
				id: pollTimer

				interval: 2000
				repeat: true
				running: !installationProgress.finished

				onTriggered: {
					installationProgress.requestState()
				}
			}

			Timer {
				id: closeTimer

				interval: 3000

				onTriggered: {
					PopupManager.closeMessage(installationProgress.messageId)
					installationProgress.destroy()
				}
			}

			// An ended installation is forgotten when its popup has been closed.
			Timer {
				id: closedCheckTimer

				interval: 1000
				repeat: true

				onTriggered: {
					if (!PopupManager.messageIsOpened(installationProgress.messageId)){
						closedCheckTimer.stop()
						installationProgress.destroy()
					}
				}
			}

			GqlSdlRequestSender {
				id: stateRequestSender

				// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
				// never starts a send with an unresolved singleton property.
				gqlCommandId: "GetServiceVersionInstallation"

				sdlObjectComp: Component {
					InstallServiceVersionPayload {
						onFinished: {
							installationProgress.failedRequestCount = 0
							installationProgress.update(hasInstallations() ? m_installations : null)
						}
					}
				}

				// A failed request is repeated by the next poll, without a popup of its own.
				function onError(message, type){
					installationProgress.failedRequestCount++
					if (installationProgress.failedRequestCount >= installationProgress.maxFailedRequestCount){
						installationProgress.stopFollowing(qsTr("The state of the installation is not known: %1").arg(message))
					}

					stateRequestSender.finished(-1)
				}

				onFinished: {
					installationProgress.requestPending = false
				}
			}
		}
	}

	Component {
		id: installationPopupComp

		ServiceVersionInstallationPopup {}
	}

	property InstallServiceVersionInput installVersionInput: InstallServiceVersionInput {}

	Component {
		id: installDialogComp

		InstallServiceVersionDialog {
			onInstallConfirmed: {
				serviceVersionsView.installVersion(versionIds, agentIds)
			}
		}
	}

	GqlSdlRequestSender {
		id: installVersionRequestSender

		requestType: 1
		// Wire name (same as AgentinoUpdatesSdlCommandIds) - a string literal so JQML
		// never starts a send with an unresolved singleton property.
		gqlCommandId: "InstallServiceVersion"

		sdlObjectComp: Component {
			InstallServiceVersionPayload {
				onFinished: {
					serviceVersionsView.showInstallationResult(m_installationId ? String(m_installationId) : "",
															   m_version ? String(m_version) : "",
															   hasInstallations() ? m_installations : null)
				}
			}
		}
	}

	RemoteCollectionView {
		id: versionsCollectionView

		objectName: "ServiceVersionsCollectionView"

		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: metaInfoSeparator.left

		collectionId: "ServiceVersions"

		// The path of the folder of a version is requested with the rows but has no column.
		additionalFieldIds: ["id", "name", "path"]

		// The panel of this view is used instead: the built-in one is hidden in a tab narrower than 5 panel widths.
		visibleMetaInfo: false
		// The server sends no change notifications for this collection.
		subscriptionCommandId: ""

		// The versions of this service type and operating system family only (an empty family: all of them).
		function getHeaders(){
			return {
				"servicetypeid": serviceVersionsView.serviceTypeId,
				"operatingsystem": serviceVersionsView.operatingSystem
			}
		}

		commandsDelegateComp: Component {
			ServiceVersionsCommandsDelegate {
				collectionView: versionsCollectionView
				addEnabled: serviceVersionsView.operatingSystem !== "" && serviceVersionsView.serviceFolder !== ""
				setServiceFolderEnabled: serviceVersionsView.operatingSystem !== ""

				onAddRequested: {
					serviceVersionsView.openAddVersionDialog()
				}

				onSetServiceFolderRequested: {
					serviceVersionsView.openSetServiceFolderDialog()
				}

				onInstallRequested: {
					ModalDialogManager.openDialog(installDialogComp, {
						"versionIds": versionIds,
						"versionName": serviceVersionsView.getSelectedVersionNames().join(", ")
					})
				}
			}
		}

		onSelectionChanged: {
			serviceVersionsView.updateMetaInfo()
		}
	}

	Connections {
		target: versionsCollectionView.dataController

		function onObjectMetaInfoReceived(metaInfoData){
			versionsMetaInfo.elementMetaInfo = metaInfoData
			versionsMetaInfo.stopLoading()
		}

		function onObjectMetaInfoReceiveFailed(message){
			versionsMetaInfo.stopLoading()
		}

		// "Remove" (the standard collection command: RemoveElements, removed by the server controller).
		// The server sends no change notifications for this collection, so the list is reloaded here.
		function onElementsRemoved(elementIds){
			versionsCollectionView.doUpdateGui()
			serviceVersionsView.updateMetaInfo()
		}
	}

	Rectangle {
		id: metaInfoSeparator

		anchors.right: versionsMetaInfo.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		width: 1
		color: Style.borderColor
		opacity: 0.5
	}

	MetaInfo {
		id: versionsMetaInfo

		objectName: "ServiceVersionsMetaInfo"

		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom

		width: Style.sizeHintXXS

		contentVisible: false
	}

	BaseText {
		anchors.fill: versionsMetaInfo
		anchors.margins: Style.marginM

		visible: !versionsMetaInfo.contentVisible

		text: qsTr("Select a version to see the agents it is installed on")
		color: Style.textColor
		font.pixelSize: Style.fontSizeM
		wrapMode: Text.WordWrap
	}
}
