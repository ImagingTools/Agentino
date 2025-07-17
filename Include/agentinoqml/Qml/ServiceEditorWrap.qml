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
	
	onLoadPlugin: {
		serviceEditor.pluginLoaded = false
		if (!serviceEditor.serviceData){
			return
		}

		loadPluginInput.m_servicePath = serviceEditor.serviceData.m_path
		loadPluginRequestSender.send(loadPluginInput)
	}

	onServiceDataChanged: {
		if (serviceData){
			if (serviceData.hasInputConnections() && serviceData.m_inputConnections.count > 0){
				pluginLoaded = true
			}
		}
		else{
			pluginLoaded = false
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
				serviceEditor.doUpdateGui()
				serviceEditor.pluginServicePath = serviceEditor.serviceData.m_path
				serviceEditor.pluginLoaded = true
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
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onBeginStopService: {
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onServiceStarted: {
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", false)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", true)
			}
		}
		
		onServiceStopped: {
			if (serviceEditor.commandsController){
				serviceEditor.commandsController.setCommandIsEnabled("Start", true)
				serviceEditor.commandsController.setCommandIsEnabled("Stop", false)
			}
		}
		
		onServiceStatusChanged: {
			if (serviceId === serviceEditor.serviceId){
				serviceEditor.serviceRunning = status === "RUNNING"
				
				if (serviceEditor.commandsController){
					serviceEditor.commandsController.setCommandIsEnabled("Start", status === "NOT_RUNNING")
					serviceEditor.commandsController.setCommandIsEnabled("Stop", status === "RUNNING")
				}
				
			}
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
					
					console.log("PluginInfo", this.toJson())
					console.log("m_inputConnections", m_inputConnections)
					documentManager.setBlockUndoManager(serviceEditor.serviceId, true)

					if (hasInputConnections()){
						serviceEditor.serviceData.m_inputConnections = m_inputConnections.copyMe()
					}

					if (hasOutputConnections()){
						serviceEditor.serviceData.m_outputConnections = m_outputConnections.copyMe()
					}

					documentManager.setBlockUndoManager(serviceEditor.serviceId, false)
					serviceEditor.doUpdateGui()
					
					serviceEditor.pluginServicePath = serviceEditor.serviceData.m_path
					serviceEditor.pluginLoaded = true
				}
			}
		}
		
		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}
}
