#include <agentgql/CServiceControllerProxyComp.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	imtbase::CTreeItemModel* resultModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);

	if (!resultModelPtr->ContainsKey("errors")){
		if (m_serviceManagerCompPtr.IsValid()){
			const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");

			QByteArray itemData;
			QByteArray agentId;
			QByteArray objectId;
			if (inputParamPtr != nullptr){
				objectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();
				itemData = inputParamPtr->GetFieldArgumentValue("Item").toByteArray();

				const imtgql::CGqlObject* addition = inputParamPtr->GetFieldArgumentObjectPtr("addition");
				if (addition != nullptr) {
					agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
				}
			}

			imtbase::CTreeItemModel itemModel;
			if (gqlRequest.GetCommandId() == "ServiceAdd"){
				imtbase::CTreeItemModel* dataModelPtr = resultModelPtr->GetTreeItemModel("item");
				if (dataModelPtr != nullptr){
					itemModel.CopyFrom(*dataModelPtr);
				}
			}
			else{
				if (!itemModel.CreateFromJson(itemData)){
					return nullptr;
				}
			}

			QString serviceName;
			if (itemModel.ContainsKey("Name")){
				serviceName = itemModel.GetData("Name").toString();
			}

			QString serviceDescription;
			if (itemModel.ContainsKey("Description")){
				serviceDescription = itemModel.GetData("Description").toString();
			}

			istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(GetServiceInfoFromRepresentationModel(itemModel));
			serviceInfoPtr->SetObjectUuid(objectId);

			if (m_serviceManagerCompPtr->ServiceExists(agentId, objectId)){
				m_serviceManagerCompPtr->SetService(agentId, objectId, *serviceInfoPtr.PopPtr());
			}
			else{
				m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr.PopPtr(), objectId, serviceName, serviceDescription);
			}
		}
	}

	return resultModelPtr;
}


agentinodata::IServiceInfo* CServiceControllerProxyComp::GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const
{
	istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

	QByteArray serviceId;
	if (representationModel.ContainsKey("Id")){
		serviceId = representationModel.GetData("Id").toByteArray();
	}

	QString serviceName;
	if (representationModel.ContainsKey("Name")){
		serviceName = representationModel.GetData("Name").toString();
	}

	QString serviceTypeName;
	if (representationModel.ContainsKey("TypeName")){
		serviceTypeName = representationModel.GetData("TypeName").toString();
	}

	if (representationModel.ContainsKey("Path")){
		QByteArray path = representationModel.GetData("Path").toByteArray();
		serviceInfoPtr->SetServicePath(path);
	}

	if (representationModel.ContainsKey("SettingsPath")){
		QByteArray settingsPath = representationModel.GetData("SettingsPath").toByteArray();
		serviceInfoPtr->SetServiceSettingsPath(settingsPath);
	}

	if (representationModel.ContainsKey("Arguments")){
		QByteArray arguments = representationModel.GetData("Arguments").toByteArray();
		serviceInfoPtr->SetServiceArguments(arguments.split(' '));
	}

	if (representationModel.ContainsKey("IsAutoStart")){
		bool isAutoStart = representationModel.GetData("IsAutoStart").toBool();
		serviceInfoPtr->SetIsAutoStart(isAutoStart);
	}

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();

	if (representationModel.ContainsKey("InputConnections")){
		imtbase::CTreeItemModel* inputConnectionsModelPtr = representationModel.GetTreeItemModel("InputConnections");
		if (inputConnectionsModelPtr != nullptr){
			for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray id = inputConnectionsModelPtr->GetData("Id", i).toByteArray();
				QString usageId = inputConnectionsModelPtr->GetData("UsageId", i).toString();
				QString serviceTypeName = inputConnectionsModelPtr->GetData("ServiceTypeName", i).toString();
				QString description = inputConnectionsModelPtr->GetData("Description", i).toString();
				QString host = inputConnectionsModelPtr->GetData("Host", i).toString();
				int port = inputConnectionsModelPtr->GetData("Port", i).toInt();

				QUrl connectionUrl;
				connectionUrl.setHost(host);
				connectionUrl.setPort(port);

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
												 serviceTypeName.toUtf8(),
												 usageId.toUtf8(),
												 imtservice::IServiceConnectionParam::CT_INPUT,
												 connectionUrl)
											 );

				if (inputConnectionsModelPtr->ContainsKey("ExternPorts", i)){
					imtbase::CTreeItemModel* externPortsModelPtr = inputConnectionsModelPtr->GetTreeItemModel("ExternPorts", i);
					if (externPortsModelPtr != nullptr){
						imtbase::CTreeItemModel* elementsModelPtr = externPortsModelPtr;
						if (elementsModelPtr != nullptr){
							for (int j = 0; j < elementsModelPtr->GetItemsCount(); j++){
								QByteArray objectId = elementsModelPtr->GetData("Id", j).toByteArray();
								QString externDescription = elementsModelPtr->GetData("Description", j).toString();
								QString externHost = elementsModelPtr->GetData("Host", j).toString();
								int externPort = elementsModelPtr->GetData("Port", j).toInt();

								imtservice::IServiceConnectionParam::IncomingConnectionParam externConnection;

								QUrl url;
								url.setPort(externPort);
								url.setHost(externHost);

								externConnection.description = externDescription;
								externConnection.url = url;

								urlConnectionParamPtr->AddExternConnection(externConnection);
							}
						}
					}
				}

				connectionCollectionPtr->InsertNewObject("ConnectionInfo", serviceTypeName, description, urlConnectionParamPtr.PopPtr(), id);
			}
		}
	}

	imtbase::IObjectCollection* dependantServiceConnectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();

	if (representationModel.ContainsKey("OutputConnections")){
		imtbase::CTreeItemModel* outputConnectionsModelPtr = representationModel.GetTreeItemModel("OutputConnections");
		if (outputConnectionsModelPtr != nullptr && dependantServiceConnectionCollectionPtr != nullptr){
			for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray id = outputConnectionsModelPtr->GetData("Id", i).toByteArray();

				QString serviceTypeName = outputConnectionsModelPtr->GetData("ServiceTypeName", i).toString();
				QString usageId = outputConnectionsModelPtr->GetData("UsageId", i).toString();
				QString description = outputConnectionsModelPtr->GetData("Description", i).toString();
				QString dependantServiceConnectionId = outputConnectionsModelPtr->GetData("dependantConnectionId", i).toString();

				istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
				urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam(
													serviceTypeName.toUtf8(),
													usageId.toUtf8(),
													dependantServiceConnectionId.toUtf8()));

				dependantServiceConnectionCollectionPtr->InsertNewObject("ConnectionLink", serviceTypeName, description, urlConnectionLinkParamPtr.PopPtr(), id);
			}
		}
	}

	return serviceInfoPtr.PopPtr();
}


imtbase::CTreeItemModel* CServiceControllerProxyComp::GetRepresentationModelFromServiceInfo(const agentinodata::IServiceInfo& serviceInfo) const
{
	return nullptr;
}


} // namespace agentgql


