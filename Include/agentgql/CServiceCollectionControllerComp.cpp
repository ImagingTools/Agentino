#include <agentgql/CServiceCollectionControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#undef GetObject
#undef StartService

// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


bool CServiceCollectionControllerComp::SetupGqlItem(
			const imtgql::CGqlRequest& gqlRequest,
			imtbase::CTreeItemModel& model,
			int itemIndex,
			const QByteArray& collectionId,
			QString& errorMessage) const
{
	QByteArray agentId;
	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr != nullptr){
		const imtgql::CGqlObject* addition = gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			agentId = addition->GetFieldArgumentValue("clientId").toByteArray();
		}
	}

	istd::TDelPtr<imtbase::IObjectCollection> collectionPtr = GetObjectCollection(agentId);
	if (!collectionPtr.IsValid()){
		errorMessage = QString("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty() && collectionPtr.IsValid()){
		agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (collectionPtr->GetObjectData(collectionId, serviceDataPtr)){
			serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
		}

		if (serviceInfoPtr != nullptr){
			QByteArray serviceId;
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = collectionPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "Id"){
					serviceId = collectionId;
					elementInformation = serviceId;
				}
				else if(informationId == "Name"){
					elementInformation = collectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_NAME).toString();
				}
				else if(informationId == "Description"){
					elementInformation = collectionPtr->GetElementInfo(collectionId, imtbase::IObjectCollection::EIT_NAME).toString();
				}
				else if(informationId == "Path"){
					elementInformation = serviceInfoPtr->GetServicePath();
				}
				else if(informationId == "StartScript"){
					elementInformation = serviceInfoPtr->GetStartScriptPath();
				}
				else if(informationId == "StopScript"){
					elementInformation = serviceInfoPtr->GetStopScriptPath();
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
				else if(informationId == "Status"){
					if (m_serviceControllerCompPtr.IsValid()){
						agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
						agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
						elementInformation = processStateEnum.id;
					}
				}
				else if(informationId == "StatusName"){
					if (m_serviceControllerCompPtr.IsValid()){
						agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
						agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
						elementInformation = processStateEnum.name;
					}
				}
				else if(informationId == "IsAutoStart"){
					elementInformation = serviceInfoPtr->IsAutoStart();
				}
				else if(informationId == "TracingLevel"){
					elementInformation = serviceInfoPtr->GetTracingLevel();
				}
				else if(informationId == "Version"){
					elementInformation = serviceInfoPtr->GetServiceVersion();
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


imtbase::CTreeItemModel *CServiceCollectionControllerComp::ListObjects(const imtgql::CGqlRequest &gqlRequest, QString &errorMessage) const
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

		istd::TDelPtr<imtbase::IObjectCollection> collectionPtr = GetObjectCollection(agentId);

		if (!collectionPtr.IsValid()){
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

		int elementsCount = collectionPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

		imtbase::ICollectionInfo::Ids ids = collectionPtr->GetElementIds(offset, count, &filterParams);

		for (QByteArray id: ids){
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


imtbase::CTreeItemModel* CServiceCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Internal error").toUtf8();
		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());
	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();

	QByteArray serviceId = GetObjectIdFromInputParams(gqlRequest.GetParams());

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		const agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			QString description = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QString startScriptPath = serviceInfoPtr->GetStartScriptPath();
			QString stopScriptPath = serviceInfoPtr->GetStopScriptPath();
			QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
			QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeName().toUtf8();
			QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');
			bool isAutoStart = serviceInfoPtr->IsAutoStart();
			int tracingLevel = serviceInfoPtr->GetTracingLevel();
			QString serviceVersion = serviceInfoPtr->GetServiceVersion();

			dataModel->SetData("Id", serviceId);
			dataModel->SetData("Name", serviceName);
			dataModel->SetData("Description", description);
			dataModel->SetData("Path", servicePath);
			dataModel->SetData("StartScript", startScriptPath);
			dataModel->SetData("StopScript", stopScriptPath);
			dataModel->SetData("SettingsPath", settingsPath);
			dataModel->SetData("Arguments", arguments);
			dataModel->SetData("IsAutoStart", isAutoStart);
			dataModel->SetData("ServiceTypeName", serviceTypeName);
			dataModel->SetData("Version", serviceVersion);
			dataModel->SetData("TracingLevel", tracingLevel);

			if (m_serviceControllerCompPtr.IsValid()){
				agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
				agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
				dataModel->SetData("Status", processStateEnum.id);
			}

			imtbase::CTreeItemModel* inputConnectionsModelPtr = dataModel->AddTreeModel("InputConnections");
			imtbase::CTreeItemModel* outputConnectionsModelPtr = dataModel->AddTreeModel("OutputConnections");

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
			pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), nullptr));

			if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")) {
				SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(serviceName), "CServiceCollectionControllerComp");
				m_pluginMap.remove(serviceName);
			}

			if (m_pluginMap.contains(serviceName)){
				const imtservice::IConnectionCollectionPlugin::IConnectionCollectionFactory* connectionCollectionFactoryPtr = nullptr;
				for (int index = 0; index < m_pluginMap[serviceName]->m_plugins.count(); index++){
					imtservice::IConnectionCollectionPlugin* pluginPtr = m_pluginMap[serviceName]->m_plugins[index].pluginPtr;
					if (pluginPtr != nullptr){
						connectionCollectionFactoryPtr = pluginPtr->GetConnectionCollectionFactory();

						break;
					}
				}
				Q_ASSERT(connectionCollectionFactoryPtr != nullptr);
				istd::TDelPtr<imtservice::IConnectionCollection> connectionCollection = connectionCollectionFactoryPtr->CreateInstance();
				if (connectionCollection != nullptr){
					serviceVersion = connectionCollection->GetServiceVersion();
					dataModel->SetData("Version", serviceVersion);
					tracingLevel = connectionCollection->GetTracingLevel();
					dataModel->SetData("TracingLevel", tracingLevel);
					const imtbase::ICollectionInfo* collectionInfo = connectionCollection->GetUrlList();
					const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
					if (objectCollection != nullptr){
						imtbase::ICollectionInfo::Ids ids = collectionInfo->GetElementIds();
						for (const QByteArray& id: ids){
							const imtservice::IServiceConnectionParam* connectionParamPtr = connectionCollection->GetConnectionMetaInfo(id);
							if (connectionParamPtr == nullptr){
								continue;
							}

							if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
								int index = inputConnectionsModelPtr->InsertNewItem();
								QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
								QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
								inputConnectionsModelPtr->SetData("Id", "", index);
								inputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								inputConnectionsModelPtr->SetData("ServiceTypeName", connectionParamPtr->GetServiceTypeName(), index);
								inputConnectionsModelPtr->SetData("UsageId", connectionParamPtr->GetUsageId(), index);
								inputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								inputConnectionsModelPtr->SetData("ServiceName", serviceName, index);
								inputConnectionsModelPtr->SetData("DefaultUrl", connectionParamPtr->GetDefaultUrl().toString(), index);
								inputConnectionsModelPtr->AddTreeModel("ExternPorts", index);

								imtbase::IObjectCollection::DataPtr dataPtr;
								objectCollection->GetObjectData(id, dataPtr);
								imtservice::CUrlConnectionParam* connectionParam = dynamic_cast<imtservice::CUrlConnectionParam*>(dataPtr.GetPtr());
								if (connectionParam != nullptr){
									QUrl url = connectionParam->GetUrl();
									inputConnectionsModelPtr->SetData("Host", url.host(), index);
									inputConnectionsModelPtr->SetData("Port", url.port(), index);
								}
							}
							else{
								int index = outputConnectionsModelPtr->InsertNewItem();
								QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
								QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
								QString usageId = connectionParamPtr->GetUsageId();
								QString serviceTypeName = connectionParamPtr->GetServiceTypeName();
								QString defaultUrl = connectionParamPtr->GetDefaultUrl().toString();

								outputConnectionsModelPtr->SetData("Id", id, index);
								outputConnectionsModelPtr->SetData("UsageId", usageId, index);
								outputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								outputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								outputConnectionsModelPtr->SetData("ServiceName", serviceName, index);
								outputConnectionsModelPtr->SetData("ServiceTypeName", serviceTypeName, index);
								outputConnectionsModelPtr->SetData("DefaultUrl", defaultUrl, index);

								imtbase::IObjectCollection::DataPtr dataPtr;
								objectCollection->GetObjectData(id, dataPtr);

								imtservice::CUrlConnectionParam* connectionParam = dynamic_cast<imtservice::CUrlConnectionParam*>(dataPtr.GetPtr());
								if (connectionParam != nullptr){
									QUrl url = connectionParam->GetUrl();

									QString connectionInfoText = serviceName + "@" + url.host() + QString::number(url.port());

									outputConnectionsModelPtr->SetData("Url", connectionInfoText, index);

									outputConnectionsModelPtr->AddTreeModel("Elements", index);
								}
							}
						}
					}
				}
			}

			agentinodata::IServiceInfo::SettingsType settingsType  = serviceInfoPtr->GetSettingsType();
			switch (settingsType){
			case agentinodata::IServiceInfo::ST_PLUGIN:
				dataModel->SetData("Type", "ACF");
				break;
			default:
				dataModel->SetData("Type", "None");
				break;
			}
		}
	}

	rootModelPtr->SetExternTreeModel("data", dataModel);

	return rootModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CServiceCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	imtbase::CTreeItemModel* resultPtr = BaseClass::InsertObject(gqlRequest, errorMessage);

	if (resultPtr && resultPtr->ContainsKey("data")){
		imtbase::CTreeItemModel* objectRepresentationDataModelPtr = GetObject(gqlRequest, errorMessage);
		if (objectRepresentationDataModelPtr != nullptr){
			resultPtr->SetExternTreeModel("item", objectRepresentationDataModelPtr->GetTreeItemModel("data"));
		}
	}

	return resultPtr;
}


