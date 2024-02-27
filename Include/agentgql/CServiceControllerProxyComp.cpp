#include <agentgql/CServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>


namespace agentgql
{


imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	imtbase::CTreeItemModel* resultModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);

	if (!resultModelPtr->ContainsKey("errors")){
		if (m_agentCollectionCompPtr.IsValid()){
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

			istd::TDelPtr<agentinodata::IServiceInfo> serviceInfoPtr = GetServiceInfoFromRepresentationModel(itemModel);
			if (serviceInfoPtr.IsValid()){
				imtbase::IObjectCollection::DataPtr dataPtr;
				if (m_agentCollectionCompPtr->GetObjectData(agentId, dataPtr)){
					agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
					if (agentInfoPtr != nullptr){
						imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
						if (serviceCollectionPtr != nullptr){
							imtbase::IObjectCollection::DataPtr serviceDataPtr;
							if (serviceCollectionPtr->GetObjectData(objectId, serviceDataPtr)){
								serviceCollectionPtr->SetObjectData(objectId, *serviceInfoPtr.PopPtr());
							}
							else{
								serviceCollectionPtr->InsertNewObject("ServiceInfo", serviceName, serviceDescription, serviceInfoPtr.PopPtr(), objectId);
							}
						}
					}

					m_agentCollectionCompPtr->SetObjectData(agentId, *agentInfoPtr);
				}
			}
		}
	}

	return resultModelPtr;
}


agentinodata::IServiceInfo* CServiceControllerProxyComp::GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const
{
	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CServiceInfo);

	QByteArray serviceId;
	if (representationModel.ContainsKey("Id")){
		serviceId = representationModel.GetData("Id").toByteArray();
	}

	QString serviceName;
	if (representationModel.ContainsKey("Name")){
		serviceName = representationModel.GetData("Name").toString();
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

	imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetConnectionCollection();

	if (representationModel.ContainsKey("InputConnections")){
		imtbase::CTreeItemModel* inputConnectionsModelPtr = representationModel.GetTreeItemModel("InputConnections");
		if (inputConnectionsModelPtr != nullptr){
			for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray id = inputConnectionsModelPtr->GetData("Id", i).toByteArray();
				QString connectionName = inputConnectionsModelPtr->GetData("ConnectionName", i).toString();
				QString description = inputConnectionsModelPtr->GetData("Description", i).toString();
				QString host = inputConnectionsModelPtr->GetData("Host", i).toString();
				int port = inputConnectionsModelPtr->GetData("Port", i).toInt();

				QUrl connectionUrl;
				connectionUrl.setHost(host);
				connectionUrl.setPort(port);

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
												 serviceName.toUtf8(),
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

				connectionCollectionPtr->InsertNewObject("ConnectionInfo", connectionName, description, urlConnectionParamPtr.PopPtr(), id);
			}
		}
	}

	if (representationModel.ContainsKey("OutputConnections")){
		imtbase::CTreeItemModel* outputConnectionsModelPtr = representationModel.GetTreeItemModel("OutputConnections");
		if (outputConnectionsModelPtr != nullptr){
			for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray id = outputConnectionsModelPtr->GetData("Id", i).toByteArray();
				QString connectionName = outputConnectionsModelPtr->GetData("ConnectionName", i).toString();
				QString description = outputConnectionsModelPtr->GetData("Description", i).toString();

				QString urlStr = outputConnectionsModelPtr->GetData("Url", i).toString();

				QString serviceName = urlStr.split('@')[0];
				QStringList data = urlStr.split('@')[1].split(':');
				QUrl url;
				if (data.size() == 2){
					url.setHost(data[0]);
					url.setPort(data[1].toInt());
				}

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
												 serviceName.toUtf8(),
												 imtservice::IServiceConnectionParam::CT_OUTPUT,
												 url)
											 );

				connectionCollectionPtr->InsertNewObject("ConnectionInfo", connectionName, description, urlConnectionParamPtr.PopPtr(), id);
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


