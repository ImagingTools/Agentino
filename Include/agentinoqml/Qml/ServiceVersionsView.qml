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

	// Installation is not implemented yet: the agents chosen in the dialog are not used.
	Component {
		id: installDialogComp

		InstallServiceVersionDialog {}
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