imtbase::CTreeItemModel* CServiceCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		SendErrorMessage(0, QString("m_objectCollectionCompPtr is invalid"), "CServiceCollectionControllerComp");

		return nullptr;
	}

	if (!m_serviceControllerCompPtr.IsValid()){
		SendErrorMessage(0, QString("m_serviceControllerCompPtr is invalid"), "CServiceCollectionControllerComp");

		return nullptr;
	}

	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParam("input");
	if (inputParamPtr == nullptr){
		SendErrorMessage(0, QString("GraphQL input params invalid"), "CServiceCollectionControllerComp");

		return nullptr;
	}

	QByteArray objectId = inputParamPtr->GetFieldArgumentValue("Id").toByteArray();

	QByteArray itemData = inputParamPtr->GetFieldArgumentValue("Item").toByteArray();
	if (itemData.isEmpty()){
		return nullptr;
	}

	imtbase::CTreeItemModel itemModel;
	if (!itemModel.CreateFromJson(itemData)){
		SendErrorMessage(0, QString("Unable to create tree model from json"), "CServiceCollectionControllerComp");

		return nullptr;
	}

	QString servicePath;
	if (itemModel.ContainsKey("Path")){
		servicePath = itemModel.GetData("Path").toString();
	}

	QByteArray serviceTypeName;
	if (itemModel.ContainsKey("ServiceTypeName")){
		serviceTypeName = itemModel.GetData("ServiceTypeName").toByteArray();
	}

	bool enableVerbose = false;
	if (itemModel.ContainsKey("EnableVerbose")){
		enableVerbose = itemModel.GetData("EnableVerbose").toBool();
	}

	QFileInfo fileInfo(servicePath);
	QString pluginPath = fileInfo.path() + "/Plugins";

	istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceTypeName];
	pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings), nullptr));

	if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceSettings")) {
		SendWarningMessage(0, QString("Unable to load a plugin for '%1'").arg(serviceTypeName), "CServiceCollectionControllerComp");
		m_pluginMap.remove(serviceTypeName);
	}

	istd::TDelPtr<imtservice::IConnectionCollection> connectionCollectionPtr;
	if (m_pluginMap.contains(serviceTypeName)){
		for (int index = 0; index < m_pluginMap[serviceTypeName]->m_plugins.count(); index++){
			imtservice::IConnectionCollectionPlugin* pluginPtr = m_pluginMap[serviceTypeName]->m_plugins[index].pluginPtr;
			if (pluginPtr != nullptr){
				connectionCollectionPtr.SetPtr( pluginPtr->GetConnectionCollectionFactory()->CreateInstance());
				break;
			}
		}
	}

	QString name;
	QString description;
	const QList<imtgql::CGqlObject> params = gqlRequest.GetParams();
	if (params.size() > 0){
		name = params.at(0).GetFieldArgumentValue("name").toByteArray();
		description = params.at(0).GetFieldArgumentValue("description").toString();
	}
	istd::IChangeable* savedObject = BaseClass::CreateObject(gqlRequest, objectId, name, description, errorMessage);
	agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(savedObject);
	if (serviceInfoPtr == nullptr){
		if (errorMessage.isEmpty()){
			errorMessage = QString("Unable to create object");
		}

		SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

		return nullptr;
	}

	if (connectionCollectionPtr.IsValid()){
		QString serviceVersion = connectionCollectionPtr->GetServiceVersion();
		serviceInfoPtr->SetServiceVersion(serviceVersion);
	}
	else{
		SendWarningMessage(0, QString("Connection collection for '%1' from plugin is invalid").arg(serviceTypeName), "CServiceCollectionControllerComp");
	}

	if (m_objectCollectionCompPtr->GetElementIds().contains(objectId)){
		if (!m_objectCollectionCompPtr->SetObjectData(objectId, *savedObject)){
			errorMessage = QString("Cannot update service: '%1'").arg(qPrintable(objectId));
			SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

			return nullptr;
		}
	}
	else{
		QByteArray result = m_objectCollectionCompPtr->InsertNewObject("DocumentInfo", name, description, savedObject, objectId);
		if (result.isEmpty()){
			errorMessage = QString("Cannot insert service: '%1'").arg(qPrintable(objectId));
			SendErrorMessage(0, errorMessage, "CServiceCollectionControllerComp");

			return nullptr;
		}
	}

	if (connectionCollectionPtr.IsValid()){
		bool needToUpdate = false;
		if (itemModel.ContainsKey("InputConnections")){
			imtbase::CTreeItemModel* inputConnectionsModelPtr = itemModel.GetTreeItemModel("InputConnections");
			if (inputConnectionsModelPtr != nullptr){
				for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
					QByteArray usageId = inputConnectionsModelPtr->GetData("UsageId", i).toByteArray();
					int servicePort = inputConnectionsModelPtr->GetData("Port", i).toInt();

					if (connectionCollectionPtr.IsValid()){
						const QUrl* actualUrlPtr = connectionCollectionPtr->GetUrl(usageId);
						if (actualUrlPtr != nullptr && actualUrlPtr->port() != servicePort){
							needToUpdate = true;

							break;
						}
					}
				}
			}
		}

		if (!needToUpdate && itemModel.ContainsKey("OutputConnections")){
			imtbase::CTreeItemModel* outputConnectionsModelPtr = itemModel.GetTreeItemModel("OutputConnections");
			if (outputConnectionsModelPtr != nullptr){
				for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
					QByteArray outputConnectionId = outputConnectionsModelPtr->GetData("Id", i).toByteArray();
					QString urlStr = outputConnectionsModelPtr->GetData("Url", i).toString();

					QUrl url(urlStr);

					if (connectionCollectionPtr.IsValid()){
						const QUrl* actualUrlPtr = connectionCollectionPtr->GetUrl(outputConnectionId);
						if (actualUrlPtr != nullptr && actualUrlPtr->port() != url.port()){
							needToUpdate = true;

							break;
						}
					}
				}
			}
		}

		if (!needToUpdate && enableVerbose != connectionCollectionPtr->GetTracingLevel() > -1){
			needToUpdate = true;
		}

		if (needToUpdate){
			bool wasRunning = false;
			agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(objectId);
			if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING){
				wasRunning = true;

				bool wasStopped = m_serviceControllerCompPtr->StopService(objectId);
				if (!wasStopped){
					errorMessage = QString("Service '%1' cannot be stopped").arg(serviceTypeName);
					SendErrorMessage(0, errorMessage);

					return nullptr;
				}
			}

			if (itemModel.ContainsKey("EnableVerbose")){
				if (enableVerbose){
					connectionCollectionPtr->SetTracingLevel(0);
				}
				else{
					connectionCollectionPtr->SetTracingLevel(-1);
				}
			}

			if (itemModel.ContainsKey("InputConnections")){
				imtbase::CTreeItemModel* inputConnectionsModelPtr = itemModel.GetTreeItemModel("InputConnections");
				if (inputConnectionsModelPtr != nullptr){
					for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
						QByteArray inputConnectionId = inputConnectionsModelPtr->GetData("Id", i).toByteArray();
						QByteArray usageId = inputConnectionsModelPtr->GetData("UsageId", i).toByteArray();
						int servicePort = inputConnectionsModelPtr->GetData("Port", i).toInt();

						if (connectionCollectionPtr.IsValid()){
							QUrl url;
							url.setHost("localhost");
							url.setPort(servicePort);

							connectionCollectionPtr->SetUrl(usageId, url);
						}
					}
				}
			}

			if (itemModel.ContainsKey("OutputConnections")){
				imtbase::CTreeItemModel* outputConnectionsModelPtr = itemModel.GetTreeItemModel("OutputConnections");
				if (outputConnectionsModelPtr != nullptr){
					for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
						QByteArray outputConnectionId = outputConnectionsModelPtr->GetData("Id", i).toByteArray();
						QString urlStr = outputConnectionsModelPtr->GetData("Url", i).toString();

						QUrl url(urlStr);

						if (connectionCollectionPtr.IsValid()){
							connectionCollectionPtr->SetUrl(outputConnectionId, url);
						}
					}
				}
			}

			if (wasRunning){
				m_serviceControllerCompPtr->StartService(objectId);
			}
		}
	}

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = nullptr;
	imtbase::CTreeItemModel* notificationModel = nullptr;

	if (!errorMessage.isEmpty()){
		imtbase::CTreeItemModel* errorsModel = rootModelPtr->AddTreeModel("errors");
		errorsModel->SetData("message", errorMessage);
	}
	else{
		dataModel = new imtbase::CTreeItemModel();
		notificationModel = new imtbase::CTreeItemModel();
		notificationModel->SetData("Id", objectId);
		notificationModel->SetData("Name", name);
		dataModel->SetExternTreeModel("updatedNotification", notificationModel);
	}
	rootModelPtr->SetExternTreeModel("data", dataModel);
	imtbase::CTreeItemModel* objectRepresentationDataModelPtr = GetObject(gqlRequest, errorMessage);
	if (objectRepresentationDataModelPtr != nullptr){
		rootModelPtr->SetExternTreeModel("item", objectRepresentationDataModelPtr->GetTreeItemModel("data"));
	}

	return rootModelPtr.PopPtr();
}


