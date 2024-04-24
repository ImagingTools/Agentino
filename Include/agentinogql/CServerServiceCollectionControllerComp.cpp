#include <agentinogql/CServerServiceCollectionControllerComp.h>


// ImtCore includes
#include <imtbase/imtbase.h>
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

QUrl CServerServiceCollectionControllerComp::GetUrlByDependantId(const QByteArray& dependantId) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return QUrl();
	}

	imtbase::ICollectionInfo::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

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

					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						imtbase::IObjectCollection::DataPtr connectionDataPtr;
						imtservice::CUrlConnectionParam* urlConnectionParamPtr = nullptr;
						if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
							urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
						}

						if (urlConnectionParamPtr == nullptr){
							continue;
						}

						if (connectionElementId == dependantId){
							return urlConnectionParamPtr->GetUrl();
						}

						imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = urlConnectionParamPtr->GetIncomingConnections();
						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							if (incomingConnection.id == dependantId){
								return incomingConnection.url;
							}
						}
					}
				}
			}
		}
	}

	return QUrl();
}


QStringList CServerServiceCollectionControllerComp::GetConnectionInfoAboutDependOnService(
			const QUrl& url,
			const QByteArray& connectionId) const
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

					QString serviceName = serviceCollectionPtr->GetElementInfo(serviceElementId, imtbase::IObjectCollection::EIT_NAME).toString();

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
							result << QString(serviceName + "@" + agentName + " (Port: " + QString::number(url.port()) + ")");
						}
					}
				}
			}
		}
	}

	return result;
}


