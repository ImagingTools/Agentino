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

		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					if (serviceCollectionPtr->GetElementIds().contains(objectId)) {
						istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
						imtbase::CTreeItemModel* serviceRepresentationModelPtr = GetRepresentationModelFromServiceInfo(*serviceCollectionPtr, objectId);

						rootModelPtr->SetExternTreeModel("data", serviceRepresentationModelPtr);

						return rootModelPtr.PopPtr();
					}
				}
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


imtbase::CTreeItemModel* CGetServiceControllerProxyComp::GetRepresentationModelFromServiceInfo(const imtbase::IObjectCollection& serviceCollection, const QByteArray& serviceId) const
{
	istd::TDelPtr<imtbase::CTreeItemModel> serviceRepresentationModelPtr(new imtbase::CTreeItemModel());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (serviceCollection.GetObjectData(serviceId, dataPtr)){
		agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QString serviceName = serviceCollection.GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toString();
			QString description = serviceCollection.GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
			QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');
			bool isAutoStart = serviceInfoPtr->IsAutoStart();

			serviceRepresentationModelPtr->SetData("Id", serviceId);
			serviceRepresentationModelPtr->SetData("Name", serviceName);
			serviceRepresentationModelPtr->SetData("Description", description);
			serviceRepresentationModelPtr->SetData("Path", servicePath);
			serviceRepresentationModelPtr->SetData("SettingsPath", settingsPath);
			serviceRepresentationModelPtr->SetData("Arguments", arguments);
			serviceRepresentationModelPtr->SetData("IsAutoStart", isAutoStart);

			imtbase::CTreeItemModel* inputConnectionsModelPtr = serviceRepresentationModelPtr->AddTreeModel("InputConnections");
			imtbase::CTreeItemModel* outputConnectionsModelPtr = serviceRepresentationModelPtr->AddTreeModel("OutputConnections");

			imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetConnectionCollection();
			if (connectionCollectionPtr != nullptr){
				imtbase::ICollectionInfo::Ids elementIds = connectionCollectionPtr->GetElementIds();
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
					if (connectionCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
						imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
						if (connectionParamPtr != nullptr){
							imtservice::IServiceConnectionParam::ConnectionType connectionType = connectionParamPtr->GetConnectionType();
							if (connectionType == imtservice::IServiceConnectionParam::CT_INPUT){
								int index = inputConnectionsModelPtr->InsertNewItem();
								QString connectionName = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
								QString connectionDescription = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

								inputConnectionsModelPtr->SetData("Id", connectionName, index);
								inputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								inputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								inputConnectionsModelPtr->SetData("ServiceName", connectionParamPtr->GetServiceName(), index);

								QUrl url = connectionParamPtr->GetUrl();
								inputConnectionsModelPtr->SetData("Host", url.host(), index);
								inputConnectionsModelPtr->SetData("Port", url.port(), index);

								imtbase::CTreeItemModel* externPortsModelPtr = inputConnectionsModelPtr->AddTreeModel("ExternPorts", index);

								QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
								for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
									int externIndex = externPortsModelPtr->InsertNewItem();

									externPortsModelPtr->SetData("Host", incomingConnection.url.host(), externIndex);
									externPortsModelPtr->SetData("Port", incomingConnection.url.port(), externIndex);
									externPortsModelPtr->SetData("Description", incomingConnection.description, externIndex);
								}
							}
							else if (connectionType == imtservice::IServiceConnectionParam::CT_OUTPUT){
								int index = outputConnectionsModelPtr->InsertNewItem();

								QString connectionName = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
								QString connectionDescription = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

								QUrl url = connectionParamPtr->GetUrl();

								outputConnectionsModelPtr->SetData("Id", connectionName, index);
								outputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								outputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								outputConnectionsModelPtr->SetData("ServiceName", connectionParamPtr->GetServiceName(), index);

								QString urlStr = connectionParamPtr->GetServiceName() + "@" + url.host() + ":" + QString::number(url.port());
								outputConnectionsModelPtr->SetData("Url", urlStr, index);

								imtbase::CTreeItemModel* connectionsModelPtr = GetConnectionsModel(connectionName.toUtf8());
								outputConnectionsModelPtr->SetExternTreeModel("Elements", connectionsModelPtr, index);
							}
						}
					}
				}
			}
		}
	}

	return serviceRepresentationModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CGetServiceControllerProxyComp::GetConnectionsModel(const QByteArray& connectionId) const
{
	qDebug() << "GetConnectionsModel" << connectionId;

	istd::TDelPtr<imtbase::CTreeItemModel> result(new imtbase::CTreeItemModel());

	if (m_agentCollectionCompPtr.IsValid()){
		imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			imtbase::IObjectCollection::DataPtr agentDataPtr;
			if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
				agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
				if (agentInfoPtr != nullptr){
					// Get Services
					imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
					if (serviceCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
						for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
							imtbase::IObjectCollection::DataPtr serviceDataPtr;
							if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
								agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
								if (serviceInfoPtr != nullptr){

									// Get Connections
									imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetConnectionCollection();
									if (connectionCollectionPtr != nullptr){
										imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
										for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
											qDebug() << "connectionElementId" << connectionElementId;

											if (connectionElementId == connectionId){
												imtbase::IObjectCollection::DataPtr connectionDataPtr;
												if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
													imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
													if (connectionParamPtr != nullptr){
														if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
															QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
															for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
																int index = result->InsertNewItem();

																QString urlStr = connectionParamPtr->GetServiceName() + "@" + incomingConnection.url.host() + ":" + QString::number(incomingConnection.url.port());

																result->SetData("Id", urlStr, index);
																result->SetData("Name", urlStr, index);
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return result.PopPtr();
}


} // namespace agentgql