istd::IChangeable* CServiceCollectionControllerComp::CreateObject(
			const QList<imtgql::CGqlObject>& inputParams,
			QByteArray& objectId,
			QString& name,
			QString& description,
			QString& errorMessage) const
{
	if (!m_serviceInfoFactCompPtr.IsValid() || !m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT(false);
		return nullptr;
	}

	objectId = GetObjectIdFromInputParams(inputParams);
	if (objectId.isEmpty()){
		objectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
	}

	QByteArray itemData = inputParams.at(0).GetFieldArgumentValue("Item").toByteArray();
	if (!itemData.isEmpty()){
		agentinodata::IServiceInfo* serviceInstancePtr = m_serviceInfoFactCompPtr.CreateInstance();
		if (serviceInstancePtr == nullptr){
			return nullptr;
		}

		agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CIdentifiableServiceInfo*>(serviceInstancePtr);
		if (serviceInfoPtr == nullptr){
			errorMessage = QString("Unable to get an service info!");
			return nullptr;
		}

		imtbase::CTreeItemModel itemModel;
		itemModel.CreateFromJson(itemData);

		serviceInfoPtr->SetObjectUuid(objectId);

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

		if (itemModel.ContainsKey("Path")){
			QByteArray path = itemModel.GetData("Path").toByteArray();
			QFile file(path);
			if (!file.exists()){
				errorMessage = QT_TR_NOOP(QString("Unable to save service. Error: file with path '%1' does not exist.").arg(path));

				return nullptr;
			}

			serviceInfoPtr->SetServicePath(path);
		}

		if (itemModel.ContainsKey("StartScript")){
			QByteArray path = itemModel.GetData("StartScript").toByteArray();
			serviceInfoPtr->SetStartScriptPath(path);
		}

		if (itemModel.ContainsKey("StopScript")){
			QByteArray path = itemModel.GetData("StopScript").toByteArray();
			serviceInfoPtr->SetStopScriptPath(path);
		}

		if (itemModel.ContainsKey("SettingsPath")){
			QByteArray settingsPath = itemModel.GetData("SettingsPath").toByteArray();
			serviceInfoPtr->SetServiceSettingsPath(settingsPath);
		}

		if (itemModel.ContainsKey("Arguments")){
			QByteArray arguments = itemModel.GetData("Arguments").toByteArray();
			serviceInfoPtr->SetServiceArguments(arguments.split(' '));
		}

		if (itemModel.ContainsKey("IsAutoStart")){
			bool isAutoStart = itemModel.GetData("IsAutoStart").toBool();
			serviceInfoPtr->SetIsAutoStart(isAutoStart);
		}

		if (itemModel.ContainsKey("TracingLevel")){
			int tracingLevel = itemModel.GetData("TracingLevel").toInt();
			serviceInfoPtr->SetTracingLevel(tracingLevel);
		}

		return serviceInfoPtr;
	}

	errorMessage = QString("Can not create service: %1").arg(QString(objectId));

	return nullptr;
}


void CServiceCollectionControllerComp::OnComponentDestroyed()
{
	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
}


imtbase::IObjectCollection* CServiceCollectionControllerComp::GetObjectCollection(const QByteArray& /*id*/) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		return nullptr;
	}

	istd::TDelPtr<istd::IChangeable> collectionPtr = m_objectCollectionCompPtr->CloneMe();
	if (!collectionPtr.IsValid()){
		return nullptr;
	}

	return dynamic_cast<imtbase::IObjectCollection*>(collectionPtr.PopPtr());
}


} // namespace agentgql


