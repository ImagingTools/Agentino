#include <agentinogql/CServerServiceCollectionControllerComp.h>


// ImtCore includes
#include <iqt/iqt.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/agentinodata.h>


namespace agentinogql
{


// protected methods

QStringList CServerServiceCollectionControllerComp::GetConnectionInfoAboutDependOnService(const QByteArray& connectionId) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return QStringList();
	}

	QStringList result;

	imtbase::ICollectionInfo::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

			QString agentName = m_objectCollectionCompPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();

			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();

			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}

					QString serviceTypeId = serviceInfoPtr->GetServiceTypeId();

					imtbase::IObjectCollection::DataPtr inputConnectionDataPtr;
					imtservice::CUrlConnectionParam* inputConnectionParamPtr = nullptr;
					imtbase::IObjectCollection* inputConnectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (inputConnectionCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids incommingConnectionIds = inputConnectionCollectionPtr->GetElementIds();
						if (!incommingConnectionIds.isEmpty()){
							imtbase::ICollectionInfo::Id incommingConnectionId = incommingConnectionIds[0];

							if (inputConnectionCollectionPtr->GetObjectData(incommingConnectionId, inputConnectionDataPtr)){
								inputConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(inputConnectionDataPtr.GetPtr());
							}
						}
					}

					imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
					if (dependantConnectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = dependantConnectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						imtbase::IObjectCollection::DataPtr connectionDataPtr;
						imtservice::CUrlConnectionLinkParam* urlConnectionParamPtr = nullptr;
						if (dependantConnectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
							urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
						}

						if (urlConnectionParamPtr == nullptr){
							continue;
						}

						QByteArray dependantServiceConnectionId = urlConnectionParamPtr->GetDependantServiceConnectionId();
						if (dependantServiceConnectionId == connectionId){
							if (inputConnectionParamPtr != nullptr){
								const int httpPort = inputConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
								const int wsPort = inputConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
								result << QString(serviceTypeId + "@" + agentName + " (http: " + QString::number(httpPort) + "; " + "ws: " + QString::number(wsPort) + ")");
							}
						}
					}
				}
			}
		}
	}

	return result;
}


QStringList CServerServiceCollectionControllerComp::GetConnectionInfoAboutServiceDepends(const QByteArray& /*connectionId*/) const
{
	return QStringList();
}


istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> CServerServiceCollectionControllerComp::GetConnectionInfo(const QByteArray& connectionId) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return nullptr;
	}

	imtbase::ICollectionInfo::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

			QString agentName = m_objectCollectionCompPtr->GetElementInfo(elementId, imtbase::IObjectCollection::EIT_NAME).toString();

			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();

			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}

					imtbase::IObjectCollection* inputConnectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (inputConnectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = inputConnectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){\
						if (connectionElementId == connectionId){
							imtbase::IObjectCollection::DataPtr connectionDataPtr;
							if (inputConnectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
								istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> retVal;
								retVal.MoveCastedPtr(connectionDataPtr.GetPtr()->CloneMe());

								return retVal;
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}


sdl::imtbase::ImtCollection::CGetElementMetaInfoPayload CServerServiceCollectionControllerComp::OnGetElementMetaInfo(
			const sdl::imtbase::ImtCollection::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	sdl::imtbase::ImtCollection::CGetElementMetaInfoPayload response;
	response.Version_1_0.Emplace();

	sdl::imtbase::ImtCollection::GetElementMetaInfoRequestArguments arguments = getElementMetaInfoRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0){
		Q_ASSERT(false);
		return response;
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->elementId){
		serviceId = *arguments.input.Version_1_0->elementId;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::imtbase::ImtCollection::CElementMetaInfo::V1_0 elementMetaInfo;
	QList<sdl::imtbase::ImtBaseTypes::CParameter::V1_0> infoParams;

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		return response;
	}

	QByteArray languageId;
	const imtgql::IGqlContext* gqlContextPtr = gqlRequest.GetRequestContext();
	if (gqlContextPtr != nullptr){
		languageId = gqlContextPtr->GetLanguageId();
	}

	imtbase::IObjectCollection* dependantCollectionPtr = nullptr;
	imtbase::IObjectCollection* inputCollectionPtr = nullptr;

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
		agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			dependantCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
			inputCollectionPtr = serviceInfoPtr->GetInputConnections();
		}
	}

	if (inputCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids connectionIds = inputCollectionPtr->GetElementIds();
		if (!connectionIds.isEmpty()){
			QString incomingConnectionStr = QT_TR_NOOP("Incoming Connections");

			if (m_translationManagerCompPtr.IsValid()){
				incomingConnectionStr = iqt::GetTranslation(
							m_translationManagerCompPtr.GetPtr(),
							incomingConnectionStr.toUtf8(),
							languageId,
							"agentinogql::CServerServiceCollectionControllerComp");
			}

			sdl::imtbase::ImtBaseTypes::CParameter::V1_0 parameter;
			parameter.id = QByteArrayLiteral("IncomingConnections");
			parameter.typeId = parameter.id;
			parameter.name = incomingConnectionStr;

			QString incomingConnectionsData;
			QStringList connectionInfos;
			for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (inputCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
					imtservice::CUrlConnectionParam* urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
					if (urlConnectionParamPtr != nullptr){
						const QString host = urlConnectionParamPtr->GetHost();
						const int httpPort = urlConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
						const int wsPort = urlConnectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);

						incomingConnectionsData += host + QString(" (http: %1; ws: %2)").arg(QString::number(httpPort), QString::number(wsPort)) + "\n";
						connectionInfos << GetConnectionInfoAboutDependOnService(connectionId);

						imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = urlConnectionParamPtr->GetIncomingConnections();
						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							connectionInfos << GetConnectionInfoAboutDependOnService(incomingConnection.id);
						}
					}
				}
			}

			parameter.data = incomingConnectionsData;
			infoParams << parameter;

			if (!connectionInfos.isEmpty()){
				sdl::imtbase::ImtBaseTypes::CParameter::V1_0 dependantParameter;
				dependantParameter.id = QByteArrayLiteral("DependantConnections");
				dependantParameter.typeId = dependantParameter.id;

				QString dependantServicesStr = QT_TR_NOOP("Dependant services on port");
				if (m_translationManagerCompPtr.IsValid()){
					dependantServicesStr = iqt::GetTranslation(
								m_translationManagerCompPtr.GetPtr(),
								dependantServicesStr.toUtf8(),
								languageId,
								"agentinogql::CServerServiceCollectionControllerComp");
				}

				dependantParameter.name = dependantServicesStr;
				dependantParameter.data = connectionInfos.join('\n');

				infoParams << dependantParameter;
			}
		}
	}

	if (dependantCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids connectionIds = dependantCollectionPtr->GetElementIds();
		if (!connectionIds.isEmpty()){
			sdl::imtbase::ImtBaseTypes::CParameter::V1_0 serviceDependsParameter;
			serviceDependsParameter.id = QByteArrayLiteral("IncomingConnections");
			serviceDependsParameter.typeId = serviceDependsParameter.id;

			QString serviceDependsOnStr = QT_TR_NOOP("Service depends on");
			if (m_translationManagerCompPtr.IsValid()){
				serviceDependsOnStr = iqt::GetTranslation(
							m_translationManagerCompPtr.GetPtr(),
							serviceDependsOnStr.toUtf8(),
							languageId,
							"agentinogql::CServerServiceCollectionControllerComp");
			}

			serviceDependsParameter.name = serviceDependsOnStr;
			QString serviceDependsData;
			for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (dependantCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
					imtservice::CUrlConnectionLinkParam* urlConnectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
					if (urlConnectionLinkParamPtr != nullptr){
						QByteArray dependantServiceConnectionId = urlConnectionLinkParamPtr->GetDependantServiceConnectionId();

						istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> inputConnectionPtr = GetConnectionInfo(dependantServiceConnectionId);
						if (inputConnectionPtr.IsValid()){
							const QString host = inputConnectionPtr->GetHost();
							const int httpPort = inputConnectionPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
							const int wsPort = inputConnectionPtr->GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
							serviceDependsData += host + " (http: " + QString::number(httpPort)+ ";" + "ws: " + QString::number(wsPort) + ")\n";
						}
					}
				}
			}
			serviceDependsParameter.data = serviceDependsData;
			infoParams << serviceDependsParameter;
		}
	}

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus =  m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequiredServices = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceId);
	QString dependantStatus = agentinodata::IServiceCompositeInfo::ToString(stateOfRequiredServices);
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING && stateOfRequiredServices != agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		sdl::imtbase::ImtBaseTypes::CParameter::V1_0 informationParameter;
		informationParameter.id = QByteArrayLiteral("Information");
		informationParameter.typeId = informationParameter.id;

		QString serviceDependsOnStr = QT_TR_NOOP("Information");

		if (m_translationManagerCompPtr.IsValid()){
			serviceDependsOnStr = iqt::GetTranslation(
						m_translationManagerCompPtr.GetPtr(),
						serviceDependsOnStr.toUtf8(),
						languageId,
						"agentinogql::CServerServiceCollectionControllerComp");
		}
		informationParameter.name = serviceDependsOnStr;

		QStringList dependantStatusInfo = GetDependantStatusInfo(serviceId);
		informationParameter.data = dependantStatusInfo.join("; ");

		infoParams << informationParameter;
	}

	elementMetaInfo.infoParams.Emplace();
	elementMetaInfo.infoParams->FromList(infoParams);
	response.Version_1_0->elementMetaInfo = elementMetaInfo;

	return response;
}


