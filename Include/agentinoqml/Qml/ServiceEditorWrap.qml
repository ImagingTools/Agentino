import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtauthgui 1.0
import imtcontrols 1.0
import agentino 1.0
import agentinoServicesSdl 1.0

ServiceEditor {
	id: serviceEditor

	// clientId is declared on ServiceEditor; set by collection/topology views.
	property string serviceId: serviceEditor.serviceData ? serviceEditor.serviceData.m_id : ""

	// L3 transport isolated from presentation (QG2).
	property var transport: ServiceEditorTransport {
		headersProvider: serviceEditor
	}

	onLoadPlugin: {
		serviceEditor.pluginLoaded = false
		serviceEditor.pluginLoading = true
		serviceEditor.pluginLoadFailed = false
		if (!serviceEditor.serviceData){
			return
		}
		transport.loadPlugin(serviceEditor.serviceData.m_path)
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
		transport.saveSettings(serviceEditor.serviceId, content)
	}

	onRequestAvailableConnections: transport.loadAvailableConnections(connectionUsageIds)

	onSetOutputConnection: {
		transport.setOutputConnection(serviceEditor.serviceId, connectionId, dependantConnectionId)
	}

	Connections {
		target: serviceEditor.transport

		function onPluginLoadFailed(){
			serviceEditor.pluginLoadingFailed()
		}

		function onAvailableConnectionsLoaded(payload){
			serviceEditor.setAvailableConnections(payload)
		}

		function onOutputConnectionSet(connectionId, successful, connectionParam){
			serviceEditor.outputConnectionApplied(connectionId, successful, connectionParam)
		}

		function onPluginLoaded(pluginInfo) {
			if (!serviceEditor.serviceData){
				return
			}
			let documentManager = serviceEditor.documentManager
			if (!documentManager){
				return
			}
			documentManager.setBlockUndoManager(serviceEditor.serviceId, true)
			let info = pluginInfo
			if (info && info.hasInputConnections && info.hasInputConnections()){
				serviceEditor.serviceData.m_inputConnections = info.m_inputConnections.copyMe()
				serviceEditor.serviceData.m_inputConnections.owner = serviceEditor.serviceData
			}
			if (info && info.hasOutputConnections && info.hasOutputConnections()){
				serviceEditor.serviceData.m_outputConnections = info.m_outputConnections.copyMe()
				serviceEditor.serviceData.m_outputConnections.owner = serviceEditor.serviceData
			}
			documentManager.setBlockUndoManager(serviceEditor.serviceId, false)
			serviceEditor.setPluginLoaded(serviceEditor.serviceData.m_path)
			serviceEditor.pluginLoading = false
			serviceEditor.updateGui()
		}
		function onCollectionChangedForService(serviceId) {
			if (!serviceEditor.serviceData){
				return
			}
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

	property var serviceStatusModel: ServiceStatusModel {}

	property bool updateCommands: commandsReceived && serviceData !== null
	onUpdateCommandsChanged: {
		if (updateCommands && commandsController){
			let status = serviceEditor.serviceStatus !== ""
				? serviceEditor.serviceStatus
				: serviceEditor.normalizeServiceStatus(serviceData.m_status)
			// undefined (agent offline) is not startable/stoppable.
			commandsController.setCommandIsEnabled("Start", serviceStatusModel.isStartable(status))
			commandsController.setCommandIsEnabled("Stop", serviceStatusModel.isStoppable(status))
		}
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
				serviceEditor.setServiceStatus(documentModel.m_status)
				serviceEditor.refreshIsNewService()
				if (serviceEditor.commandsController){
					serviceEditor.commandsController.setCommandIsEnabled("Start", serviceStatusModel.isStartable(serviceEditor.serviceStatus))
					serviceEditor.commandsController.setCommandIsEnabled("Stop", serviceStatusModel.isStoppable(serviceEditor.serviceStatus))
				}
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
			serviceDocumentDataController.documentId = savedDocumentId
			serviceDocumentDataController.updateDocumentModel()
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

		onServiceStatusChanged: {
			if (serviceId === serviceEditor.serviceId){
				serviceEditor.setServiceStatus(status)
				let normalized = serviceEditor.serviceStatus
				if (normalized === ServiceStatus.s_Starting){
					serviceEditor.setOperationInProgress(true, qsTr("Starting service..."))
				}
				else if (normalized === ServiceStatus.s_Stopping){
					serviceEditor.setOperationInProgress(true, qsTr("Stopping service..."))
				}
				else {
					serviceEditor.setOperationInProgress(false, "")
				}

				if (serviceEditor.commandsController){
					// isStartable/isStoppable treat undefined as neither (agent offline).
					serviceEditor.commandsController.setCommandIsEnabled("Start", serviceStatusModel.isStartable(normalized))
					serviceEditor.commandsController.setCommandIsEnabled("Stop", serviceStatusModel.isStoppable(normalized))
				}
			}
		}

		onStartServiceFailed: {
			if (serviceId === serviceEditor.serviceId){
				serviceEditor.setOperationInProgress(false, "")
			}
		}

		onStopServiceFailed: {
			if (serviceId === serviceEditor.serviceId){
				serviceEditor.setOperationInProgress(false, "")
			}
		}

		function getHeaders(){
			return serviceEditor.getHeaders()
		}
	}
}
