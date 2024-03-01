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
		const imtgql::CGqlObject* addition =gqlInputParamPtr->GetFieldArgumentObjectPtr("addition");
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
						QProcess::ProcessState state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
						agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
						elementInformation = processStateEnum.id;
					}
				}
				else if(informationId == "StatusName"){
					if (m_serviceControllerCompPtr.IsValid()){
						QProcess::ProcessState state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
						agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
						elementInformation = processStateEnum.name;
					}
				}
				else if(informationId == "IsAutoStart"){
					elementInformation = serviceInfoPtr->IsAutoStart();
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
			QString serviceName = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toString();
			QString description = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QString settingsPath = serviceInfoPtr->GetServiceSettingsPath();
			QString arguments = serviceInfoPtr->GetServiceArguments().join(' ');
			bool isAutoStart = serviceInfoPtr->IsAutoStart();

			dataModel->SetData("Id", serviceId);
			dataModel->SetData("Name", serviceName);
			dataModel->SetData("Description", description);
			dataModel->SetData("Path", servicePath);
			dataModel->SetData("SettingsPath", settingsPath);
			dataModel->SetData("Arguments", arguments);
			dataModel->SetData("IsAutoStart", isAutoStart);

			if (m_serviceControllerCompPtr.IsValid()){
				QProcess::ProcessState state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
				agentinodata::ProcessStateEnum processStateEnum = agentinodata::GetProcceStateRepresentation(state);
				dataModel->SetData("Status", processStateEnum.id);
			}

			imtbase::CTreeItemModel* inputConnectionsModelPtr = dataModel->AddTreeModel("InputConnections");
			imtbase::CTreeItemModel* outputConnectionsModelPtr = dataModel->AddTreeModel("OutputConnections");

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";
			if (!m_pluginMap.contains(serviceName)){
				LoadPluginDirectory(pluginPath, serviceName);
			}

			if (m_pluginMap.contains(serviceName)){
				istd::TDelPtr<imtservice::IConnectionCollection> connectionCollection = m_pluginMap[serviceName].pluginPtr->GetConnectionCollectionFactory()->CreateInstance();
				if (connectionCollection != nullptr){
					const imtbase::ICollectionInfo* collectionInfo = connectionCollection->GetUrlList();
					const imtbase::IObjectCollection* objectCollection = dynamic_cast<const imtbase::IObjectCollection*>(collectionInfo);
					if (objectCollection != nullptr){
						QByteArrayList ids = collectionInfo->GetElementIds();
						for (QByteArray id: ids){
							const imtservice::IServiceConnectionParam* connectionParamPtr = connectionCollection->GetConnectionMetaInfo(id);
							if (connectionParamPtr == nullptr){
								continue;
							}

							if (connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionParam::CT_INPUT){
								int index = inputConnectionsModelPtr->InsertNewItem();
								QString connectionName = objectCollection->GetElementInfo(id, imtbase::IObjectCollection::EIT_NAME).toString();
								QString connectionDescription = collectionInfo->GetElementInfo(id, imtbase::IObjectCollection::EIT_DESCRIPTION).toString();
								inputConnectionsModelPtr->SetData("Id", connectionName, index);
								inputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								inputConnectionsModelPtr->SetData("ServiceTypeName", connectionParamPtr->GetServiceTypeName(), index);
								inputConnectionsModelPtr->SetData("UsageId", connectionParamPtr->GetUsageId(), index);
								inputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								inputConnectionsModelPtr->SetData("ServiceName", serviceName, index);
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
								outputConnectionsModelPtr->SetData("Id", connectionName, index);
								outputConnectionsModelPtr->SetData("UsageId", connectionParamPtr->GetUsageId(), index);
								outputConnectionsModelPtr->SetData("ConnectionName", connectionName, index);
								outputConnectionsModelPtr->SetData("Description", connectionDescription, index);
								outputConnectionsModelPtr->SetData("ServiceName", serviceName, index);

								imtbase::IObjectCollection::DataPtr dataPtr;
								objectCollection->GetObjectData(id, dataPtr);

								imtservice::CUrlConnectionParam* connectionParam = dynamic_cast<imtservice::CUrlConnectionParam*>(dataPtr.GetPtr());
								if (connectionParam != nullptr){
									QUrl url = connectionParam->GetUrl();

									QString name = serviceName + "@" + url.host() + QString::number(url.port());

									outputConnectionsModelPtr->SetData("Url", name, index);

									imtbase::CTreeItemModel* elementsModel = outputConnectionsModelPtr->AddTreeModel("Elements", index);
									elementsModel->SetData("Id", name);
									elementsModel->SetData("Name", name);
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

	if (resultPtr->ContainsKey("data")){
		imtbase::CTreeItemModel* objectRepresentationDataModelPtr = GetObject(gqlRequest, errorMessage);
		if (objectRepresentationDataModelPtr != nullptr){
			resultPtr->SetExternTreeModel("item", objectRepresentationDataModelPtr->GetTreeItemModel("data"));
		}
	}

	return resultPtr;
}


imtbase::CTreeItemModel* CServiceCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	imtbase::CTreeItemModel* resultPtr = BaseClass::UpdateObject(gqlRequest, errorMessage);

	const imtgql::CGqlObject* gqlInputParamPtr = gqlRequest.GetParam("input");
	if (gqlInputParamPtr == nullptr){
		return nullptr;
	}

	QByteArray objectId = gqlInputParamPtr->GetFieldArgumentValue("Id").toByteArray();
	QByteArray itemData = gqlInputParamPtr->GetFieldArgumentValue("Item").toByteArray();
	if (itemData.isEmpty()){
		return nullptr;
	}

	imtbase::CTreeItemModel itemModel;
	if (!itemModel.CreateFromJson(itemData)){
		return nullptr;
	}

	QString servicePath;
	if (itemModel.ContainsKey("Path")){
		servicePath = itemModel.GetData("Path").toString();
	}

	if (itemModel.ContainsKey("InputConnections")){
		imtbase::CTreeItemModel* inputConnectionsModelPtr = itemModel.GetTreeItemModel("InputConnections");
		if (inputConnectionsModelPtr != nullptr){
			for (int i = 0; i < inputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray inputConnectionId = inputConnectionsModelPtr->GetData("Id", i).toByteArray();
				QByteArray serviceName = inputConnectionsModelPtr->GetData("ServiceName", i).toByteArray();
				int servicePort = inputConnectionsModelPtr->GetData("Port", i).toInt();

				QFileInfo fileInfo(servicePath);
				QString pluginPath = fileInfo.path() + "/Plugins";

				if (!m_pluginMap.contains(serviceName)){
					LoadPluginDirectory(pluginPath, serviceName);
				}

				QStringList keys = m_pluginMap.keys();

				if (m_pluginMap.contains(serviceName)){
					istd::TDelPtr<imtservice::IConnectionCollection> connectionCollectionPtr = m_pluginMap[serviceName].pluginPtr->GetConnectionCollectionFactory()->CreateInstance();
					if (connectionCollectionPtr != nullptr){
						const imtbase::ICollectionInfo* collectionInfoPtr = connectionCollectionPtr->GetUrlList();
						if (collectionInfoPtr == nullptr){
							continue;
						}

						QUrl url;
						url.setHost("localhost");
						url.setPort(servicePort);

						connectionCollectionPtr->SetUrl(inputConnectionId, url);
					}
				}
			}
		}
	}

	if (itemModel.ContainsKey("OutputConnections")){
		imtbase::CTreeItemModel* outputConnectionsModelPtr = itemModel.GetTreeItemModel("OutputConnections");
		if (outputConnectionsModelPtr != nullptr){
			for (int i = 0; i < outputConnectionsModelPtr->GetItemsCount(); i++){
				QByteArray outputConnectionId = outputConnectionsModelPtr->GetData("Id", i).toByteArray();
				QByteArray serviceName = outputConnectionsModelPtr->GetData("ServiceName", i).toByteArray();
				QString urlStr = outputConnectionsModelPtr->GetData("Url", i).toString();

				QUrl url(urlStr);

				if (m_pluginMap.contains(serviceName)){
					istd::TDelPtr<imtservice::IConnectionCollection> connectionCollection = m_pluginMap[serviceName].pluginPtr->GetConnectionCollectionFactory()->CreateInstance();
					if (connectionCollection != nullptr){
						const imtbase::ICollectionInfo* collectionInfoPtr = connectionCollection->GetUrlList();
						if (collectionInfoPtr == nullptr){
							continue;
						}

						connectionCollection->SetUrl(outputConnectionId, url);
					}
				}
			}
		}
	}

	if (m_serviceControllerCompPtr.IsValid()){
		QProcess::ProcessState serviceStatus = m_serviceControllerCompPtr->GetServiceStatus(objectId);
		if (serviceStatus == QProcess::ProcessState::Running){
			m_serviceControllerCompPtr->StopService(objectId);
			m_serviceControllerCompPtr->StartService(objectId);
		}
	}

	return resultPtr;
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
			serviceInfoPtr->SetServicePath(path);
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

		return serviceInfoPtr;
	}

	errorMessage = QString("Can not create service: %1").arg(QString(objectId));

	return nullptr;
}


void CServiceCollectionControllerComp::OnComponentDestroyed()
{
	for (typename PluginMap::iterator iter = m_pluginMap.begin(); iter != m_pluginMap.end(); ++iter){
		iter->destroyFunc(iter->pluginPtr);
	}

	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
}


bool CServiceCollectionControllerComp::LoadPluginDirectory(const QString& pluginDirectoryPath, const QString& serviceName) const
{
	SendInfoMessage(0, QString("Looking for the document plugins in '%1'").arg(pluginDirectoryPath));

	if (!pluginDirectoryPath.isEmpty() && QFileInfo(pluginDirectoryPath).exists()){
		QDir pluginsDirectory(pluginDirectoryPath);

		QFileInfoList pluginsList = pluginsDirectory.entryInfoList(QStringList() << "*.plugin");

		for (const QFileInfo& pluginPath : pluginsList){
#ifdef Q_OS_WIN
			SetDllDirectory(pluginPath.absolutePath().toStdWString().c_str());
#endif
			SendInfoMessage(0, QString("Load: '%1'").arg(pluginPath.canonicalFilePath()));

			QLibrary library(pluginPath.canonicalFilePath());
			if (library.load()){
				IMT_CREATE_PLUGIN_FUNCTION(ServiceSettings) createPluginFunc = (IMT_CREATE_PLUGIN_FUNCTION(ServiceSettings))library.resolve(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings));
				if (createPluginFunc != NULL){
					istd::TDelPtr<imtservice::IConnectionCollectionPlugin> pluginInstancePtr = createPluginFunc();
					if (pluginInstancePtr.IsValid()){
						PluginInfo pluginInfo;
						pluginInfo.pluginPath = pluginPath.canonicalFilePath();
						pluginInfo.pluginPtr = pluginInstancePtr.PopPtr();
						pluginInfo.destroyFunc = (IMT_DESTROY_PLUGIN_FUNCTION(ServiceSettings))library.resolve(IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceSettings));
						m_pluginMap.insert(serviceName, pluginInfo);
					}
				}
			}
			else{
				SendErrorMessage(0, QString("%1").arg(library.errorString()));
			}
		}

		return true;
	}

	return false;
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


