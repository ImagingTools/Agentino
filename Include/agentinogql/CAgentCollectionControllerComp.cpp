#include <agentinogql/CAgentCollectionControllerComp.h>


// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>
#include <istd/TDelPtr.h>
#include <iprm/CParamsSet.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/CAgentStatusInfo.h>


namespace agentinogql
{


// reimplemented (icomp::CComponentBase)

void CAgentCollectionControllerComp::OnComponentCreated()
{
	m_timer.setInterval(500);
	m_timer.setSingleShot(true);
	connect(&m_timer, &QTimer::timeout, this, &CAgentCollectionControllerComp::OnTimeout);
}


// reimplemented (imtgql::CObjectCollectionControllerCompBase)

bool CAgentCollectionControllerComp::SetupGqlItem(
			const imtgql::CGqlRequest& gqlRequest,
			imtbase::CTreeItemModel& model,
			int itemIndex,
			const QByteArray& collectionId,
			QString& errorMessage) const
{
	qDebug("Test log qDebug()");
	SendErrorMessage(0, "Test log", "CObjectCollectionControllerCompBase");

	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty() && m_objectCollectionCompPtr.IsValid()){
		agentinodata::CIdentifiableAgentInfo* agentPtr = nullptr;
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(collectionId, agentDataPtr)){
			agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentDataPtr.GetPtr());
		}

		if (agentPtr != nullptr){
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "Id"){
					elementInformation = agentPtr->GetObjectUuid();
				}
				else if(informationId == "Name"){
					QString name = m_objectCollectionCompPtr->GetElementInfo(collectionId, imtbase::ICollectionInfo::EIT_NAME).toString();
					elementInformation = name;
				}
				else if(informationId == "Description"){
					QString description = m_objectCollectionCompPtr->GetElementInfo(collectionId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();

					elementInformation = description;
				}
				else if(informationId == "ComputerName"){
					elementInformation = agentPtr->GetComputerName();
				}
				else if(informationId == "Services"){
					imtbase::IObjectCollection* serviceCollectionPtr = agentPtr->GetServiceCollection();
					if (serviceCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
						QStringList result;
						for (const imtbase::ICollectionInfo::Id& id : ids){
							QString name = serviceCollectionPtr->GetElementInfo(id, imtbase::ICollectionInfo::EIT_NAME).toString();
							result << name;
						}

						elementInformation = result.join(';');
					}
				}
				else if(informationId == "Status"){
					elementInformation = "Disconnected";

					if (m_agentStatusCollectionCompPtr.IsValid()){
						imtbase::IObjectCollection::DataPtr dataPtr;
						if (m_agentStatusCollectionCompPtr->GetObjectData(collectionId, dataPtr)){
							const agentinodata::CAgentStatusInfo* agentStatusInfoPtr = dynamic_cast<const agentinodata::CAgentStatusInfo*>(dataPtr.GetPtr());
							if (agentStatusInfoPtr != nullptr){
								agentinodata::IAgentStatusInfo::AgentStatus status = agentStatusInfoPtr->GetAgentStatus();
								switch(status){
								case agentinodata::IAgentStatusInfo::AS_CONNECTED:
									elementInformation = "Connected";
									break;
								case agentinodata::IAgentStatusInfo::AS_DISCONNECTED:
									elementInformation = "Disconnected";
									break;
								default:
									elementInformation = "Undefined";
								}
							}
						}
					}
				}
				else if(informationId == "LastConnection"){
					QDateTime lastConnection = agentPtr->GetLastConnection();
					if (!lastConnection.isNull()){
						lastConnection.setTimeSpec(Qt::UTC);

						elementInformation = lastConnection.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
					}
				}
				else if(informationId == "Version"){
					elementInformation = agentPtr->GetVersion();
				}

				if (elementInformation.isNull()){
					elementInformation = "";
				}

				retVal = retVal && model.SetData(informationId, elementInformation, itemIndex);
			}

			return true;
		}

	}
	errorMessage = "Unable to get object data from object collection";

	return false;
}


