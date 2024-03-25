#include <agentgql/CMessageCollectionControllerComp.h>


// Windows includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#undef GetObject
#undef StartService

// ACF includes
#include <idoc/IDocumentMetaInfo.h>
#include <iprm/CTextParam.h>
#include <ilog/CMessage.h>

// ImtCore includes
#include <imtbase/CCollectionFilter.h>
#include <imtauth/ICompanyBaseInfo.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtdb/CSqlDatabaseObjectCollectionComp.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


bool CMessageCollectionControllerComp::SetupGqlItem(
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

	istd::TDelPtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(gqlRequest, errorMessage);
	if (!messageCollectionPtr.IsValid()){
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	bool retVal = true;

	QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");

	if (!informationIds.isEmpty() && messageCollectionPtr.IsValid()){
		ilog::CMessage* messagePtr = nullptr;
		imtbase::IObjectCollection::DataPtr massageDataPtr;
		if (messageCollectionPtr->GetObjectData(collectionId, massageDataPtr)){
			messagePtr = dynamic_cast<ilog::CMessage*>(massageDataPtr.GetPtr());
		}

		if (messagePtr != nullptr){
			QByteArray serviceId;
			for (QByteArray informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = messageCollectionPtr->GetObjectTypeId(collectionId);
				}
				else if(informationId == "Id"){
					serviceId = collectionId;
					elementInformation = serviceId;
				}
				else if(informationId == "MessageDescription"){
					elementInformation = messagePtr->GetInformationDescription();
				}
				else if(informationId == "MessageCategory"){
					elementInformation = messagePtr->GetInformationCategory();
				}
				else if(informationId == "MessageId"){
					elementInformation = messagePtr->GetInformationId();
				}
				else if(informationId == "MessageFlags"){
					elementInformation = messagePtr->GetInformationFlags();
				}
				else if(informationId == "MessageSource"){
					elementInformation = messagePtr->GetInformationSource();
				}
				else if(informationId == "MessageTimeStamp"){
					elementInformation = messagePtr->GetInformationTimeStamp().toString();
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


imtbase::CTreeItemModel *CMessageCollectionControllerComp::ListObjects(const imtgql::CGqlRequest &gqlRequest, QString &errorMessage) const
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

		istd::TDelPtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(gqlRequest, errorMessage);
		if (!messageCollectionPtr.IsValid()){
			SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");

			return nullptr;
		}

		iprm::CParamsSet filterParams;

		int offset = 0, count = -1;

		if (viewParamsGql != nullptr){
			offset = viewParamsGql->GetFieldArgumentValue("Offset").toInt();
			count = viewParamsGql->GetFieldArgumentValue("Count").toInt();
			PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
		}

		int elementsCount = messageCollectionPtr->GetElementsCount(&filterParams);

		int pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		notificationModel->SetData("PagesCount", pagesCount);
		notificationModel->SetData("TotalCount", elementsCount);

		imtbase::ICollectionInfo::Ids ids = messageCollectionPtr->GetElementIds(offset, count, &filterParams);

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


imtbase::CTreeItemModel* CMessageCollectionControllerComp::GetObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	return nullptr;
}


// reimplemented (icomp::CComponentBase)

void CMessageCollectionControllerComp::OnComponentDestroyed()
{
	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
}


imtbase::IObjectCollection* CMessageCollectionControllerComp::GetMessageCollection(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Internal error").toUtf8();
		return nullptr;
	}

	QByteArray serviceId; // = GetObjectIdFromInputParams(gqlRequest.GetParams());
	const QList<imtgql::CGqlObject> inputParams = gqlRequest.GetParams();
	const imtgql::CGqlObject* viewParamsGql = nullptr;
	if (inputParams.size() > 0){
		viewParamsGql = inputParams.at(0).GetFieldArgumentObjectPtr("viewParams");

		const imtgql::CGqlObject* addition = inputParams.at(0).GetFieldArgumentObjectPtr("addition");
		if (addition != nullptr) {
			serviceId = addition->GetFieldArgumentValue("serviceId").toByteArray();
		}
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		const agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeName().toUtf8();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
			pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

			if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")) {
				SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(serviceName), "CMessageCollectionControllerComp");
				m_pluginMap.remove(serviceName);

				return nullptr;
			}

			if (m_pluginMap.contains(serviceName)){
				const imtservice::IObjectCollectionPlugin::IObjectCollectionFactory* messageCollectionFactoryPtr = nullptr;
				for (int index = 0; index < m_pluginMap[serviceName]->m_plugins.count(); index++){
					imtservice::IObjectCollectionPlugin* pluginPtr = m_pluginMap[serviceName]->m_plugins[index].pluginPtr;
					if (pluginPtr != nullptr){
						messageCollectionFactoryPtr = pluginPtr->GetObjectCollectionFactory();

						break;
					}
				}
				if (messageCollectionFactoryPtr != nullptr){
					imtbase::IObjectCollection* messageCollection = messageCollectionFactoryPtr->CreateInstance();
	
					return messageCollection;
				}
			}
		}
	}

	return nullptr;
}


} // namespace agentgql


