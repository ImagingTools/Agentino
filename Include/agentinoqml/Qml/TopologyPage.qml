import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
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
	
	property var documentManager: MainDocumentService.getDocumentService("Topology");
	property string collectionId: "Services"
	property string selectedAgentId: ""
	property bool serviceOperationInProgress: false
	property string serviceOperationText: ""
	
	//for scrollBars
	property real originX: 0;
	property real originY: 0;
	//for scrollBars
	
	function setServiceOperation(inProgress, description){
		serviceOperationInProgress = inProgress
		serviceOperationText = description
	}

	function normalizeServiceStatus(status){
		switch (status){
			case "RUNNING":
			case ServiceStatus.s_Running: return ServiceStatus.s_Running
			case "NOT_RUNNING":
			case ServiceStatus.s_NotRunning: return ServiceStatus.s_NotRunning
			case "STARTING":
			case ServiceStatus.s_Starting: return ServiceStatus.s_Starting
			case "STOPPING":
			case ServiceStatus.s_Stopping: return ServiceStatus.s_Stopping
		}
		return ServiceStatus.s_Undefined
	}
	
	Component.onCompleted: {
		if (documentManager){
			documentManager.registerDocumentView("Service", serviceEditorComp);
			documentManager.registerDocumentDataController("Service", serviceDataControllerComp);
			documentManager.registerDocumentValidator("Service", serviceValidatorComp);
		}
		
		getTopologyRequestSender.send()
	}
	
	commandsControllerComp: Component {GqlBasedCommandsController {
			typeId: "Topology"
			
			onCommandsChanged: {
				let serviceSelected = scheme.selectedIndex >= 0 && scheme.selectedIndex < scheme.objectsModel.count
				setCommandVisible("Save", false)
				setCommandIsEnabled("New", true)
				setCommandIsEnabled("Edit", serviceSelected)
				setCommandIsEnabled("Remove", serviceSelected)
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
			documentTypeId: "Service"
			documentManager: topologyPage.documentManager
			removeDialogTitle: qsTr("Deleting a service")
			removeMessage: qsTr("Delete the selected service?")
			
			onCommandActivated: {
				if (commandId === "ZoomIn"){
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

			function onNew(){
				// Server app has an "Agents" document service → pick target agent.
				// Agent app has no Agents page → create service locally without clientid.
				if (MainDocumentService.getDocumentService("Agents")){
					ModalDialogManager.openDialog(newServiceAgentDialogComp, {})
					return
				}

				MainDocumentService.tryRegisterDocumentService("Agents", function(result){
					if (result){
						ModalDialogManager.openDialog(newServiceAgentDialogComp, {})
					}
					else if (topologyPage.documentManager){
						topologyPage.selectedAgentId = ""
						topologyPage.documentManager.insertNewDocument("Service", qsTr("New service"))
					}
				})
			}

			function onRemove(){
				ModalDialogManager.openDialog(topologyRemoveDialogComp, {})
			}

			function getHeaders(){
				return topologyPage.getHeaders();
			}

			Component {
				id: topologyRemoveDialogComp

				MessageDialog {
					width: Style.sizeHintM
					title: serviceCommandsDelegate.removeDialogTitle
					message: serviceCommandsDelegate.removeMessage
					onFinished: {
						if (buttonId == Enums.yes && scheme.selectedService !== ""){
							collectionDataProvider.removeElements([scheme.selectedService])
						}
					}
				}
			}

			Component {
				id: newServiceAgentDialogComp

				Dialog {
					id: newServiceAgentDialog
					width: Style.sizeHintM
					title: qsTr("Select target agent")

					property string chosenAgentId: ""

					Component.onCompleted: {
						addButton(Enums.apply, qsTr("Select"), false)
						addButton(Enums.cancel, qsTr("Cancel"), true)
					}

					onFinished: {
						if (buttonId === Enums.apply && chosenAgentId !== "" && topologyPage.documentManager){
							topologyPage.selectedAgentId = chosenAgentId
							topologyPage.documentManager.insertNewDocument("Service", qsTr("New service"))
						}
					}

					contentComp: Component {
						Item {
							width: newServiceAgentDialog.width
							height: Style.controlHeightM + 2 * Style.marginL

							ComboBoxGqlSimple {
								id: agentCombo
								anchors.centerIn: parent
								width: parent.width - 2 * Style.marginL
								height: Style.controlHeightM
								gqlCommandId: "AgentsList"
								fields: ["id", "computerName"]
								nameId: "computerName"

								onModelChanged: {
									if (model && model.getItemsCount() > 0){
										currentIndex = 0
									}
								}

								onCurrentIndexChanged: {
									if (currentIndex >= 0 && model && model.containsKey("id", currentIndex)){
										newServiceAgentDialog.chosenAgentId = model.getData("id", currentIndex)
										newServiceAgentDialog.setButtonEnabled(Enums.apply, true)
									}
									else{
										newServiceAgentDialog.chosenAgentId = ""
										newServiceAgentDialog.setButtonEnabled(Enums.apply, false)
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	SchemeView {
		id: scheme
		maximumObjectWidth: 360
		objectFontSize: Style.fontSizeXL
		objectSecondaryFontSize: Style.fontSizeS
		strokeSecondaryText: false
		
		anchors.left: parent.left;
		anchors.right: metaInfo.left;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;
		
		property string selectedService: ""
		
		onAutoFitChanged: {
			console.log("SchemeView onAutoFitChanged", autoFit);
			if (topologyPage.commandsController){
				topologyPage.commandsController.setToggled("AutoFit", scheme.autoFit);
			}
		}
		
		onObjectMoveFinished: {
			saveModelRequestSender.send()
		}

		onDeleteSignal: {
			if (scheme.selectedService !== ""){
				ModalDialogManager.openDialog(topologyRemoveDialogComp, {})
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
				topologyPage.selectedAgentId = item.m_agentId
				let status = topologyPage.normalizeServiceStatus(item.m_status);

				topologyPage.commandsController.setCommandIsEnabled("Start", status === ServiceStatus.s_NotRunning)
				topologyPage.commandsController.setCommandIsEnabled("Stop", status === ServiceStatus.s_Running)
				topologyPage.commandsController.setCommandIsEnabled("Edit", true)
				topologyPage.commandsController.setCommandIsEnabled("Remove", true)
				
				metaInfo.contentVisible = true;
				collectionDataProvider.getObjectMetaInfo(selectedService)
			}
			else{
				selectedService = ""
				topologyPage.selectedAgentId = ""
				topologyPage.commandsController.setCommandIsEnabled("Start", false)
				topologyPage.commandsController.setCommandIsEnabled("Stop", false)
				topologyPage.commandsController.setCommandIsEnabled("Edit", false)
				topologyPage.commandsController.setCommandIsEnabled("Remove", false)
				
				metaInfo.contentVisible = false;
			}
		}
		
		function goInside(){
			let documentManager = MainDocumentService.getDocumentService("Topology");
			if (documentManager){
				if(objectsModel.count > selectedIndex && selectedIndex >= 0){
					let item = scheme.objectsModel.get(scheme.selectedIndex).item
					let serviceId = item.m_id;
					let serviceName = item.m_mainText;
					
					documentManager.openDocument(serviceId, "Service", serviceName);
				}
			}
		}

		Shortcut {
			sequence: "Return"
			enabled: true
			onActivated: {
				scheme.goInside()
			}
		}
	}

	Rectangle {
		id: serviceOperationIndicator
		anchors.top: parent.top
		anchors.right: metaInfo.left
		anchors.topMargin: Style.marginXL
		anchors.rightMargin: Style.marginXL
		width: Math.min(serviceOperationRow.width + 2 * Style.marginL,
			Math.max(0, metaInfo.x - 2 * Style.marginXL))
		height: 36
		clip: true
		radius: height / 2
		color: Style.backgroundColor
		border.color: Style.lightBlueColor
		border.width: 1
		visible: topologyPage.serviceOperationInProgress
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
				width: Math.max(0, serviceOperationIndicator.width - 2 * Style.marginL - 18 - serviceOperationRow.spacing)
				text: topologyPage.serviceOperationText
				font.pixelSize: Style.fontSizeM
				elide: Text.ElideRight
			}
		}
	}

	MetaInfo {
		id: metaInfo;
		
		anchors.right: parent.right;
		anchors.top: parent.top;
		anchors.bottom: parent.bottom;
		
		width: 250;

		Component.onCompleted: {
			registerViewDelegate("ExternPaths", metaInfoViewExternPathsDelegateComp)
		}
	}
	
	CollectionRepresentation {
		id: collectionDataProvider
		collectionId: "Services"
		onObjectMetaInfoReceived: {
			metaInfo.elementMetaInfo = metaInfoData
		}
		
		function getHeaders(){
			return topologyPage.getHeaders();
		}
	}
	
	function getHeaders(){
		let headers = {}

		if (topologyPage.selectedAgentId !== ""){
			headers["clientid"] = topologyPage.selectedAgentId;
		}
		if (scheme.selectedService !== ""){
			headers["serviceid"] = scheme.selectedService;
		}
		
		return headers
	}

	Component {
		id: metaInfoViewExternPathsDelegateComp
		MetaInfoViewDelegateBase {
			id: metaInfoDelegate

			height: pathColumn.height

			Column {
				id: pathColumn
				width: metaInfoDelegate.width
				spacing: Style.spacingS
				Repeater {
					model: metaInfoDelegate.viewData.split('\n')
					delegate: Item {
						height: linkText.height
						width: parent.width
						Text {
							id: linkText
							width: parent.width
							font.family: Style.fontFamily
							font.pixelSize: Style.fontSizeS
							font.underline: true
							wrapMode: Text.WordWrap
							color: Style.lightBlueColor
							elide: Text.ElideRight
							text: modelData
						}

						MouseArea {
							anchors.fill: linkText
							hoverEnabled: true
							cursorShape: Qt.PointingHandCursor
							onClicked: {
								console.log("onClicked", modelData)
								Qt.openUrlExternally(modelData)
							}
						}
					}
				}
			}
		}
	}
	
	Component {
		id: serviceValidatorComp;
		
		ServiceValidator {}
	}
	
	Component {
		id: serviceEditorComp;
		
		ServiceEditorWrap {
			id: serviceEditor
			documentManager: topologyPage.documentManager
			
			Component.onCompleted: {
				clientId = topologyPage.selectedAgentId
			}
		}
	}
	
	Component {
		id: serviceDataControllerComp
		
		ServiceDocumentDataController {
			property string clientId: ""

			Component.onCompleted: {
				clientId = topologyPage.selectedAgentId
			}

			function getHeaders(){
				let headers = {}
				if (clientId !== ""){
					headers["clientid"] = clientId
				}
				return headers
			}
		}
	}
	
	SubscriptionClient {
		id: topologySubscriptionClient;
		gqlCommandId: "OnTopologyChanged";
		onMessageReceived: {
			getTopologyRequestSender.send()
		}
	}

	SubscriptionClient {
		id: servicesCollectionSubscriptionClient
		gqlCommandId: "OnServicesCollectionChanged"

		onMessageReceived: {
			getTopologyRequestSender.send()
		}
	}

	// Server-only: agent connect/disconnect must refresh offline markers (agentOnline / icons).
	// On the agent app this subscription is a no-op if the command is not published.
	SubscriptionClient {
		id: agentStatusSubscriptionClient
		gqlCommandId: "OnAgentStatusChanged"

		onMessageReceived: {
			getTopologyRequestSender.send()
		}
	}
	
	SubscriptionClient {
		id: subscriptionClient;
		gqlCommandId: "OnServiceStatusChanged";
		
		onMessageReceived: {
			let dataModel = data
			let serviceId = dataModel.getData("serviceid")
			let serviceStatus = topologyPage.normalizeServiceStatus(dataModel.getData(ServiceStatus.s_Key))
			let dependencyStatus
			
			let index = scheme.findModelIndex(serviceId);
			if (index < 0){
				return;
			}
			
			let item = scheme.objectsModel.get(index).item

			// Offline agent: keep Alert/Warning from GetTopology; do not paint stale live status.
			if (item.m_agentOnline === false){
				item.m_status = ServiceStatus.s_Undefined
				item.m_icon1 = "Icons/Alert"
				item.m_icon2 = "Icons/Warning"
				if (index === scheme.selectedIndex && topologyPage.commandsController){
					topologyPage.commandsController.setCommandIsEnabled("Start", false)
					topologyPage.commandsController.setCommandIsEnabled("Stop", false)
				}
				scheme.requestPaint()
				return
			}
			
			item.m_status = serviceStatus;
			if (serviceStatus === ServiceStatus.s_Running){
				item.m_icon1 = "Icons/Running";
			}
			else if (serviceStatus === ServiceStatus.s_NotRunning){
				item.m_icon1 = "Icons/Stopped";
			}
			else if (serviceStatus === ServiceStatus.s_Undefined){
				item.m_icon1 = "Icons/Alert";
			}

			if (index === scheme.selectedIndex && topologyPage.commandsController){
				topologyPage.commandsController.setCommandIsEnabled("Start", serviceStatus === ServiceStatus.s_NotRunning);
				topologyPage.commandsController.setCommandIsEnabled("Stop", serviceStatus === ServiceStatus.s_Running);
			}
			
			serviceStatus = item.m_status;
			
			let dependencyStatusModel = dataModel.getData(DependencyStatus.s_Key)
			if (!dependencyStatusModel){
				scheme.requestPaint()
				return
			}
			for (let i = 0; i < dependencyStatusModel.getItemsCount(); i++){
				serviceId = dependencyStatusModel.getData("id", i);
				index = scheme.findModelIndex(serviceId);
				dependencyStatus = topologyPage.normalizeServiceStatus(dependencyStatusModel.getData(DependencyStatus.s_Key, i))
				if (index < 0){
					continue
				}
				
				let serviceItem = scheme.objectsModel.get(index).item;
				if (serviceItem.m_agentOnline === false){
					serviceItem.m_icon2 = "Icons/Warning"
					continue
				}
				if (dependencyStatus === ServiceStatus.s_NotRunning){
					serviceItem.m_icon2 = "Icons/Error"
				}
				else if (dependencyStatus === ServiceStatus.s_Undefined){
					serviceItem.m_icon2 = "Icons/Warning"
				}
				else {
					serviceItem.m_icon2 = ""
				}
				
				if (serviceStatus !== ServiceStatus.s_Running){
					item.m_icon2 = ""
				}
				
				if (serviceId === scheme.selectedService){
					collectionDataProvider.getObjectMetaInfo(scheme.selectedService)
				}
			}

			scheme.requestPaint()
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
			SaveTopologyResponse {}
		}
	}
	
	GqlSdlRequestSender {
		id: getTopologyRequestSender;
		
		gqlCommandId: AgentinoTopologySdlCommandIds.s_getTopology;
		
		sdlObjectComp: Component {
			Topology {
				onFinished: {
					scheme.selectedIndex = -1
					scheme.selectedService = ""
					topologyPage.selectedAgentId = ""
					metaInfo.contentVisible = false
					scheme.objectsModel = m_services;
					scheme.requestPaint()
					if (topologyPage.commandsController){
						topologyPage.commandsController.setCommandIsEnabled("Start", false)
						topologyPage.commandsController.setCommandIsEnabled("Stop", false)
						topologyPage.commandsController.setCommandIsEnabled("Edit", false)
						topologyPage.commandsController.setCommandIsEnabled("Remove", false)
					}
				}
			}
		}
	}
}
