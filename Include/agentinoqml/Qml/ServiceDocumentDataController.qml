import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtguigql 1.0
import agentinoServicesSdl 1.0

GqlRequestDocumentDataController {
	id: requestDocumentDataController
	
	// Wire names (match AgentinoServicesSdlCommandIds) as string literals for JQML.
	gqlGetCommandId: "GetService";
	gqlUpdateCommandId: "UpdateService";
	gqlAddCommandId: "AddService";
	
	subscriptionCommandId: "OnServicesCollectionChanged";
	
	property var serviceData: documentModel;
	
	typeId: "ServiceInfo";
	documentName: serviceData ? serviceData.m_name: "";
	documentDescription: serviceData ? serviceData.m_description: "";
	
	// Prefer DataScope when parent injects one (QG1).
	property var dataScope: null

	function getHeaders(){
		if (dataScope !== null) {
			let scoped = {}
			if (dataScope.agentId)
				scoped["clientid"] = dataScope.agentId
			if (dataScope.serviceId)
				scoped["serviceid"] = dataScope.serviceId
			if (Object.keys(scoped).length > 0)
				return scoped
		}
		return {}
	}
	
	documentModelComp: Component {
		ServiceData {}
	}
}
