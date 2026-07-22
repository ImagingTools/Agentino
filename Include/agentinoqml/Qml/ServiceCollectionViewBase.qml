import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0
import agentinoServicesSdl 1.0
import imtbaseImtCollectionSdl 1.0
import agentino 1.0

RemoteCollectionView {
	id: root;

	// ViewBase's default contentColor (backgroundColor2) showed through the commands panel
	// area above the table - baseColor matches the rest of the Agents/Services UI.
	contentColor: Style.baseColor;

	property string clientId;
	property string clientName;
	property string serviceId;
	property string serviceName;
	property bool serviceOperationInProgress: false
	property string serviceOperationText: ""

	AgentinoCollectionTableStyle {
		id: collectionTableStyle
	}

	// QG1: typed scope; getHeaders() is a thin transport adapter for ImtCore widgets.
	property var dataScope: DataScope {
		agentId: root.clientId
		serviceId: root.serviceId
	}
	
	filterMenuVisible: false;

	collectionId: "Services";
	additionalFieldIds: ["description","status","statusName", "dependencyStatus", "dependantStatusInfo"]
	
	hasPagination: false
	
	property var documentManager: MainDocumentService.getDocumentService(root.collectionId);
	
	function setServiceOperation(inProgress, description){
		serviceOperationInProgress = inProgress
		serviceOperationText = description
	}
	
	Component.onDestruction: {
		let documentManagerPtr = MainDocumentService.getDocumentService("Agents")
		if (documentManagerPtr){
			documentManagerPtr.unRegisterDocumentView("Service" + root.clientId, "ServiceEditor");
			documentManagerPtr.unRegisterDocumentDataController("Service" + root.clientId);
		}
	}
	
	onVisibleChanged: {
		if (visible && table.elements.getItemsCount() !== 0){
			root.doUpdateGui();
		}
	}

	onClientIdChanged: {
		if (root.dataScope)
			root.dataScope.agentId = root.clientId
		if (clientId == ""){
			return
		}

		if (documentManager){
			documentManager.registerDocumentView("Service", serviceEditorComp);
			documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
		}
	}

	onServiceIdChanged: {
		if (root.dataScope)
			root.dataScope.serviceId = root.serviceId
	}
	
	Component.onCompleted: {
		collectionTableStyle.apply(root.table)
	}

	onHeadersChanged: {
		if (root.table.headers.getItemsCount() > 0){
			// Re-apply after CollectionViewBase may reset border defaults on headers.
			collectionTableStyle.apply(root.table)
			let orderIndex = root.table.getHeaderIndex("statusName");
			root.table.setColumnContentComponent(orderIndex, stateColumnContentComp);
			let nameIndex = root.table.getHeaderIndex("name");
			root.table.setColumnContentComponent(nameIndex, nameColumnContentComp);
		}
	}
	
	commandsDelegateComp: Component {ServiceCollectionViewCommandsDelegate {
			id: serviceCommandsDelegate
			collectionView: root
			documentTypeId: "Service"
			documentManager: root.documentManager
			function getHeaders(){
				return root.getHeaders()
			}
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
		// Adapter only: maps DataScope → GQL headers (not for use in pure L1 views).
		let headers = {}
		if (root.dataScope && root.dataScope.agentId)
			headers["clientid"] = root.dataScope.agentId
		else if (root.clientId !== "")
			headers["clientid"] = root.clientId
		if (root.dataScope && root.dataScope.serviceId)
			headers["serviceid"] = root.dataScope.serviceId
		else if (root.serviceId !== "")
			headers["serviceid"] = root.serviceId
		return headers
	}

	onSelectionChanged: {
		if (selectedIndexes.length > 0){
			let index = selectedIndexes[0]

			root.serviceId = root.table.elements.getData(ServiceItemTypeMetaInfo.s_id, index);
			root.serviceName = root.table.elements.getData(ServiceItemTypeMetaInfo.s_name, index);
		}
		else{
			root.serviceId = ""
			root.serviceName = ""
		}
	}
	
	Component {
		id: serviceEditorComp;
		
		ServiceEditorWrap {
			clientId: root.clientId
			documentManager: root.documentManager
		}
	}
	
	Component {
		id: serviceValidatorComp;
		
		ServiceValidator {}
	}
	
	Component {
		id: serviceDataControllerComp
		
		ServiceDocumentDataController {
			function getHeaders(){
				return root.getHeaders();
			}
		}
	}
	
	Component {
		id: stateColumnContentComp;
		TableCellIconTextDelegate {
			icon.width: icon.visible ? 9 : 0;
			onReused: {
				if (rowIndex >= 0){
					let status = root.table.elements.getData(ServiceItemTypeMetaInfo.s_status, rowIndex);
					if (status === ServiceStatus.s_Running){
						console.log(ServiceStatus.s_Running ,status);
						
						icon.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
					}
					else if (status === ServiceStatus.s_NotRunning  || status === ServiceStatus.s_Stopping || status === ServiceStatus.s_Starting){
						icon.source = "../../../../" + Style.getIconPath("Icons/Stopped", Icon.State.On, Icon.Mode.Normal);
					}
					else{
						icon.source = "../../../../" + Style.getIconPath("Icons/Alert", Icon.State.On, Icon.Mode.Normal);
					}
				}
			}
		}
	}
	
	Component {
		id: nameColumnContentComp;
		TableCellIconTextDelegate {
			id: cellDelegate
			icon.width: icon.visible ? 9 : 0;
			
			ToolButton {
				anchors.fill: cellDelegate.icon
				tooltipText: width > 0 ? cellDelegate.rowDelegate.tableItem.elements.getData("dependantStatusInfo", cellDelegate.rowIndex) : ""
				decorator: Component {
					ToolButtonDecorator{
						color: "transparent"
					}
				}
			}
			
			onReused: {
				if (rowIndex >= 0){
					let status = root.table.elements.getData("status", rowIndex);
					let dependencyStatus = cellDelegate.rowDelegate.tableItem.elements.getData("dependencyStatus", rowIndex);
					if (status !== ServiceStatus.s_Running){
						icon.visible = false;
					}
					else if (dependencyStatus === DependencyStatus.s_NotRunning){
						icon.source = "../../../../" + Style.getIconPath("Icons/Error", Icon.State.On, Icon.Mode.Normal);
						icon.visible = true
					}
					else if (dependencyStatus === DependencyStatus.s_Undefined){
						icon.source = "../../../../" + Style.getIconPath("Icons/Warning", Icon.State.On, Icon.Mode.Normal);
						icon.visible = true
					}
					else {
						icon.visible =false;
					}
				}
			}
		}
	}
	
	Rectangle {
		id: serviceOperationIndicator
		anchors.top: parent.top
		anchors.right: parent.right
		anchors.topMargin: Style.marginXL
		anchors.rightMargin: Style.marginXL
		width: serviceOperationRow.width + 2 * Style.marginL
		height: 36
		radius: height / 2
		color: Style.backgroundColor
		border.color: Style.lightBlueColor
		border.width: 1
		visible: root.serviceOperationInProgress
		z: 100
		
		Row {
			id: serviceOperationRow
			anchors.centerIn: parent
			spacing: Style.spacingS
			
			Loading {
				anchors.verticalCenter: parent.verticalCenter
				width: 18
				height: 18
				indicatorSize: 14
				background.color: "transparent"
			}
			
			BaseText {
				anchors.verticalCenter: parent.verticalCenter
				text: root.serviceOperationText
				font.pixelSize: Style.fontSizeM
			}
		}
	}
	
	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnServiceStatusChanged";
		onMessageReceived: {
			// Refresh rows (status → undefined when agent disconnects) then re-apply
			// Start/Stop enablement for the current selection.
			root.doUpdateGui();
			if (root.commandsDelegate && root.commandsDelegate.updateItemSelection){
				root.commandsDelegate.updateItemSelection(root.table.getSelectedIndexes());
			}
		}
	}
}

