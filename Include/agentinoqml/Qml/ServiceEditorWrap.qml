import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentinoServicesSdl 1.0

ServiceEditor {
	id: serviceEditor
	
	property string clientId
	property string serviceId: serviceEditor.serviceData ? serviceEditor.serviceData.m_id : ""
	property var documentManager
	
	// ServiceEditor's own getHeaders() returns {} - without "clientid"/"serviceid" the
	// server can't route requests like "Authorization" to this specific service
	// (CAgentGqlRemoteRepresentationControllerComp requires both headers), so every
	// GQL request issued from within ServiceEditor (login, administration view, ...)
	// silently fails. Override with the ids this wrapper already has.
	function getHeaders(){
		return {
			"clientid": serviceEditor.clientId,
			"serviceid": serviceEditor.serviceId
		};
	}
	
	onLoadPlugin: {
		serviceEditor.pluginLoaded = false
		serviceEditor.pluginLoading = true
		serviceEditor.pluginLoadFailed = false
		if (!serviceEditor.serviceData){
			return
		}

		loadPluginInput.m_servicePath = serviceEditor.serviceData.m_path
		loadPluginRequestSender.send(loadPluginInput)
	}

	onServiceDataChanged: {
		if (serviceData){
			if (serviceData.hasInputConnections() && serviceData.m_inputConnections.count > 0){
				serviceEditor.pluginLoading = false
				serviceEditor.pluginLoadFailed = false
				serviceEditor.setPluginLoaded(serviceEditor.serviceData.m_path)
			}
		}
		else{
			serviceEditor.pluginLoaded = false
			serviceEditor.pluginLoading = false
			serviceEditor.pluginLoadFailed = false
		}
	}
	
	property string loadedSettingsServiceId: ""
	property bool settingsRequested: false
	onServiceIdChanged: {
		if (serviceId !== "" && serviceId !== loadedSettingsServiceId){
			loadedSettingsServiceId = serviceId
			settingsRequested = false
		}
	}

	onSaveSettings: {
		if (serviceEditor.serviceId === "" || serviceEditor.settingsPath === ""){
			return
		}
		
		updateServiceSettingsInput.m_serviceId = "" + serviceEditor.serviceId
		updateServiceSettingsInput.m_content = "" + content
		updateServiceSettingsRequestSender.send(updateServiceSettingsInput)
	}
	
	ServiceInput {
		id: getServiceSettingsInput
	}
	
	ServiceSettingsInput {
		id: updateServiceSettingsInput
	}
	
	GqlSdlRequestSender {
		id: getServiceSettingsRequestSender
		gqlCommandId: AgentinoServicesSdlCommandIds.s_getServiceSettings
		
		sdlObjectComp: Component {
			ServiceSettingsPayload {
				onFinished: {
					serviceEditor.setSettings(m_content, m_exists, m_path)
				}
			}
		}
		
		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}

	GqlSdlRequestSender {
		id: updateServiceSettingsRequestSender
		requestType: 1
		gqlCommandId: AgentinoServicesSdlCommandIds.s_updateServiceSettings
		
		sdlObjectComp: Component {
			ServiceSettingsPayload {
				onFinished: {
					serviceEditor.setSettings(m_content, m_exists, m_path)
					ModalDialogManager.showWarningDialog(qsTr("Service settings saved"))
				}
			}
		}
		
		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}

	commandsDelegateComp: Component {ViewCommandsDelegateBase {
			view: serviceEditor;
			
			onCommandActivated: {
				let serviceId = serviceEditor.serviceId

				if (commandId == "Start"){
					serviceController.startService(serviceId)
				}
				else if (commandId == "Stop"){
					serviceController.stopService(serviceId)
				}
			}
		}
	}
	
	commandsControllerComp: Component {GqlBasedCommandsController {
			typeId: "Service";
			function getHeaders(){
				return serviceEditor.getHeaders();
			}
			
			onCommandsReceived: {
				serviceEditor.commandsReceived = true
			}
		}
	}
	
	property bool commandsReceived: false
	
	property bool updateCommands: commandsReceived && serviceData !== null
	onUpdateCommandsChanged: {
		if (updateCommands && commandsController){
			commandsController.setCommandIsEnabled("Start", serviceData.m_status === "notRunning")
			commandsController.setCommandIsEnabled("Stop", serviceData.m_status === "running")
		}
	}
	
	function getHeaders(){
		if (!serviceEditor.serviceData){
			return {}
		}

		let headers = {}
		headers["clientid"] = serviceEditor.clientId;
		headers["serviceid"] = serviceEditor.serviceData.m_id;
		return headers
	}
	
	ServiceDocumentDataController {
		id: serviceDocumentDataController
		function getHeaders(){
			return serviceEditor.getHeaders();
		}
		
		onModelChanged: {
			let documentManager = serviceEditor.documentManager
			if (documentManager){
				documentManager.setBlockUndoManager(documentId, true)
				serviceEditor.serviceData.copyFrom(documentModel)
				documentManager.setBlockUndoManager(documentId, false)
				documentManager.clearUndoManager(documentId)
				serviceEditor.serviceIsDirty = false
				serviceEditor.doUpdateGui()
				serviceEditor.pluginLoading = false
				serviceEditor.pluginLoadFailed = false
				serviceEditor.setPluginLoaded(serviceEditor.serviceData.m_path)
			}
		}
	}
	
	Connections {
		target: serviceEditor.documentManager
		function onDocumentIsDirtyChanged(documentId, isDirty) {
			if (documentId === serviceEditor.documentId){
				serviceEditor.serviceIsDirty = isDirty
			}
		}
		
		function onDocumentSaved(savedDocumentId) {
			if (savedDocumentId !== serviceEditor.documentId){
				return
			}
			
			serviceEditor.serviceIsDirty = false
			if (serviceEditor.serviceData && serviceEditor.serviceData.hasPath()) {
				serviceEditor.requestLoadPlugin()
			}
		}
	}
	
	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnServicesCollectionChanged"
		function getHeaders(){
			return serviceEditor.getHeaders();
		}
		
		onMessageReceived: {
			if (!serviceEditor.serviceData){
				return
			}

			if (data.containsKey("serviceid")){
				let serviceId = data.getData("serviceid")
				if (serviceId === serviceEditor.serviceData.m_id){
					let ok = true
					if (serviceEditor.serviceData.hasInputConnections()){
						ok = serviceEditor.serviceData.m_inputConnections.count === 0
					}

					if (ok){
						serviceEditor.pluginLoaded = false
						serviceEditor.pluginLoading = false
						serviceEditor.pluginLoadFailed = false
						serviceDocumentDataController.documentId = serviceId
						serviceDocumentDataController.updateDocumentModel()
					}
				}
			}
		}
	}
	
	GqlBasedServiceController {
		id: serviceController
		onBeginStartService: {
			serviceEditor.setOperationInProgress(true, qsTr("Starting service..."))
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onBeginStopService: {
			serviceEditor.setOperationInProgress(true, qsTr("Stopping service..."))
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onServiceStarted: {
			serviceEditor.setOperationInProgress(false, "")
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", true)
			}
		}
		
		onServiceStopped: {
			serviceEditor.setOperationInProgress(false, "")
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", true)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onServiceStatusChanged: {
			serviceEditor.setOperationInProgress(false, "")
			if (serviceId === serviceEditor.serviceId){
				serviceEditor.serviceRunning = status === "RUNNING"
				
				if (serviceEditor.commandsController){
					serviceEditor.commandsController.setCommandIsEnabled("Start", status === "NOT_RUNNING")
					serviceEditor.commandsController.setCommandIsEnabled("Stop", status === "RUNNING")
				}
				
			}
		}
		
		onStartServiceFailed: {
			serviceEditor.setOperationInProgress(false, "")
		}
		
		onStopServiceFailed: {
			serviceEditor.setOperationInProgress(false, "")
		}
		
		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}
	
	LoadPluginInput {
		id: loadPluginInput
	}
	
	GqlSdlRequestSender {
		id: loadPluginRequestSender
		gqlCommandId: AgentinoServicesSdlCommandIds.s_loadPlugin
		requestType: 1
		onFinished: {
			if (status === -1) {
				serviceEditor.pluginLoadingFailed()
			}
		}
		sdlObjectComp: Component {
			PluginInfo {
				onFinished: {
					if (!serviceEditor.serviceData){
						return
					}
					let documentManager = serviceEditor.documentManager
					if (!documentManager){
						return
					}
					
					documentManager.setBlockUndoManager(serviceEditor.serviceId, true)

					if (hasInputConnections()){
						serviceEditor.serviceData.m_inputConnections = m_inputConnections.copyMe()
						// copyMe() doesn't set .owner, which BaseClass/BaseModel's change-bubbling
						// relies on to forward nested modelChanged signals up to serviceData
						// (compare ServiceData.qml's emplaceInputConnections(), which sets it
						// right after creating the field the normal way). Without this, editing
						// anything inside the reloaded Connections page stops marking the
						// document dirty / registering undo steps.
						serviceEditor.serviceData.m_inputConnections.owner = serviceEditor.serviceData
					}

					if (hasOutputConnections()){
						serviceEditor.serviceData.m_outputConnections = m_outputConnections.copyMe()
						serviceEditor.serviceData.m_outputConnections.owner = serviceEditor.serviceData
					}

					documentManager.setBlockUndoManager(serviceEditor.serviceId, false)

					// doUpdateGui() no-ops while ViewBase's internal blockingUpdateGui/
					// blockingUpdateModel flag is set (e.g. right after the model was just
					// touched above), which is why the Connections page previously stayed
					// empty until the editor was reopened. Call updateGui() directly instead -
					// same fix already applied for the post-save case in onServiceDataChanged.
					serviceEditor.setPluginLoaded(serviceEditor.serviceData.m_path)
					serviceEditor.pluginLoading = false
					serviceEditor.updateGui()
				}
			}
		}
		
		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}
}
