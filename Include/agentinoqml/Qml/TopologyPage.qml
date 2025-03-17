import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0
import imtdocgui 1.0
import agentino 1.0
import agentinoTopologySdl 1.0
import agentinoServicesSdl 1.0

ViewBase {
	id: topologyPage;
	
	anchors.fill: parent;
	clip: true;
	
	property var documentManager: MainDocumentManager.getDocumentManager("Topology");
	
	//for scrollBars
	property real originX: 0;
	property real originY: 0;
	//for scrollBars
	
	Component.onCompleted: {
		if (documentManager){
			documentManager.registerDocumentView("Service", "ServiceView", serviceEditorComp);
			documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
			documentManager.registerDocumentValidator("Service", serviceValidatorComp);
		}
		
		getTopologyRequestSender.send()
	}
	
	commandsControllerComp: Component {GqlBasedCommandsController {
			id: commandsRepresentationProvider
			typeId: "Topology"
			
			onCommandsChanged: {
				setIsToggleable("AutoFit", true);
				setToggled("AutoFit", scheme.autoFit);
			}
		}
	}
	
	onVisibleChanged: {
		if (topologyPage.visible){
			getTopologyRequestSender.send()
		}
	}
	
	commandsDelegateComp: Component {ServiceCollectionViewCommandsDelegate {
			id: serviceCommandsDelegate
			collectionView: topologyPage
			view: topologyPage;
			
			onCommandActivated: {
				if (commandId === "Save"){
					saveModelRequestSender.send()
				}
				else if (commandId === "ZoomIn"){
					scheme.setAutoFit(false);
					
					scheme.zoomIn();
				}
				else if (commandId === "ZoomOut"){
					scheme.setAutoFit(false);
					
					scheme.zoomOut();
				}
				else if (commandId === "ZoomReset"){
					scheme.resetZoom();
				}
				else if (commandId === "AutoFit"){
					scheme.setAutoFit(!scheme.autoFit);
				}
			}
			
			function onStart(){
				if(scheme.objectsModel.count > scheme.selectedIndex && scheme.selectedIndex >= 0){
					let item = scheme.objectsModel.get(scheme.selectedIndex).item
					let serviceId = item.m_id;
					
					startService(serviceId);
				}
			}
			
			function onStop(){
				if(scheme.objectsModel.count > scheme.selectedIndex && scheme.selectedIndex >= 0){
					let item = scheme.objectsModel.get(scheme.selectedIndex).item
					let serviceId = item.m_id;
					
					stopService(serviceId);
				}
			}
			
			function onEdit(){
				scheme.goInside()
			}
			
			function getHeaders(){
				return topologyPage.getHeaders();
			}
		}
	}
	
	SchemeView {
		id: scheme
		
		anchors.left: parent.left;
		anchors.right: metaInfo.left;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;
		
		property string selectedService: ""
		
		onAutoFitChanged: {
			if (topologyPage.commandsController){
				topologyPage.commandsController.setToggled("AutoFit", scheme.autoFit);
			}
		}
		
		onModelDataChanged: {
			if (topologyPage.commandsController){
				topologyPage.commandsController.setCommandIsEnabled("Save", true)
			}
		}
		
		onSelectedIndexChanged: {
			if (!topologyPage.commandsController){
				return;
			}
						
			if(objectsModel.count > selectedIndex && selectedIndex >= 0){
				let item = scheme.objectsModel.get(scheme.selectedIndex).item
				let serviceId = item.m_id;
				
				selectedService = serviceId;
				let status = item.m_status;
				
				console.log("status", status);
				
				topologyPage.commandsController.setCommandIsEnabled("Start", status === "NOT_RUNNING")
				topologyPage.commandsController.setCommandIsEnabled("Stop", status === "RUNNING")
				topologyPage.commandsController.setCommandIsEnabled("Edit", true)
				
				metaInfo.contentVisible = true;
				metaInfoProvider.getMetaInfo(selectedService);
			}
			else{
				selectedService = ""
				topologyPage.commandsController.setCommandIsEnabled("Start", false)
				topologyPage.commandsController.setCommandIsEnabled("Stop", false)
				topologyPage.commandsController.setCommandIsEnabled("Edit", false)
				
				metaInfo.contentVisible = false;
			}
		}
		
		function goInside(){
			let documentManager = MainDocumentManager.getDocumentManager("Topology");
			if (documentManager){
				if(objectsModel.count > selectedIndex && selectedIndex >= 0){
					let item = scheme.objectsModel.get(scheme.selectedIndex).item
					let serviceId = item.m_id;
					
					documentManager.openDocument(serviceId, "Service", "ServiceView");
				}
			}
		}
	}
	
	MetaInfo {
		id: metaInfo;
		
		anchors.right: parent.right;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;
		
		width: 250;
	}
	
	MetaInfoProvider {
		id: metaInfoProvider;
		
		getMetaInfoGqlCommand: "GetServiceMetaInfo";
		
		onMetaInfoModelChanged: {
			metaInfo.metaInfoModel = metaInfoModel;
		}
		
		function getHeaders(){
			return topologyPage.getHeaders();
		}
	}
	
	function getHeaders(){
		let headers = {}
		
		if(scheme.selectedIndex >= 0){
			let item = scheme.objectsModel.get(scheme.selectedIndex).item
			let agentId = item.m_agentId;
			headers["clientid"] = agentId;
			headers["serviceid"] = scheme.selectedService;
		}
		
		return headers
	}
	
	Component {
		id: serviceValidatorComp;
		
		ServiceValidator {
		}
	}
	
	Component {
		id: serviceEditorComp;
		
		ServiceEditor {
			id: serviceEditor
			documentManager: topologyPage.documentManager;
			commandsDelegateComp: Component {ViewCommandsDelegateBase {
					view: serviceEditor;
				}
			}
			
			commandsControllerComp: Component {GqlBasedCommandsController {
					typeId: "Service";
					function getHeaders(){
						return topologyPage.getHeaders();
					}
				}}
			
			Component.onCompleted: {
				let item = scheme.objectsModel.get(scheme.selectedIndex).item
				let agentId = item.m_agentId;
				serviceEditor.agentId = agentId;
			}
			
			function getHeaders(){
				return topologyPage.getHeaders();
			}
		}
	}
	
	Component {
		id: serviceDataControllerComp
		
		ServiceDocumentDataController {
			function getHeaders(){
				return topologyPage.getHeaders();
			}
		}
	}
	
	SubscriptionClient {
		id: topologySubscriptionClient;
		gqlCommandId: "OnTopologyChanged";
		onMessageReceived: {
			// getTopologyRequestSender.send()
		}
	}
	
	// TODO: Status ?
	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnServiceStatusChanged";
		
		onMessageReceived: {
			console.log("OnServiceStatusChanged", data.toJson());
			if (data.containsKey("OnServiceStatusChanged")){
				let dataModel = data.getData("OnServiceStatusChanged")
				let serviceId = dataModel.getData("serviceid")
				let serviceStatus = dataModel.getData(ServiceStatus.s_Key)
				let dependencyStatus

				let index = scheme.findModelIndex(serviceId);
				if (index < 0){
					return;
				}
				
				let item = scheme.objectsModel.get(index).item

				item.m_status = serviceStatus;
				if (serviceStatus === "RUNNING"){
					item.m_icon1 = "Icons/Running";
				}
				else if (serviceStatus === "NOT_RUNNING"){
					item.m_icon1 = "Icons/Stopped";
				}
				else{
					item.m_icon1 = "Icons/Alert";
				}
				
				if (index === scheme.selectedIndex){
					topologyPage.commandsController.setCommandIsEnabled("Start", serviceStatus === "NOT_RUNNING");
					topologyPage.commandsController.setCommandIsEnabled("Stop", serviceStatus === "RUNNING");
				}
				
				serviceStatus = item.m_status;
				
				let dependencyStatusModel = dataModel.getData(DependencyStatus.s_Key)
				for (let i = 0; i < dependencyStatusModel.getItemsCount(); i++){
					serviceId = dependencyStatusModel.getData("id", i);
					index = scheme.findModelIndex(serviceId);
					dependencyStatus = dependencyStatusModel.getData(DependencyStatus.s_Key, i)
					
					console.log("dependency serviceId", serviceId);
					console.log("dependency index", index);
					
					let serviceItem = scheme.objectsModel.get(index).item;
					
					console.log("serviceItem", serviceItem);
					console.log("dependencyStatus", dependencyStatus);

					if (dependencyStatus === "NOT_RUNNING"){
						serviceItem.m_icon2 = "Icons/Error"
					}
					else if (dependencyStatus === "UNDEFINED") {
						serviceItem.m_icon2 = "Icons/Warning"
					}
					else {
						serviceItem.m_icon2 = ""
					}
					
					if (serviceStatus !== "RUNNING"){
						item.m_icon2 = ""
					}
					
					if (serviceId === scheme.selectedService){
						metaInfoProvider.getMetaInfo(scheme.selectedService);
					}
				}
				
				scheme.requestPaint()
			}
		}
	}
	
	GqlSdlRequestSender {
		id: saveModelRequestSender;
		requestType: 1;
		gqlCommandId: AgentinoTopologySdlCommandIds.s_saveTopology;
		
		inputObjectComp: Component{
			Topology {
				m_services: scheme.objectsModel;
			}
		}
		
		sdlObjectComp: Component {
			SaveTopologyResponse {
				onFinished: {
					topologyPage.commandsController.setCommandIsEnabled("Save", !m_successful);
				}
			}
		}
	}
	
	GqlSdlRequestSender {
		id: getTopologyRequestSender;
		
		gqlCommandId: AgentinoTopologySdlCommandIds.s_getTopology;
		
		sdlObjectComp: Component {
			Topology {
				onFinished: {
					scheme.objectsModel = m_services;
					scheme.requestPaint()
					topologyPage.commandsController.setCommandIsEnabled("Save", false)
				}
			}
		}
	}
}
