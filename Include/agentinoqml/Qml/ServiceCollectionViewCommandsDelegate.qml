import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcolgui 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import agentino 1.0
import agentinoServicesSdl 1.0

DocumentCollectionViewDelegate {
	id: container;
	
	viewTypeId: "ServiceEditor"
	documentTypeId: "Service"
	
	removeDialogTitle: qsTr("Deleting an service");
	removeMessage: qsTr("Delete the selected service ?");
	
	function updateItemSelection(selectedItems){
		if (container.collectionView && container.collectionView.commandsController){
			let isEnabled = selectedItems.length > 0;
			let commandsController = container.collectionView.commandsController;
			if(commandsController){
				commandsController.setCommandIsEnabled("Remove", isEnabled);
				commandsController.setCommandIsEnabled("Edit", isEnabled);
				if (isEnabled){
					let elements = container.collectionView.table.elements;
					
					let status = elements.getData(ServiceItemTypeMetaInfo.s_status, selectedItems[0]);
					
					commandsController.setCommandIsEnabled("Start", String(status) === ServiceStatus.s_NotRunning);
					commandsController.setCommandIsEnabled("Stop", String(status) === ServiceStatus.s_Running);
				}
			}
		}
	}
	
	function getHeaders(){
		return {};
	}
	
	onCommandActivated: {
		if (commandId == "Start" || commandId == "Stop"){
			let commandsController = container.collectionView.commandsController;
			
			commandsController.setCommandIsEnabled("Start", false);
			commandsController.setCommandIsEnabled("Stop", false);
		}
		
		if (commandId === "Start"){
			onStart();
		}
		else if (commandId === "Stop"){
			onStop();
		}
	}
	
	function startService(serviceId){
		serviceInput.m_serviceId = serviceId;
		startServiceRequestSender.send(serviceInput);
	}
	
	function stopService(serviceId){
		serviceInput.m_serviceId = serviceId;
		stopServiceRequestSender.send(serviceInput);
	}
	
	function onStart(){
		let elements = container.collectionView.table.elements;
		let indexes = container.collectionView.table.getSelectedIndexes();
		if (indexes.length > 0){
			let index = indexes[0];
			let serviceId = elements.getData(ServiceItemTypeMetaInfo.s_id, index)
			
			startService(serviceId);
		}
	}
	
	function onStop(){
		let elements = container.collectionView.table.elements;
		let indexes = container.collectionView.table.getSelectedIndexes();
		if (indexes.length > 0){
			let index = indexes[0];
			let serviceId = elements.getData(ServiceItemTypeMetaInfo.s_id, index)
			
			stopService(serviceId)
		}
	}
	
	function setupContextMenu(){
		let commandsController = collectionView.commandsController;
		if (commandsController){
			container.contextMenuModel.clear();
			
			let canEdit = commandsController.commandExists("Edit");
			let canRemove = commandsController.commandExists("Remove");
			
			if (canEdit){
				let index = container.contextMenuModel.insertNewItem();
				
				container.contextMenuModel.setData("id", "Edit", index);
				container.contextMenuModel.setData("name", qsTr("Edit"), index);
				container.contextMenuModel.setData("icon", "Icons/Edit", index);
			}
			
			if (canRemove){
				let index = container.contextMenuModel.insertNewItem();
				
				container.contextMenuModel.setData("id", "Remove", index);
				container.contextMenuModel.setData("name", qsTr("Remove"), index);
				container.contextMenuModel.setData("icon", "Icons/Delete", index);
			}
			
			container.contextMenuModel.refresh();
		}
	}
	
	ServiceInput {
		id: serviceInput;
	}
	
	GqlSdlRequestSender {
		id: startServiceRequestSender;
		requestType: 1;
		gqlCommandId: AgentinoServicesSdlCommandIds.s_startService;
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					console.log("ServiceStatusResponse", m_status);
				}
			}
		}
		
		function getHeaders(){
			return container.getHeaders()
		}
	}
	
	GqlSdlRequestSender {
		id: stopServiceRequestSender;
		requestType: 1;
		gqlCommandId: AgentinoServicesSdlCommandIds.s_stopService;
		
		sdlObjectComp: Component {
			ServiceStatusResponse {
				onFinished: {
					console.log("ServiceStatusResponse", m_status);
				}
			}
		}
		
		function getHeaders(){
			return container.getHeaders()
		}
	}
}

