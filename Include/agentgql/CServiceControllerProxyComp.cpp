#include <agentgql/CServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (m_agentCollectionCompPtr.IsValid()){

		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");

		QByteArray itemData;
		if (inputParamPtr != nullptr){
			itemData = inputParamPtr->GetFieldArgumentValue("Item").toByteArray();
		}

		imtbase::CTreeItemModel itemModel;
		if (!itemModel.CreateFromJson(itemData)){
			return nullptr;
		}

		istd::TDelPtr<agentinodata::IServiceInfo> serviceInfoPtr = GetServiceInfoFromRepresentationModel(itemModel);

		QByteArray agentId;

		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
			}
		}

		imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();

		for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
			imtbase::IObjectCollection::DataPtr dataPtr;
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
}


agentinodata::IServiceInfo* CServiceControllerProxyComp::GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const
{
//	istd::TDelPtr<agentinodata::IServiceInfo> serviceInfoPtr
//	if (itemModel.ContainsKey("Name")){
//		name = itemModel.GetData("Name").toString();
//	}

//	if (name.isEmpty()){
//		errorMessage = QT_TR_NOOP("Service name can't be empty");
//		return nullptr;
//	}

//		serviceInfoPtr->SetServiceName(name.toUtf8());

//		if (itemModel.ContainsKey("Description")){
//			description = itemModel.GetData("Description").toString();
//			serviceInfoPtr->SetServiceDescription(description.toUtf8());
//		}

//	if (itemModel.ContainsKey("Path")){
//		QByteArray path = itemModel.GetData("Path").toByteArray();
//		serviceInfoPtr->SetServicePath(path);
//	}

//	if (itemModel.ContainsKey("SettingsPath")){
//		QByteArray settingsPath = itemModel.GetData("SettingsPath").toByteArray();
//		serviceInfoPtr->SetServiceSettingsPath(settingsPath);
//	}

//	if (itemModel.ContainsKey("Arguments")){
//		QByteArray arguments = itemModel.GetData("Arguments").toByteArray();
//		serviceInfoPtr->SetServiceArguments(arguments.split(' '));
//	}

//	if (itemModel.ContainsKey("IsAutoStart")){
//		bool isAutoStart = itemModel.GetData("IsAutoStart").toBool();
//		serviceInfoPtr->SetIsAutoStart(isAutoStart);
//	}

//	if (itemModel.ContainsKey("InputConnections")){
//		imtbase::CTreeItemModel* inputConnectionsModelPtr = itemModel.GetTreeItemModel("InputConnections");
//		if (inputConnectionsModelPtr != nullptr){

//			for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
//				QString connectionType = inputConnectionsModelPtr->GetData("ConnectionType").toString();
//				QString description = inputConnectionsModelPtr->GetData("Description").toString();
//				QString host = inputConnectionsModelPtr->GetData("Host").toString();
//				int port = inputConnectionsModelPtr->GetData("Port").toInt();

//				QUrl connectionUrl;
//				connectionUrl.setHost(host);
//				connectionUrl.setPort(port);

//				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
//				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
//												 name.toUtf8(),
//												 imtservice::IServiceConnectionParam::CT_INPUT,
//												 connectionUrl)
//											 );

//				if (inputConnectionsModelPtr->ContainsKey("ExternPorts")){
//					imtbase::CTreeItemModel* externPortsModelPtr = inputConnectionsModelPtr->GetTreeItemModel("ExternPorts");
//					if (externPortsModelPtr != nullptr){
//						imtbase::CTreeItemModel* elementsModelPtr = externPortsModelPtr->GetTreeItemModel("Elements");
//						if (elementsModelPtr != nullptr){
//							for (int i = 0; i < elementsModelPtr->GetItemsCount(); i++){
//								QByteArray objectId = elementsModelPtr->GetData("Id").toByteArray();
//								QString description = elementsModelPtr->GetData("Description").toString();
//								QString externHost = elementsModelPtr->GetData("Host").toString();
//								int externPort = elementsModelPtr->GetData("Port").toInt();

//								imtservice::IServiceConnectionParam::IncomingConnectionParam externConnection;

//								externConnection.description = description;

//								QUrl url;
//								url.setPort(externPort);
//								url.setHost(externHost);

//								externConnection.description = description;
//								externConnection.url = url;

//								urlConnectionParamPtr->AddExternConnection(externConnection);
//							}
//						}
//					}
//				}

//				if (m_connectionCollectionCompPtr.IsValid()){
//					if (m_connectionCollectionCompPtr->GetElementIds().contains(objectId)){
//						m_connectionCollectionCompPtr->SetObjectData(objectId, *urlConnectionParamPtr.PopPtr());
//					}
//					else{
//						m_connectionCollectionCompPtr->InsertNewObject("ConnectionInfo", connectionType, description, urlConnectionParamPtr.PopPtr());
//					}
//				}
//			}
//		}
//	}

//	if (itemModel.ContainsKey("OutputConnections")){
//		imtbase::CTreeItemModel* outputConnectionsModelPtr = itemModel.GetTreeItemModel("OutputConnections");
//		if (outputConnectionsModelPtr != nullptr){

//		}
//	}
	return nullptr;
}


} // namespace agentgql


