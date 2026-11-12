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
	
	property string operationServiceId: ""
	
	viewTypeId: "ServiceEditor"
	documentTypeId: "Service"
	
	removeDialogTitle: qsTr("Deleting an service");
	removeMessage: qsTr("Delete the selected service ?");
	
	// Shared with onServiceStatusChanged: only known stopped/running allow Start/Stop.
	// undefined (agent disconnected) must keep both disabled.
	property var serviceStatusModel: ServiceStatusModel {}

	function updateItemSelection(selectedItems){
		if (!collectionView){
			return
		}
		
		if (!collectionView.commandsController){
			return
		}
		
		let commandsController = collectionView.commandsController
		let elements = collectionView.table.elements;

		let startEnabled = false
		let stopEnabled = false
		
		let isEnabled = selectedItems.length > 0;
		if (isEnabled){
			let status = elements.getData(ServiceItemTypeMetaInfo.s_status, selectedItems[0]);
			startEnabled = serviceStatusModel.isStartable(status)
			stopEnabled = serviceStatusModel.isStoppable(status)
		}
		
		commandsController.setCommandIsEnabled("Remove", isEnabled)
		commandsController.setCommandIsEnabled("Edit", isEnabled)
		setCommandEnabled("Start", startEnabled);
		setCommandEnabled("Stop", stopEnabled);
	}
	
	// Start/Stop also live in the context menu, which keeps its own enabled state.
	function setCommandEnabled(commandId, isEnabled){
		if (collectionView && collectionView.commandsController){
			collectionView.commandsController.setCommandIsEnabled(commandId, isEnabled)
		}
		
		for (let i = 0; i < container.contextMenuModel.getItemsCount(); i++){
			if (container.contextMenuModel.getData("id", i) === commandId){
				container.contextMenuModel.setData("isEnabled", isEnabled, i);
			}
		}
	}
	
	function getHeaders(){
		return {};
	}
	
	onCommandActivated: {
		if (commandId !== "Start" && commandId !== "Stop"){
			return
		}
		
		// commandHandle() emits commandActivated even for disabled commands.
		if (!collectionView || !collectionView.commandsController
				|| !collectionView.commandsController.commandIsEnabled(commandId)){
			return
		}
		
		if (commandId === "Start"){
			onStart();
		}
		else{
			onStop();
		}
	}
	
	function startService(serviceId){
		serviceController.startService(serviceId)
	}
	
	function stopService(serviceId){
		serviceController.stopService(serviceId)
	}
	
	function beginOperation(serviceId, description){
		operationServiceId = serviceId
		if (collectionView && collectionView.setServiceOperation){
			collectionView.setServiceOperation(true, description)
		}
	}
	
	function finishOperation(){
		operationServiceId = ""
		if (collectionView && collectionView.setServiceOperation){
			collectionView.setServiceOperation(false, "")
		}
	}
	
	function onStart(){
		let elements = container.collectionView.table.elements;
		let indexes = container.collectionView.table.getSelectedIndexes();
		if (indexes.length > 0){
			let serviceId = elements.getData(ServiceItemTypeMetaInfo.s_id, indexes[0])

			setCommandEnabled("Start", false)
			startService(serviceId);
		}
	}

	function onStop(){
		let elements = container.collectionView.table.elements;
		let indexes = container.collectionView.table.getSelectedIndexes();
		if (indexes.length > 0){
			let serviceId = elements.getData(ServiceItemTypeMetaInfo.s_id, indexes[0])

			setCommandEnabled("Stop", false)
			stopService(serviceId)
		}
	}
	
	function setupContextMenu(){
		let commandsController = collectionView.commandsController;
		if (commandsController){
			container.contextMenuModel.clear();
			
			let canEdit = commandsController.commandExists("Edit");
			let canRemove = commandsController.commandExists("Remove");
			let canStart = commandsController.commandExists("Start");
			let canStop = commandsController.commandExists("Stop");
			
			if (canStart){
				let index = container.contextMenuModel.insertNewItem();
				
				container.contextMenuModel.setData("id", "Start", index);
				container.contextMenuModel.setData("name", qsTr("Start"), index);
				container.contextMenuModel.setData("icon", "Icons/Play", index);
				container.contextMenuModel.setData("isEnabled", commandsController.commandIsEnabled("Start"), index);
			}
			
			if (canStop){
				let index = container.contextMenuModel.insertNewItem();
				
				container.contextMenuModel.setData("id", "Stop", index);
				container.contextMenuModel.setData("name", qsTr("Stop"), index);
				container.contextMenuModel.setData("icon", "Icons/Stop", index);
				container.contextMenuModel.setData("isEnabled", commandsController.commandIsEnabled("Stop"), index);
			}
			
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

		function isTransitionalStatus(status){
			let value = String(status)
			return value === ServiceStatus.s_Starting
				|| value === "STARTING"
				|| value === "SS_STARTING"
				|| value === "starting"
				|| value === ServiceStatus.s_Stopping
				|| value === "STOPPING"
				|| value === "SS_STOPPING"
				|| value === "stopping"
		}

		function isStartingStatus(status){
			let value = String(status)
			return value === ServiceStatus.s_Starting
				|| value === "STARTING"
				|| value === "SS_STARTING"
				|| value === "starting"
		}

		function disableStartStopCommands(){
			container.setCommandEnabled("Start", false)
			container.setCommandEnabled("Stop", false)
		}

		onBeginStartService: {
			container.beginOperation(serviceId, qsTr("Starting service..."))
			disableStartStopCommands()
		}

		onBeginStopService: {
			container.beginOperation(serviceId, qsTr("Stopping service..."))
			disableStartStopCommands()
		}

		// Do not finish on serviceStarted/serviceStopped: StartService often returns STARTING
		// while the process is still coming up. Popup stays until a terminal status
		// (RUNNING / NOT_RUNNING) arrives via onServiceStatusChanged (subscription or response).

		onServiceStatusChanged: {
			// Only drive the operation popup for the service we started/stopped (or any
			// when operationServiceId is empty after a race).
			if (container.operationServiceId !== "" && serviceId !== container.operationServiceId){
				return
			}

			if (isStartingStatus(status)){
				container.beginOperation(serviceId, qsTr("Starting service..."))
				disableStartStopCommands()
				return
			}
			if (isTransitionalStatus(status)){
				container.beginOperation(serviceId, qsTr("Stopping service..."))
				disableStartStopCommands()
				return
			}

			// Terminal status (running / notRunning / undefined) — hide popup and
			// restore Start/Stop only when the process state is known and actionable.
			container.finishOperation()

			container.setCommandEnabled("Start", container.serviceStatusModel.isStartable(status))
			container.setCommandEnabled("Stop", container.serviceStatusModel.isStoppable(status))
		}

		onStartServiceFailed: container.finishOperation()
		onStopServiceFailed: container.finishOperation()
	}
}