imtbase::CTreeItemModel *CAgentCollectionControllerComp::ListObjects(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return nullptr;
	}

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

		const imtgql::CGqlObject* viewParamsGql = nullptr;
		if (inputParams.size() > 0){
			viewParamsGql = inputParams.at(0).GetFieldArgumentObjectPtr("viewParams");
		}

		iprm::CParamsSet filterParams;

		int offset = 0, count = -1;

		if (viewParamsGql != nullptr){
			offset = viewParamsGql->GetFieldArgumentValue("Offset").toInt();
			count = viewParamsGql->GetFieldArgumentValue("Count").toInt();
			PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
		}

		int elementsCount = m_objectCollectionCompPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

		imtbase::ICollectionInfo::Ids ids = m_objectCollectionCompPtr->GetElementIds(offset, count, &filterParams);

		for (const imtbase::ICollectionInfo::Id& id: ids){
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


imtbase::CTreeItemModel* CAgentCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Internal error").toUtf8();
		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();

	QByteArray objectId = GetObjectIdFromInputParams(gqlRequest.GetParams());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(objectId, dataPtr)){
		const agentinodata::CAgentInfo* agentPtr = dynamic_cast<const agentinodata::CAgentInfo*>(dataPtr.GetPtr());
		if (agentPtr != nullptr){
			QString name = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();
			QString description = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_DESCRIPTION).toString();
			QDateTime lastConnection = agentPtr->GetLastConnection();

			dataModel->SetData("Id", objectId);
			dataModel->SetData("Name", name);
			dataModel->SetData("Description", description);
			dataModel->SetData("LastConnection", lastConnection.toString("dd.MM.yyyy"));
			dataModel->SetData("TracingLevel", agentPtr->GetTracingLevel());
		}
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


istd::IChangeable* CAgentCollectionControllerComp::CreateObject(
			const QList<imtgql::CGqlObject>& inputParams,
			QByteArray& objectId,
			QString& name,
			QString& description,
			QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	objectId = GetObjectIdFromInputParams(inputParams);
	if (objectId.isEmpty()){
		objectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}

	QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
	if (!itemData.isEmpty()){
		istd::TDelPtr<agentinodata::IAgentInfo> agentInstancePtr = m_agentFactCompPtr.CreateInstance();
		if (agentInstancePtr == nullptr){
			return nullptr;
		}

		agentinodata::CIdentifiableAgentInfo* agentPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(agentInstancePtr.GetPtr());
		if (agentPtr == nullptr){
			errorMessage = QT_TR_NOOP("Unable to get an service info!");
			return nullptr;
		}

		imtbase::CTreeItemModel itemModel;
		itemModel.CreateFromJson(itemData);

		agentPtr->SetObjectUuid(objectId);

		if (itemModel.ContainsKey("Name")){
			name = itemModel.GetData("Name").toString();
		}

		if (name.isEmpty()){
			errorMessage = QT_TR_NOOP("Service name can't be empty");
			return nullptr;
		}

		if (itemModel.ContainsKey("Description")){
			description = itemModel.GetData("Description").toString();
		}

		if (itemModel.ContainsKey("ComputerName")){
			QString computerName = itemModel.GetData("ComputerName").toString();
			agentPtr->SetComputerName(computerName);
		}

		if (itemModel.ContainsKey("Version")){
			QString version = itemModel.GetData("Version").toString();
			agentPtr->SetVersion(version);
		}

		if (itemModel.ContainsKey("TracingLevel")){
			int tracingLevel = itemModel.GetData("TracingLevel").toInt();
			agentPtr->SetTracingLevel(tracingLevel);
		}

		return agentInstancePtr.PopPtr();
	}

	errorMessage = QString("Can not create agent: '%1'").arg(QString(objectId));

	return nullptr;
}


