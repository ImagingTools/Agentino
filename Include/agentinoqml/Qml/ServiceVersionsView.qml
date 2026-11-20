import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0

// Versions page of a service type, opened in its own tab by a double click on the "Update" page
// (document type "ServiceVersions" of the "Update" document service, see UpdatePage.qml).
// The table lists the versions uploaded to the server (collection "ServiceVersions",
// CServiceVersionCollectionControllerComp); the panel on the right lists the agents the selected
// version is installed on.
ViewBase {
	id: serviceVersionsView

	// Service type ID - the ID of the document, set by DocumentService.setupDocumentView.
	property string documentId: ""

	function updateGui(){
		versionsCollectionView.doUpdateGui()
	}

	onDocumentIdChanged: {
		versionsCollectionView.doUpdateGui()
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

		// The panel of this view is used instead: the built-in one is hidden in a tab narrower than 5 panel widths.
		visibleMetaInfo: false
		// The server sends no change notifications for this collection.
		subscriptionCommandId: ""

		// The versions of this service type only (HeaderFilterHeaderIds of the server controller).
		function getHeaders(){
			return {"servicetypeid": serviceVersionsView.documentId}
		}

		commandsDelegateComp: Component {
			ServiceVersionsCommandsDelegate {
				collectionView: versionsCollectionView

				onLoadRequested: {
					PopupManager.addInfoMessage(qsTr("Loading of service versions is not available yet"), true)
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
