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
	property var documentManager
	
	commandsDelegateComp: Component {ViewCommandsDelegateBase {
			view: serviceEditor;
		}
	}
	
	commandsControllerComp: Component {GqlBasedCommandsController {
			typeId: "Service";
			function getHeaders(){
				return serviceEditor.getHeaders();
			}
		}
	}
	
	signal serviceStatusChanged(string serviceId, string status)
	onServiceStatusChanged: {
		console.log("ServiceEditor onServiceStatusChanged", serviceId, status)
		if (!serviceEditor.serviceData){
			return
		}

		if (serviceId == serviceEditor.serviceData.m_id){
			serviceEditor.serviceRunning = status === "RUNNING"
		}
	}
	
	function getHeaders(){
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
					serviceDocumentDataController.documentId = serviceId
					serviceDocumentDataController.updateDocumentModel()
				}
			}
		}
	}
}