imtbase::CTreeItemModel* CAgentCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();

	QByteArray agentId = GetObjectIdFromInputParams(inputParams);

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
		agentinodata::CIdentifiableAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(dataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			agentInfoPtr->SetLastConnection(QDateTime::currentDateTimeUtc());

			if (inputParams.count() > 0){
				QByteArray item = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
				QJsonDocument itemDoc = QJsonDocument::fromJson(item);
				if (itemDoc.object().contains("Version")){
					QString version = itemDoc.object().value("Version").toString();
					agentInfoPtr->SetVersion(version);
				}
			}

			if (!m_objectCollectionCompPtr->SetObjectData(agentId, *agentInfoPtr)){
				qDebug() << QString("Unable to set data to the collection object with ID: '%1'.").arg(qPrintable(agentId));
			}
		}
	}
	else{
		QString name;
		QString description;

		istd::TDelPtr<istd::IChangeable> objectPtr = BaseClass::CreateObject(gqlRequest, agentId, name, description, errorMessage);
		if (objectPtr == nullptr){
			return nullptr;
		}

		agentinodata::CIdentifiableAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CIdentifiableAgentInfo*>(objectPtr.GetPtr());
		if (agentInfoPtr == nullptr){
			return nullptr;
		}

		agentInfoPtr->SetLastConnection(QDateTime::currentDateTimeUtc());

		m_objectCollectionCompPtr->InsertNewObject("DocumentInfo", name, description, agentInfoPtr, agentId);
	}

	m_connectedAgents.append(agentId);
	m_timer.start();

	return nullptr;
}


imtbase::CTreeItemModel* CAgentCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_agentFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();
	if (inputParams.size() == 0){
		errorMessage = QString("Unable to update model. Error: GraphQL input params is invalid.");
		SendErrorMessage(0, errorMessage);

		return nullptr;
	}

	imtbase::CTreeItemModel* retVal = nullptr;
	QByteArray objectId = GetObjectIdFromInputParams(inputParams);

	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(objectId, agentDataPtr)){
		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
		if (agentInfoPtr != nullptr){
			QDateTime dateTime = QDateTime::currentDateTimeUtc();
			agentInfoPtr->SetLastConnection(dateTime);

			QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
			QString name;

			if (!itemData.isEmpty()){
				imtbase::CTreeItemModel itemModel;
				if (!itemModel.CreateFromJson(itemData)){
					errorMessage = QString("Unable to create model from JSON. Error: invalid JSON: '%1'.").arg(itemData);
					SendErrorMessage(0, errorMessage);

					return nullptr;
				}

				if (itemModel.ContainsKey("TracingLevel")){
					int tracingLevel = itemModel.GetData("TracingLevel").toInt();
					agentInfoPtr->SetTracingLevel(tracingLevel);
				}

				if (itemModel.ContainsKey("Name")){
					name = itemModel.GetData("Name").toString();
					m_objectCollectionCompPtr->SetElementName(objectId, name);
				}

				if (itemModel.ContainsKey("Description")){
					QString description = itemModel.GetData("Description").toString();
					m_objectCollectionCompPtr->SetElementDescription(objectId, description);
				}
			}

			if (!m_objectCollectionCompPtr->SetObjectData(objectId, *agentInfoPtr)){
				qDebug() << QString("Unable to set data to the collection object with ID: '%1'.").arg(qPrintable(objectId));
			}
			else{
				retVal = new imtbase::CTreeItemModel();
				imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();
				imtbase::CTreeItemModel* notificationModel = new imtbase::CTreeItemModel();
				notificationModel->SetData("Id", objectId);
				notificationModel->SetData("Name", name);
				dataModel->SetExternTreeModel("updatedNotification", notificationModel);
				retVal->SetExternTreeModel("data", dataModel);
			}
		}
	}

	return retVal;
}


