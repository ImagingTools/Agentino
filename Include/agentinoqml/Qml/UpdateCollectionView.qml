import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0
import agentinoUpdatesSdl 1.0
import imtbaseImtCollectionSdl 1.0
import agentino 1.0

RemoteCollectionView {
	id: root;

	property string clientId;
	property string clientName;

	filterMenuVisible: false;

	collectionId: "Updates";
	additionalFieldIds: ["version", "updateType", "description", "fileSize", "publishedDate", "status"]

	hasPagination: false

	property var documentManager: MainDocumentService.getDocumentService(root.collectionId);

	Component.onCompleted: {
		if (documentManager){
			documentManager.registerDocumentView("Update", updateEditorComp);
			documentManager.registerDocumentDataController("Update", updateDataControllerComp);
		}
	}

	onClientIdChanged: {
		if (clientId == ""){
			return
		}
		root.doUpdateGui();
	}

	onHeadersChanged: {
		if (root.table.headers.getItemsCount() > 0){
			let statusIndex = root.table.getHeaderIndex("status");
			root.table.setColumnContentComponent(statusIndex, statusColumnContentComp);
		}
	}

	dataControllerComp: Component {CollectionRepresentation {
			id: collectionRepresentation
			collectionId: root.collectionId;
			additionalFieldIds: root.additionalFieldIds;

			function getHeaders(){
				return root.getHeaders()
			}
		}
	}

	function handleSubscription(dataModel){
		root.doUpdateGui();
	}

	function getHeaders(){
		let headers = {}
		headers["clientid"] = root.clientId;
		return headers
	}

	onSelectionChanged: {
		if (selectedIndexes.length > 0){
			let index = selectedIndexes[0]
			let updateId = root.table.elements.getData(UpdateItemTypeMetaInfo.s_id, index);
			if (documentManager){
				documentManager.openDocument(updateId, "Update");
			}
		}
	}

	Component {
		id: updateEditorComp;

		UpdateEditor {
			clientId: root.clientId
		}
	}

	Component {
		id: updateDataControllerComp

		DocumentDataController {
			function getHeaders(){
				return root.getHeaders();
			}
		}
	}

	Component {
		id: statusColumnContentComp;
		TableCellIconTextDelegate {
			icon.width: icon.visible ? 9 : 0;
			onReused: {
				if (rowIndex >= 0){
					let status = root.table.elements.getData("status", rowIndex);
					if (status === UpdateStatusEnum.s_Installed){
						icon.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
					}
					else if (status === UpdateStatusEnum.s_Failed){
						icon.source = "../../../../" + Style.getIconPath("Icons/Alert", Icon.State.On, Icon.Mode.Normal);
					}
					else if (status === UpdateStatusEnum.s_Downloading || status === UpdateStatusEnum.s_Installing){
						icon.source = "../../../../" + Style.getIconPath("Icons/Stopped", Icon.State.On, Icon.Mode.Normal);
					}
					else{
						icon.visible = false;
					}
				}
			}
		}
	}

	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnUpdateProgress";
		onMessageReceived: {
			root.doUpdateGui();
		}
	}
}