QStringList CServerServiceCollectionControllerComp::GetConnectionInfoAboutServiceDepends(const QByteArray& connectionId) const
{
	return QStringList();
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

	QByteArray agentId;
	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		const imtgql::CGqlObject* addition =gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
		}
	}

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
			QByteArray serviceId;
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = serviceCollectionPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "Id"){
					serviceId = collectionId;
					elementInformation = serviceId;
				}
				else if(informationId == "Name"){
					elementInformation = serviceCollectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_NAME).toString();
				}
				else if(informationId == "Description"){
					elementInformation = serviceCollectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
				}
				else if(informationId == "Path"){
					elementInformation = serviceInfoPtr->GetServicePath();
				}
				else if(informationId == "SettingsPath"){
					elementInformation = serviceInfoPtr->GetServiceSettingsPath();
				}
				else if(informationId == "Arguments"){
					elementInformation = serviceInfoPtr->GetServiceArguments().join(' ');
				}
				else if(informationId == "Type"){
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
				else if(informationId == "Status" || informationId == "StatusName"){
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
					if (informationId == "Status"){
						elementInformation = processStateEnum.id;
					}
					else{
						elementInformation = processStateEnum.name;
					}
				}
				else if(informationId == "IsAutoStart"){
					elementInformation = serviceInfoPtr->IsAutoStart();
				}
				else if(informationId == "Version"){
					elementInformation = serviceInfoPtr->GetServiceVersion();
				}
				else if(informationId == "DependencyStatus"){
					agentinodata::IServiceCompositeInfo::StateOfRequiredServices state = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(collectionId);
					elementInformation = agentinodata::IServiceCompositeInfo::ToString(state);
				}
				else if(informationId == "DependantStatusInfo"){
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
	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = nullptr;
	imtbase::CTreeItemModel* itemsModel = nullptr;
	imtbase::CTreeItemModel* notificationModel = nullptr;

	if (!errorMessage.isEmpty()){
		imtbase::CTreeItemModel* errorsItemModel = rootModelPtr->AddTreeModel("errors");
		errorsItemModel->SetData("message", errorMessage);
	}
	else{
		dataModel = new imtbase::CTreeItemModel();
		itemsModel = new imtbase::CTreeItemModel();
		notificationModel = new imtbase::CTreeItemModel();

		QByteArray agentId;
		const imtgql::CGqlObject* viewParamsGql = nullptr;
		if (inputParams.size() > 0){
			viewParamsGql = inputParams.at(0).GetFieldArgumentObjectPtr("viewParams");

			const imtgql::CGqlObject* addition = inputParams.at(0).GetFieldArgumentObjectPtr("addition");
			if (addition != nullptr) {
				agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
			}
		}

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

		int offset = 0, count = -1;

		if (viewParamsGql != nullptr){
			offset = viewParamsGql->GetFieldArgumentValue("Offset").toInt();
			count = viewParamsGql->GetFieldArgumentValue("Count").toInt();
			PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
		}

		int elementsCount = serviceCollectionPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

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
		itemsModel->SetIsArray(true);
		dataModel->SetExternTreeModel("items", itemsModel);
		dataModel->SetExternTreeModel("notification", notificationModel);
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CServerServiceCollectionControllerComp::GetMetaInfo(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		Q_ASSERT(0);

		return nullptr;
	}

	QByteArray agentId;
	QByteArray serviceId;
	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		serviceId = gqlInputParamPtr->GetFieldArgumentValue("Id").toByteArray();

		const imtgql::CGqlObject* addition =gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
		}
	}

	imtbase::IObjectCollection* serviceCollectionPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		}
	}

	if (serviceCollectionPtr == nullptr){
		return nullptr;
	}

	QByteArray languageId;
	imtgql::IGqlContext* gqlContextPtr = gqlRequest.GetRequestContext();
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

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel);
	imtbase::CTreeItemModel* dataModelPtr = rootModelPtr->AddTreeModel("data");

	if (inputCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids connectionIds = inputCollectionPtr->GetElementIds();
		if (!connectionIds.isEmpty()){
			int index = dataModelPtr->InsertNewItem();

			QString incomingConnectionStr = QT_TR_NOOP("Incoming Connections");

			if (m_translationManagerCompPtr.IsValid()){
				incomingConnectionStr = imtbase::GetTranslation(
						m_translationManagerCompPtr.GetPtr(),
						incomingConnectionStr.toUtf8(),
						languageId,
						"agentinogql::CServerServiceCollectionControllerComp"
					);
			}

			dataModelPtr->SetData("Name", incomingConnectionStr, index);

			QStringList connectionInfos;

			imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);
			for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (inputCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
					imtservice::CUrlConnectionParam* urlConnectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
					if (urlConnectionParamPtr != nullptr){
						int childIndex = contentModelPtr->InsertNewItem();
						QUrl url = urlConnectionParamPtr->GetUrl();
						QByteArray usageId = urlConnectionParamPtr->GetUsageId();

						contentModelPtr->SetData("Value", usageId + " (Port: " + QString::number(url.port()) + ")", childIndex);

						connectionInfos << GetConnectionInfoAboutDependOnService(url, connectionId);

						imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = urlConnectionParamPtr->GetIncomingConnections();
						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							connectionInfos << GetConnectionInfoAboutDependOnService(incomingConnection.url, incomingConnection.id);
						}
					}
				}
			}

			if (!connectionInfos.isEmpty()){
				index = dataModelPtr->InsertNewItem();

				QString dependantServicesStr = QT_TR_NOOP("Dependant services on port");

				if (m_translationManagerCompPtr.IsValid()){
					dependantServicesStr = imtbase::GetTranslation(
							m_translationManagerCompPtr.GetPtr(),
							dependantServicesStr.toUtf8(),
							languageId,
							"agentinogql::CServerServiceCollectionControllerComp"
						);
				}
				dataModelPtr->SetData("Name", dependantServicesStr, index);

				imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);

				for (const QString& info : connectionInfos){
					int childIndex = contentModelPtr->InsertNewItem();

					contentModelPtr->SetData("Value", info, childIndex);
				}
			}
		}
	}

	if (dependantCollectionPtr != nullptr){
		imtbase::ICollectionInfo::Ids connectionIds = dependantCollectionPtr->GetElementIds();
		if (!connectionIds.isEmpty()){
			int index = dataModelPtr->InsertNewItem();

			QString serviceDependsOnStr = QT_TR_NOOP("Service depends on");

			if (m_translationManagerCompPtr.IsValid()){
				serviceDependsOnStr = imtbase::GetTranslation(
						m_translationManagerCompPtr.GetPtr(),
						serviceDependsOnStr.toUtf8(),
						languageId,
						"agentinogql::CServerServiceCollectionControllerComp"
					);
			}
			dataModelPtr->SetData(	"Name", serviceDependsOnStr, index);

			imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);

			for (const imtbase::ICollectionInfo::Id& connectionId : connectionIds){
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (dependantCollectionPtr->GetObjectData(connectionId, connectionDataPtr)){
					imtservice::CUrlConnectionLinkParam* urlConnectionLinkParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
					if (urlConnectionLinkParamPtr != nullptr){
						int childIndex = contentModelPtr->InsertNewItem();
						QByteArray usageId = urlConnectionLinkParamPtr->GetUsageId();
						QByteArray dependantServiceConnectionId = urlConnectionLinkParamPtr->GetDependantServiceConnectionId();
						QUrl url = GetUrlByDependantId(dependantServiceConnectionId);

						contentModelPtr->SetData("Value", usageId + " (Port: " + QString::number(url.port()) + ")", childIndex);
					}
				}
			}
		}
	}

	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus =  m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	agentinodata::IServiceCompositeInfo::StateOfRequiredServices stateOfRequiredServices = m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceId);
	QString dependantStatus = agentinodata::IServiceCompositeInfo::ToString(stateOfRequiredServices);
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING && stateOfRequiredServices != agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		int index = dataModelPtr->InsertNewItem();

		QString serviceDependsOnStr = QT_TR_NOOP("Information");

		if (m_translationManagerCompPtr.IsValid()){
			serviceDependsOnStr = imtbase::GetTranslation(
				m_translationManagerCompPtr.GetPtr(),
				serviceDependsOnStr.toUtf8(),
				languageId,
				"agentinogql::CServerServiceCollectionControllerComp"
				);
		}
		dataModelPtr->SetData(	"Name", serviceDependsOnStr, index);

		imtbase::CTreeItemModel* contentModelPtr = dataModelPtr->AddTreeModel("Children", index);
		int childIndex = contentModelPtr->InsertNewItem();
		QStringList dependantStatusInfo = GetDependantStatusInfo(serviceId);

		contentModelPtr->SetData("Value", dependantStatusInfo.join("; "), childIndex);

		if (dependantStatus == "NotAllRunning"){
			contentModelPtr->SetData("Icon", "Icons/Error", childIndex);
		}
		else {
			contentModelPtr->SetData("Icon", "Icons/Warning", childIndex);
		}
	}

	return rootModelPtr.PopPtr();
}


//private methods implemented
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
											QString serviceName = m_serviceCompositeInfoCompPtr->GetServiceName(dependantServiceId)
																  + "@"
																  + m_serviceCompositeInfoCompPtr->GetServiceAgentName(dependantServiceId);
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


