import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0
import agentinoServicesSdl 1.0
import agentino 1.0

RemoteCollectionView {
	id: root;
	
	property string clientId;
	property string clientName;
	property string serviceId;
	property string serviceName;
	
	filterMenuVisible: false;
	
	collectionId: "Services";
	additionalFieldIds: ["Description","Status","StatusName", "DependencyStatus", "DependantStatusInfo"]
	
	hasPagination: false
	
	property var documentManager: MainDocumentManager.getDocumentManager(root.collectionId);
	
	Component.onDestruction: {
		let documentManagerPtr = MainDocumentManager.getDocumentManager("Agents")
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
		if (clientId == ""){
			return
		}
		
		commandsController.commandId = root.collectionId;
		dataController.collectionId = root.collectionId;
		
		// let documentManagerPtr = MainDocumentManager.getDocumentManager(root.collectionId)
		let documentManagerPtr = root.documentManager;
		if (documentManagerPtr){
			root.commandsDelegate.documentManager = documentManagerPtr
			
			documentManagerPtr.registerDocumentView("Service", "ServiceEditor", serviceEditorComp);
			documentManagerPtr.registerDocumentDataController("Service", serviceDataControllerComp);
			documentManagerPtr.registerDocumentValidator("Service", serviceValidatorComp);
		}
	}
	
	onHeadersChanged: {
		if (root.table.headers.getItemsCount() > 0){
			let orderIndex = root.table.getHeaderIndex("StatusName");
			root.table.setColumnContentComponent(orderIndex, stateColumnContentComp);
			let nameIndex = root.table.getHeaderIndex("Name");
			root.table.setColumnContentComponent(nameIndex, nameColumnContentComp);
		}
	}
	
	commandsDelegateComp: Component {ServiceCollectionViewCommandsDelegate {
			id: serviceCommandsDelegate
			collectionView: root
			documentTypeId: "Service"
			onCommandActivated: {
				if (commandId == "Start" || commandId == "Stop"){
					// root.commandsController.setCommandIsEnabled("Start", false);
					// root.commandsController.setCommandIsEnabled("Stop", false);
				}
			}
			
			function getHeaders(){
				return root.getHeaders()
			}
		}
	}
	
	dataControllerComp: Component {CollectionRepresentation {
			id: collectionRepresentation
			
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
		headers["serviceid"] = root.serviceId;
		console.log("ServiceCollectionView getHeaders", headers)
		return headers
	}
	
	
	onSelectionChanged: {
		console.log("onSelectionChanged", selection)
		if (selection.length > 0){
			let index = selection[0];
			
			root.serviceId = root.table.elements.getData("Id", index);
			root.serviceName = root.table.elements.getData("Name", index);
		}
	}
	
	onSelectedIndexChanged: {
		console.log("onSelectedIndexChanged", index)
		root.serviceId = root.table.elements.getData("Id", index);
		root.serviceName = root.table.elements.getData("Name", index);
	}
	
	Component {
		id: serviceEditorComp;
		
		ServiceEditor {
			id: serviceEditor
			documentManager: root.documentManager;
			
			commandsDelegateComp: Component {ViewCommandsDelegateBase {
					view: serviceEditor;
				}
			}
			
			commandsControllerComp: Component {GqlBasedCommandsController {
					typeId: "Service";
					function getHeaders(){
						return root.getHeaders();
					}
				}
			}
			
			function getHeaders(){
				return root.getHeaders();
			}
		}
	}
	
	Component {
		id: serviceValidatorComp;
		
		ServiceValidator {
		}
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
					let status = root.table.elements.getData("Status", rowIndex);
					console.log("status" ,status);
					
					if (status === ServiceStatus.s_Running){
						console.log(ServiceStatus.s_Running ,status);
						
						icon.source = "../../../../" + Style.getIconPath("Icons/Running", Icon.State.On, Icon.Mode.Normal);
					}
					else if (status === ServiceStatus.s_NotRunning  || status === ServiceStatus.s_Stopping || status === ServiceStatus.s_Starting){
						console.log("Stopped" ,status);
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
				tooltipText: width > 0 ? cellDelegate.rowDelegate.tableItem.elements.getData("DependantStatusInfo", cellDelegate.rowIndex) : ""
				decorator: Component {
					ToolButtonDecorator{
						color: "transparent"
					}
				}
			}
			
			onReused: {
				if (rowIndex >= 0){
					let status = root.table.elements.getData("Status", rowIndex);
					let dependencyStatus = cellDelegate.rowDelegate.tableItem.elements.getData("DependencyStatus", rowIndex);
					if (status !== ServiceStatus.s_Running){
						icon.visible =false;
					}
					else if (dependencyStatus === DependencyStatus.s_NotRunning){
						icon.source = "../../../../" + Style.getIconPath("Icons/Error", Icon.State.On, Icon.Mode.Normal);
						icon.visible = true
					}
					else if (dependencyStatus === DependencyStatus.s_Undefined) {
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
	
	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnServiceStatusChanged";
		onMessageReceived: {
			root.doUpdateGui();
		}
	}
}

