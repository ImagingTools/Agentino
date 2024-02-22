#include <agentgql/CGetServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>

// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>


namespace agentgql
{


imtbase::CTreeItemModel* CGetServiceControllerProxyComp::CreateInternalResponse(
		const imtgql::CGqlRequest& gqlRequest,
		QString& errorMessage) const
{
	if (m_agentCollectionCompPtr.IsValid()){
		const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");

		QByteArray agentId;
		QByteArray objectId;
		if (inputParamPtr != nullptr){
			objectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();

			const imtgql::CGqlObject* addition = inputParamPtr->GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
			}
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
}


agentinodata::IServiceInfo* CGetServiceControllerProxyComp::GetServiceInfoFromRepresentationModel(const imtbase::CTreeItemModel& representationModel) const
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
				QByteArray id = inputConnectionsModelPtr->GetData("Id").toByteArray();
				QString connectionName = inputConnectionsModelPtr->GetData("ConnectionName").toString();
				QString description = inputConnectionsModelPtr->GetData("Description").toString();
				QString host = inputConnectionsModelPtr->GetData("Host").toString();
				int port = inputConnectionsModelPtr->GetData("Port").toInt();

				QUrl connectionUrl;
				connectionUrl.setHost(host);
				connectionUrl.setPort(port);

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
												 serviceName.toUtf8(),
												 imtservice::IServiceConnectionParam::CT_INPUT,
												 connectionUrl)
											 );

				if (inputConnectionsModelPtr->ContainsKey("ExternPorts")){
					imtbase::CTreeItemModel* externPortsModelPtr = inputConnectionsModelPtr->GetTreeItemModel("ExternPorts");
					if (externPortsModelPtr != nullptr){
						imtbase::CTreeItemModel* elementsModelPtr = externPortsModelPtr->GetTreeItemModel("Elements");
						if (elementsModelPtr != nullptr){
							for (int i = 0; i < elementsModelPtr->GetItemsCount(); i++){
								QByteArray objectId = elementsModelPtr->GetData("Id", i).toByteArray();
								QString description = elementsModelPtr->GetData("Description", i).toString();
								QString externHost = elementsModelPtr->GetData("Host", i).toString();
								int externPort = elementsModelPtr->GetData("Port", i).toInt();

								imtservice::IServiceConnectionParam::IncomingConnectionParam externConnection;

								externConnection.description = description;

								QUrl url;
								url.setPort(externPort);
								url.setHost(externHost);

								externConnection.description = description;
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
				QString serviceName = outputConnectionsModelPtr->GetData("ServiceName", i).toString();
				QString description = outputConnectionsModelPtr->GetData("Description", i).toString();
				QString urlStr = outputConnectionsModelPtr->GetData("Url", i).toString();

				QUrl url = QUrl(urlStr);

				istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
				urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
												 serviceName.toUtf8(),
												 imtservice::IServiceConnectionParam::CT_INPUT,
												 url)
											 );
			}
		}
	}

	return serviceInfoPtr.PopPtr();
}


imtbase::CTreeItemModel* CGetServiceControllerProxyComp::GetRepresentationModelFromServiceInfo(const agentinodata::IServiceInfo& serviceInfo) const
{
	return nullptr;
}


} // namespace agentgql