void CAgentCollectionControllerComp::UpdateAgentService(
			const QByteArray& agentId,
			const QByteArray& serviceId) const
{
	if (!m_requestHandlerCompPtr.IsValid()){
		return;
	}

	if (!m_objectCollectionCompPtr.IsValid()){
		return;
	}

	agentinodata::CAgentInfo* agentInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
		agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
	}

	if (agentInfoPtr == nullptr){
		return;
	}

	imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
	if (serviceCollectionPtr == nullptr){
		return;
	}

	agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		return;
	}

	istd::TDelPtr<iprm::CParamsSet> paramsPtr = new iprm::CParamsSet;

	iser::ISerializable* objectCollectionPtr = dynamic_cast<iser::ISerializable*>(m_objectCollectionCompPtr.GetPtr());
	if (objectCollectionPtr != nullptr){
		paramsPtr->SetEditableParameter("AgentCollection", objectCollectionPtr);
	}

	imtbase::CTreeItemModel serviceRepresentationModel;
	bool ok = m_serviceInfoRepresentationController.GetRepresentationFromDataModel(*serviceInfoPtr, serviceRepresentationModel, paramsPtr.GetPtr());
	if (!ok){
		return;
	}

	QString serviceName = serviceCollectionPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toString();
	QString description = serviceCollectionPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();

	serviceRepresentationModel.SetData("Name", serviceName);
	serviceRepresentationModel.SetData("Description", description);

	imtgql::CGqlRequest request(imtgql::CGqlRequest::RT_MUTATION, "ServiceUpdate");
	imtgql::CGqlObject inputObject("input");
	inputObject.InsertField(QByteArray("Id"), QVariant(serviceId));
	inputObject.InsertField(QByteArray("Item"), QVariant(serviceRepresentationModel.ToJson()));

	imtgql::CGqlObject additionObject("addition");
	additionObject.InsertField("clientId", QVariant(agentId));
	inputObject.InsertField("addition", additionObject);
	request.AddParam(inputObject);

	imtgql::CGqlObject updatedObject("updatedNotification");
	updatedObject.InsertField("Id");
	request.AddField(updatedObject);

	QString errorMessage;
	istd::TDelPtr<imtbase::CTreeItemModel> responseModelPtr = m_requestHandlerCompPtr->CreateResponse(request, errorMessage);
	if (responseModelPtr.IsValid()){
	}
	else{
		SendErrorMessage(0, QString("Unable to update service on the agent"));
	}
}