// reimplemented (imtgql::CObjectCollectionControllerCompBase)

bool CServerServiceCollectionControllerComp::SetupGqlItem(
		const imtgql::CGqlRequest& gqlRequest,
		imtbase::CTreeItemModel& model,
		int itemIndex,
		const QByteArray& collectionId,
		QString& errorMessage) const
{
	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		Q_ASSERT(0);

		return false;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty()){
		agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (serviceCollectionPtr->GetObjectData(collectionId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr){
			for (const QByteArray& informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "typeId"){
					elementInformation = serviceCollectionPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "id"){
					elementInformation = collectionId;
				}
				else if(informationId == "name"){
					elementInformation = serviceInfoPtr->GetServiceName();
				}
				else if(informationId == "description"){
					elementInformation = serviceInfoPtr->GetServiceDescription();
				}
				else if(informationId == "path"){
					elementInformation = serviceInfoPtr->GetServicePath();
				}
				else if(informationId == "startScript"){
					elementInformation = serviceInfoPtr->GetStartScriptPath();
				}
				else if(informationId == "stopScript"){
					elementInformation = serviceInfoPtr->GetStopScriptPath();
				}
				else if(informationId == "settingsPath"){
					elementInformation = serviceInfoPtr->GetServiceSettingsPath();
				}
				else if(informationId == "arguments"){
					elementInformation = serviceInfoPtr->GetServiceArguments().join(' ');
				}
				else if(informationId == "type"){
					agentinodata::IServiceInfo::SettingsType settingsType  = serviceInfoPtr->GetSettingsType();
					switch (settingsType){
					case agentinodata::IServiceInfo::ST_PLUGIN:
						elementInformation = "ACF";
						break;
					default:
						elementInformation = "None";
						break;
					}
				}
				else if(informationId == "status" || informationId == "statusName"){
					agentinodata::IServiceStatusInfo::ServiceStatus status = agentinodata::IServiceStatusInfo::SS_UNDEFINED;

					if (m_serviceStatusCollectionCompPtr.IsValid()){
						imtbase::IObjectCollection::DataPtr serviceStatusDataPtr;
						if (m_serviceStatusCollectionCompPtr->GetObjectData(collectionId, serviceStatusDataPtr)){
							agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(serviceStatusDataPtr.GetPtr());
							if (serviceStatusInfoPtr != nullptr){
								status = serviceStatusInfoPtr->GetServiceStatus();
							}
						}
					}

					agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(status);
					if (informationId == "status"){
						elementInformation = processStateEnum.id;
					}
					else{
						elementInformation = processStateEnum.name;
					}
				}
				else if(informationId == "isAutoStart"){
					elementInformation = serviceInfoPtr->IsAutoStart();
				}
				else if(informationId == "tracingLevel"){
					elementInformation = serviceInfoPtr->GetTracingLevel();
				}
				else if(informationId == "version"){
					elementInformation = serviceInfoPtr->GetServiceVersion();
				}
				else if(informationId == "dependencyStatus"){
					agentinodata::IServiceCompositeInfo::StateOfRequiredServices state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(collectionId);
					elementInformation = agentinodata::IServiceCompositeInfo::ToString(state);
				}
				else if(informationId == "dependantStatusInfo"){
					elementInformation = GetDependantStatusInfo(collectionId).join("; ");
				}

				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);
			}

			return retVal;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel* CServerServiceCollectionControllerComp::ListObjects(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	const imtgql::CGqlParamObject& inputParams = gqlRequest.GetParams();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = rootModelPtr->AddTreeModel("data");
	imtbase::CTreeItemModel* itemsModel = dataModel->AddTreeModel("items");
	imtbase::CTreeItemModel* notificationModel = dataModel->AddTreeModel("notification");

	const imtgql::CGqlParamObject* viewParamsGql = nullptr;
	const imtgql::CGqlParamObject* inputObject = inputParams.GetParamArgumentObjectPtr("input");
	if (inputObject != nullptr){
		viewParamsGql = inputObject->GetParamArgumentObjectPtr("viewParams");
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return nullptr;
	}

	iprm::CParamsSet filterParams;

	int offset = 0;
	int count = -1;

	if (viewParamsGql != nullptr){
		offset = viewParamsGql->GetParamArgumentValue("offset").toInt();
		count = viewParamsGql->GetParamArgumentValue("count").toInt();
		PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
	}

	int elementsCount = serviceCollectionPtr->GetElementsCount(&filterParams);

	int pagesCount = std::ceil(elementsCount / (double)count);
	if (pagesCount <= 0){
		pagesCount = 1;
	}

	notificationModel->SetData("pagesCount", pagesCount);
	notificationModel->SetData("totalCount", elementsCount);

	imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds(offset, count, &filterParams);

	for (imtbase::ICollectionInfo::Id& id: ids){
		int itemIndex = itemsModel->InsertNewItem();
		if (itemIndex >= 0){
			if (!SetupGqlItem(gqlRequest, *itemsModel, itemIndex, id, errorMessage)){
				SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

				return nullptr;
			}
		}
	}

	return rootModelPtr.PopPtr();
}


// private methods

QStringList CServerServiceCollectionControllerComp::GetDependantStatusInfo(const QByteArray& serviceId) const
{
	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		Q_ASSERT(0);

		return QStringList();
	}

	QStringList retVal;
	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::IAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::IAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				// Get Services
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
					if (serviceElementIds.contains(serviceId)){
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
							agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
							if (serviceInfoPtr == nullptr){
								continue;
							}

							// Get Connections
							imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
							if (connectionCollectionPtr != nullptr){
								imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
								for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
									imtbase::IObjectCollection::DataPtr connectionDataPtr;
									if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
										imtservice::CUrlConnectionLinkParam* connectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
										if (connectionLinkParamPtr != nullptr){
											QByteArray dependantServiceId =  m_serviceCompositeInfoCompPtr->GetServiceId(connectionLinkParamPtr->GetDependantServiceConnectionId());
											agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceCompositeInfoCompPtr->GetServiceStatus(dependantServiceId);
											QString serviceName =
														m_serviceCompositeInfoCompPtr->GetServiceName(dependantServiceId) + "@" +
														m_serviceCompositeInfoCompPtr->GetServiceAgentName(dependantServiceId);
											QString info;

											if (serviceStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED){
												info = QT_TR_NOOP("Service status of %1 undefined");
												info = info.arg(serviceName);
											}
											else if (serviceStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
												info = QT_TR_NOOP("Service %1 not running");
												info = info.arg(serviceName);
											}

											if (!info.isEmpty() && !retVal.contains(info)){
												retVal << info;
											}
										}
									}
								}
							}
						}

						break;
					}
				}
			}
		}
	}

	return retVal;
}


} // namespace agentinogql


