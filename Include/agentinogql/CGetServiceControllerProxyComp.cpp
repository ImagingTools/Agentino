#include <agentinogql/CGetServiceControllerProxyComp.h>


// ImtCore includes
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>


namespace agentinogql
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

						if (serviceRepresentationModelPtr->ContainsKey("InputConnections")){
							imtbase::CTreeItemModel* inputConnectionsModelPtr = serviceRepresentationModelPtr->GetTreeItemModel("InputConnections");
							if (inputConnectionsModelPtr != nullptr){
								if (inputConnectionsModelPtr->GetItemsCount() == 0){
									imtbase::CTreeItemModel* baseModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
									qDebug() << "baseModelPtr" << baseModelPtr->toJSON();
									if (baseModelPtr != nullptr){
										if (baseModelPtr->ContainsKey("data")){
											imtbase::CTreeItemModel* dataBaseModelPtr = baseModelPtr->GetTreeItemModel("data");
											if (dataBaseModelPtr){
												if (dataBaseModelPtr->ContainsKey("InputConnections")){
													imtbase::CTreeItemModel* baseInputConnectionsModelPtr =  dataBaseModelPtr->GetTreeItemModel("InputConnections");
													if (baseInputConnectionsModelPtr){
														serviceRepresentationModelPtr->SetExternTreeModel("InputConnections", baseInputConnectionsModelPtr);
													}
												}
											}
										}
									}
								}
							}
						}

						if (serviceRepresentationModelPtr->ContainsKey("OutputConnections")){
							imtbase::CTreeItemModel* outputConnectionsModelPtr = serviceRepresentationModelPtr->GetTreeItemModel("OutputConnections");
							if (outputConnectionsModelPtr != nullptr){
								if (outputConnectionsModelPtr->GetItemsCount() == 0){
									imtbase::CTreeItemModel* baseModelPtr = BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
									if (baseModelPtr != nullptr){
										if (baseModelPtr->ContainsKey("data")){
											imtbase::CTreeItemModel* dataBaseModelPtr = baseModelPtr->GetTreeItemModel("data");
											if (dataBaseModelPtr){
												if (dataBaseModelPtr->ContainsKey("OutputConnections")){
													imtbase::CTreeItemModel* baseoutputConnectionsModelPtr =  dataBaseModelPtr->GetTreeItemModel("OutputConnections");
													if (baseoutputConnectionsModelPtr){
														serviceRepresentationModelPtr->SetExternTreeModel("OutputConnections", baseoutputConnectionsModelPtr);
													}
												}
											}
										}
									}
								}
							}
						}

						rootModelPtr->SetExternTreeModel("data", serviceRepresentationModelPtr);

						return rootModelPtr.PopPtr();
					}
				}
			}
		}
	}

	return BaseClass::CreateInternalResponse(gqlRequest, errorMessage);
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

			imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
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

								inputConnectionsModelPtr->SetData("Id", elementId, index);
								inputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								inputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								inputConnectionsModelPtr->SetData("ServiceTypeName", connectionParamPtr->GetServiceTypeName(), index);
								inputConnectionsModelPtr->SetData("UsageId", connectionParamPtr->GetUsageId(), index);
								inputConnectionsModelPtr->SetData("ServiceName", serviceName, index);

								QUrl url = connectionParamPtr->GetUrl();
								inputConnectionsModelPtr->SetData("Host", url.host(), index);
								inputConnectionsModelPtr->SetData("Port", url.port(), index);

								imtbase::CTreeItemModel* externPortsModelPtr = inputConnectionsModelPtr->AddTreeModel("ExternPorts", index);

								QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
								for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
									int externIndex = externPortsModelPtr->InsertNewItem();

									externPortsModelPtr->SetData("Id", incomingConnection.id, externIndex);
									externPortsModelPtr->SetData("Name", incomingConnection.name, externIndex);
									externPortsModelPtr->SetData("Description", incomingConnection.description, externIndex);
									externPortsModelPtr->SetData("Host", incomingConnection.url.host(), externIndex);
									externPortsModelPtr->SetData("Port", incomingConnection.url.port(), externIndex);
								}
							}
						}
					}
				}
			}

			imtbase::IObjectCollection* dependantServiceCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
			if (dependantServiceCollectionPtr != nullptr){
				imtbase::ICollectionInfo::Ids elementIds = dependantServiceCollectionPtr->GetElementIds();
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
					if (dependantServiceCollectionPtr->GetObjectData(elementId, connectionDataPtr)){
						imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
						if (connectionLinkParamPtr != nullptr){
							// imtservice::IServiceConnectionParam::ConnectionType connectionType = connectionParamPtr->GetConnectionType();
							QByteArray dependantServiceConnectionId = connectionLinkParamPtr->GetDependantServiceConnectionId();
							imtbase::IObjectCollection::DataPtr connectionDependantDataPtr;
							QString connectionName = dependantServiceCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
							QString connectionDescription = dependantServiceCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
							QString serviceTypeName = connectionLinkParamPtr->GetServiceTypeName();

							int index = outputConnectionsModelPtr->InsertNewItem();
							outputConnectionsModelPtr->SetData("Id", elementId, index);
							outputConnectionsModelPtr->SetData("UsageId", connectionLinkParamPtr->GetUsageId(), index);
							outputConnectionsModelPtr->SetData("DependantConnectionId", dependantServiceConnectionId, index);
							outputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
							outputConnectionsModelPtr->SetData("ServiceName", serviceName, index);
							outputConnectionsModelPtr->SetData("ServiceTypeName", serviceTypeName, index);
							outputConnectionsModelPtr->SetData("Description", connectionDescription, index);

							QUrl url = connectionLinkParamPtr->GetDefaultUrl();

							outputConnectionsModelPtr->SetData("DisplayUrl", "", index);

							imtbase::CTreeItemModel* connectionsModelPtr = GetConnectionsModel(connectionLinkParamPtr->GetUsageId());
							outputConnectionsModelPtr->SetExternTreeModel("Elements", connectionsModelPtr, index);

							if (GetConnectionObjectData(dependantServiceConnectionId, connectionDependantDataPtr, connectionName, connectionDescription)
										&& connectionDependantDataPtr.IsValid()){
								imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDependantDataPtr.GetPtr());
								if (connectionParamPtr != nullptr){
									QUrl url = connectionParamPtr->GetUrl();
									outputConnectionsModelPtr->SetData("Url", url.toString(), index);
//									QString urlStr = serviceName + "@" + url.host() + ":" + QString::number(url.port());
								}
							}
						}
					}
				}
			}
		}
	}

	return serviceRepresentationModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CGetServiceControllerProxyComp::GetConnectionsModel(const QByteArray& connectionUsageId) const
{
	qDebug() << "GetConnectionsModel" << connectionUsageId;

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
									QString serviceName = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::IObjectCollection::EIT_NAME).toString();
									// Get Connections
									imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
									if (connectionCollectionPtr != nullptr){
										imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
										for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
												imtbase::IObjectCollection::DataPtr connectionDataPtr;
											if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
												imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
												if (connectionParamPtr != nullptr && connectionParamPtr->GetUsageId() == connectionUsageId){
													if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
														int index = result->InsertNewItem();

														QUrl url = connectionParamPtr->GetUrl();
														url.setHost("localhost");

														QString urlStr = serviceName + "@" + "localhost" + ":" + QString::number(url.port());

														result->SetData("Id", connectionElementId, index);
														result->SetData("Name", urlStr, index);
														result->SetData("Url", url.toString(), index);

														QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();

														for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
															index = result->InsertNewItem();

															QString urlStr = serviceName + "@" + incomingConnection.url.host() + ":" + QString::number(incomingConnection.url.port());

															result->SetData("Id", incomingConnection.id, index);
															result->SetData("Name", urlStr, index);
															result->SetData("Url", incomingConnection.url.toString(), index);
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


bool CGetServiceControllerProxyComp::GetConnectionObjectData(
			const imtbase::IObjectCollection::Id& connectionId,
			imtbase::IObjectCollection::DataPtr& connectionDataPtr,
			QString& connectionName,
			QString &connectionDescription) const
{
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
									imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
									if (connectionCollectionPtr != nullptr){
										imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
										for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
											if (connectionElementId == connectionId){
												connectionName = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();
												connectionDescription = connectionCollectionPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

												return connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr);
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

	return false;
}


} // namespace agentinogql