void CAgentCollectionControllerComp::OnTimeout()
{
	while (!m_connectedAgents.isEmpty()){
		QByteArray agentId = m_connectedAgents.at(0);
		m_connectedAgents.removeFirst();
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_objectCollectionCompPtr->GetObjectData(agentId, dataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(dataPtr.GetPtr());
			if (agentInfoPtr != nullptr){
				imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
				if (serviceCollectionPtr != nullptr){
					imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& id : ids){
						agentinodata::CServiceInfo* serviceInfoInfoPtr = nullptr;
						imtbase::IObjectCollection::DataPtr serviceDataPtr;
						if (serviceCollectionPtr->GetObjectData(id, serviceDataPtr)){
							serviceInfoInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
						}

						if (serviceInfoInfoPtr == nullptr){
							continue;
						}

						UpdateAgentService(agentId, id);

						if (m_requestHandlerCompPtr.IsValid()){
							istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
							serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

							serviceStatusInfoPtr->SetServiceId(id);
							serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_UNDEFINED);

							imtgql::CGqlRequest request(imtgql::CGqlRequest::RT_QUERY, "ServiceItem");
							imtgql::CGqlObject inputObject("input");
							inputObject.InsertField(QByteArray("Id"), QVariant(id));

							imtgql::CGqlObject additionObject("addition");
							additionObject.InsertField("clientId", QVariant(agentId));
							inputObject.InsertField("addition", additionObject);
							request.AddParam(inputObject);

							imtgql::CGqlObject itemObject("item");
							itemObject.InsertField("Id");
							request.AddField(itemObject);

							// Service representaton model from Agent
							QString errorMessage;
							istd::TDelPtr<imtbase::CTreeItemModel> responseModelPtr = m_requestHandlerCompPtr->CreateResponse(request, errorMessage);
							if (responseModelPtr.IsValid()){
								if (responseModelPtr->ContainsKey("data")){
									imtbase::CTreeItemModel* dataModelPtr = responseModelPtr->GetTreeItemModel("data");
									if (dataModelPtr != nullptr){
										if (dataModelPtr->ContainsKey("Status")){
											QByteArray statusId = dataModelPtr->GetData("Status").toByteArray();

											if (statusId == "running"){
												serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_RUNNING);
											}
											else if (statusId == "notRunning"){
												serviceStatusInfoPtr->SetServiceStatus(agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);
											}

											if (m_serviceStatusCollectionCompPtr.IsValid()){
												if (m_serviceStatusCollectionCompPtr->GetElementIds().contains(id)){
													m_serviceStatusCollectionCompPtr->SetObjectData(id, *serviceStatusInfoPtr.PopPtr());
												}
												else{
													m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), id);
												}
											}
										}
										if (dataModelPtr->ContainsKey("Version")){
											QByteArray serviceVersion = dataModelPtr->GetData("Version").toByteArray();
											serviceInfoInfoPtr->SetServiceVersion(serviceVersion);
										}

										// Если на сервере есть сервис без input портов то берем из агента
										imtbase::IObjectCollection* inputConnectionCollectionPtr = serviceInfoInfoPtr->GetInputConnections();
										if (inputConnectionCollectionPtr != nullptr){
											if (inputConnectionCollectionPtr->GetElementIds().isEmpty()){
												if (dataModelPtr->ContainsKey("InputConnections")){
													imtbase::CTreeItemModel* agentInputConnectionsModelPtr = dataModelPtr->GetTreeItemModel("InputConnections");
													if (agentInputConnectionsModelPtr != nullptr){
														for (int i = 0; i < agentInputConnectionsModelPtr->GetItemsCount(); i++){
															QByteArray id = agentInputConnectionsModelPtr->GetData("Id", i).toByteArray();
															QString usageId = agentInputConnectionsModelPtr->GetData("UsageId", i).toString();
															QString serviceTypeName = agentInputConnectionsModelPtr->GetData("ServiceTypeName", i).toString();
															QString description = agentInputConnectionsModelPtr->GetData("Description", i).toString();
															QString host = agentInputConnectionsModelPtr->GetData("Host", i).toString();
															int port = agentInputConnectionsModelPtr->GetData("Port", i).toInt();

															QUrl connectionUrl;
															connectionUrl.setHost(host);
															connectionUrl.setPort(port);

															istd::TDelPtr<imtservice::CUrlConnectionParam> urlConnectionParamPtr;
															urlConnectionParamPtr.SetPtr(new imtservice::CUrlConnectionParam(
																							 serviceTypeName.toUtf8(),
																							 usageId.toUtf8(),
																							 imtservice::IServiceConnectionParam::CT_INPUT,
																							 connectionUrl));

															inputConnectionCollectionPtr->InsertNewObject("ConnectionInfo", serviceTypeName, description, urlConnectionParamPtr.PopPtr());
														}
													}
												}
											}
										}

										// Если на сервере есть сервис без output портов то берем из агента
										imtbase::IObjectCollection* dependantConnectionCollectionPtr = serviceInfoInfoPtr->GetDependantServiceConnections();
										if (dependantConnectionCollectionPtr != nullptr){
											if (dependantConnectionCollectionPtr->GetElementIds().isEmpty()){
												if (dataModelPtr->ContainsKey("OutputConnections")){
													imtbase::CTreeItemModel* agentOutputConnectionsModelPtr = dataModelPtr->GetTreeItemModel("OutputConnections");
													if (agentOutputConnectionsModelPtr != nullptr){
														for (int i = 0; i < agentOutputConnectionsModelPtr->GetItemsCount(); i++){
															QByteArray id = agentOutputConnectionsModelPtr->GetData("Id", i).toByteArray();

															QString serviceTypeName = agentOutputConnectionsModelPtr->GetData("ServiceTypeName", i).toString();
															QString usageId = agentOutputConnectionsModelPtr->GetData("UsageId", i).toString();
															QString description = agentOutputConnectionsModelPtr->GetData("Description", i).toString();
															QString dependantServiceConnectionId = agentOutputConnectionsModelPtr->GetData("DependantConnectionId", i).toString();

															istd::TDelPtr<imtservice::CUrlConnectionLinkParam> urlConnectionLinkParamPtr;
															urlConnectionLinkParamPtr.SetPtr(new imtservice::CUrlConnectionLinkParam(
																								 serviceTypeName.toUtf8(),
																								 usageId.toUtf8(),
																								 dependantServiceConnectionId.toUtf8()));

															dependantConnectionCollectionPtr->InsertNewObject("ConnectionLink", serviceTypeName, description, urlConnectionLinkParamPtr.PopPtr(), id);
														}
													}
												}
											}
										}
									}
								}
							}
						}

						serviceCollectionPtr->SetObjectData(id, *serviceInfoInfoPtr);
					}
				}

				m_objectCollectionCompPtr->SetObjectData(agentId, *agentInfoPtr);
			}
		}
	}
}


} // namespace agentinogql


