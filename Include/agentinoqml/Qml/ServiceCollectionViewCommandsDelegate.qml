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
		if (!collectionView){
			return
		}
		
		if (!collectionView.commandsController){
			return
		}
		
		let commandsController = collectionView.commandsController
		
		let startEnabled = false
		let stopEnabled = false
		
		let isEnabled = selectedItems.length > 0;
		if (isEnabled){
			let elements = collectionView.table.elements;
			let status = elements.getData(ServiceItemTypeMetaInfo.s_status, selectedItems[0]);
			startEnabled = String(status) === ServiceStatus.s_NotRunning
			stopEnabled = String(status) === ServiceStatus.s_Running
		}
		
		commandsController.setCommandIsEnabled("Remove", isEnabled)
		commandsController.setCommandIsEnabled("Edit", isEnabled)
		commandsController.setCommandIsEnabled("Start", startEnabled);
		commandsController.setCommandIsEnabled("Stop", stopEnabled);
	}
	
	function getHeaders(){
		return {};
	}
	
	onCommandActivated: {
		if (commandId === "Start"){
			onStart();
		}
		else if (commandId === "Stop"){
			onStop();
		}
	}
	
	function startService(serviceId){
		serviceController.startService(serviceId)
	}
	
	function stopService(serviceId){
		serviceController.stopService(serviceId)
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
	
	GqlBasedServiceController {
		id: serviceController

		function getHeaders(){
			return container.getHeaders()
		}
		
		onBeginStartService: {
			if (container.commandsController){
				container.commandsController.setCommandIsEnabled("Start", false)
				container.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
	
		onBeginStopService: {
			if (container.commandsController){
				container.commandsController.setCommandIsEnabled("Stop", false)
				container.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onServiceStarted: {
			if (container.commandsController){
				container.commandsController.setCommandIsEnabled("Start", false)
				container.commandsController.setCommandIsEnabled("Stop", true)
			}
		}
		
		onServiceStopped: {
			if (container.commandsController){
				container.commandsController.setCommandIsEnabled("Start", true)
				container.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
	}
}

