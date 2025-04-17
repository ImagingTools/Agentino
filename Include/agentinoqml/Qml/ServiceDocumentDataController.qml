import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtguigql 1.0
import agentinoServicesSdl 1.0

GqlRequestDocumentDataController {
	id: requestDocumentDataController
	
	gqlGetCommandId: AgentinoServicesSdlCommandIds.s_getService;
	gqlUpdateCommandId: AgentinoServicesSdlCommandIds.s_updateService;
	gqlAddCommandId: AgentinoServicesSdlCommandIds.s_addService;
	
	subscriptionCommandId: "OnServicesCollectionChanged";
	
	property ServiceData serviceData: documentModel;
	
	// typeId: "Service";
	documentName: serviceData ? serviceData.m_name: "";
	documentDescription: serviceData ? serviceData.m_description: "";
	
	function getHeaders(){
		return {}
	}
	
	documentModelComp: Component {
		ServiceData {}
	}

	onHasRemoteChangesChanged: {
		if (hasRemoteChanges){
			// updateDocumentModel();
		}
	}
}
