import QtQuick 2.12
import Acf 1.0
import imtguigql 1.0
import agentinoServicesSdl 1.0

GqlRequestDocumentDataController {
	id: requestDocumentDataController
	
	gqlGetCommandId: AgentinoServicesSdlCommandIds.s_getService;
	gqlUpdateCommandId: AgentinoServicesSdlCommandIds.s_updateService;
	gqlAddCommandId: AgentinoServicesSdlCommandIds.s_addService;
	
	subscriptionCommandId: "OnServicesCollectionChanged";
	
	function getHeaders(){
		return {}
	}
	
	documentModelComp: Component {
		ServiceData {}
	}
	
	payloadModel: ServiceData {
		onFinished: {
			requestDocumentDataController.documentModel = this;
		}
	}
	
	onHasRemoteChangesChanged: {
		if (hasRemoteChanges){
			// updateDocumentModel();
		}
	}
}
