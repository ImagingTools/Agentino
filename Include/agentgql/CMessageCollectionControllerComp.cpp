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
			const imtbase::IObjectCollectionIterator* objectCollectionIterator,
			QString& errorMessage) const
{
	QByteArray agentId = gqlRequest.GetHeader("clientid");

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
		if (objectCollectionIterator->GetObjectData(massageDataPtr)){
			messagePtr = dynamic_cast<ilog::CMessage*>(massageDataPtr.GetPtr());
		}

		if (messagePtr != nullptr){
			for (const QByteArray& informationId : informationIds){
				QVariant elementInformation;

				if(informationId == "TypeId"){
					elementInformation = messageCollectionPtr->GetObjectTypeId(objectCollectionIterator->GetObjectId());
				}
				else if(informationId == "Id"){
					elementInformation = objectCollectionIterator->GetObjectId();
				}
				else if(informationId == "Text"){
					elementInformation = messagePtr->GetInformationDescription();
				}
				else if(informationId == "Category"){
					elementInformation = messagePtr->GetInformationCategory();
				}
				else if(informationId == "ID"){
					elementInformation = messagePtr->GetInformationId();
				}
				else if(informationId == "Flags"){
					elementInformation = messagePtr->GetInformationFlags();
				}
				else if(informationId == "Source"){
					elementInformation = messagePtr->GetInformationSource();
				}
				else if(informationId == "Timestamp" || informationId == "LastModified"){
					elementInformation = messagePtr->GetInformationTimeStamp().toString("dd.MM.yyyy hh:mm:ss.zzz");
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


imtbase::CTreeItemModel* CMessageCollectionControllerComp::ListObjects(
			const imtgql::CGqlRequest &gqlRequest,
			QString &errorMessage) const
{
	const imtgql::CGqlObject& inputParams = gqlRequest.GetParams();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();
	imtbase::CTreeItemModel* itemsModel = new imtbase::CTreeItemModel();
	imtbase::CTreeItemModel* notificationModel = new imtbase::CTreeItemModel();
	dataModel->SetExternTreeModel("items", itemsModel);
	dataModel->SetExternTreeModel("notification", notificationModel);
	rootModelPtr->SetExternTreeModel("data", dataModel);
	itemsModel->SetIsArray(true);

	QByteArray agentId = gqlRequest.GetHeader("clientid");
	const imtgql::CGqlObject* viewParamsGql = nullptr;
	const imtgql::CGqlObject* inputObject = inputParams.GetFieldArgumentObjectPtr("input");
	if (inputObject != nullptr){
		viewParamsGql = inputObject->GetFieldArgumentObjectPtr("viewParams");
	}

	istd::TDelPtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(gqlRequest, errorMessage);
	if (!messageCollectionPtr.IsValid()){
		SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");

		return rootModelPtr.PopPtr();
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

	istd::TDelPtr<imtbase::IObjectCollectionIterator> iterator = 
				messageCollectionPtr->CreateObjectCollectionIterator(QByteArray(), offset, count, &filterParams);

	while (iterator.IsValid() && iterator->Next()){
		int itemIndex = itemsModel->InsertNewItem();
		if (itemIndex >= 0){
			if (!SetupGqlItem(gqlRequest, *itemsModel, itemIndex, iterator.GetPtr(), errorMessage)){
				SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");
			}
		}
	}

	return rootModelPtr.PopPtr();
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::GetObject(const imtgql::CGqlRequest& /*gqlRequest*/, QString& /*errorMessage*/) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::InsertObject(const imtgql::CGqlRequest& /*gqlRequest*/, QString& /*errorMessage*/) const
{
	return nullptr;
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::UpdateObject(const imtgql::CGqlRequest& /*gqlRequest*/, QString& /*errorMessage*/) const
{
	return nullptr;
}


void CMessageCollectionControllerComp::SetObjectFilter(
	const imtgql::CGqlRequest& /*gqlRequest*/,
	const imtbase::CTreeItemModel& objectFilterModel,
	iprm::CParamsSet& filterParams) const
{
	QByteArrayList keys;
	keys << "VerboseFilter" << "InfoFilter" << "WarningFilter" << "ErrorFilter" << "CriticalFilter";
	istd::TDelPtr<iprm::CParamsSet> categoryFilterPtr(new iprm::CParamsSet());

	imtbase::CTreeItemModel *categoryModel = objectFilterModel.GetTreeItemModel("Category");
	if (categoryModel == nullptr){
		return;
	}

	for (QByteArray key: keys){
		if (categoryModel->ContainsKey(key)){
			QByteArray filterValue = categoryModel->GetData(key).toByteArray();
			if (!filterValue.isEmpty()){
				istd::TDelPtr<iprm::CTextParam> textParamPtr(new iprm::CTextParam());
				textParamPtr->SetText(filterValue);
				categoryFilterPtr->SetEditableParameter(key, textParamPtr.PopPtr());
			}
		}
	}
	filterParams.SetEditableParameter("Category", categoryFilterPtr.PopPtr());
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

	QByteArray serviceId = gqlRequest.GetHeader("serviceid");
	const imtgql::CGqlObject& inputParams = gqlRequest.GetParams();

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		const agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = m_objectCollectionCompPtr->GetElementInfo(serviceId, imtbase::IObjectCollection::EIT_NAME).toByteArray();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeName().toUtf8();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			if (!m_pluginMap.contains(serviceName)){
				istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
				pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

				if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")) {
					SendErrorMessage(0, QString("Unable to load a plugin for '%1'").arg(qPrintable(serviceName)), "CMessageCollectionControllerComp");
					m_pluginMap.remove(serviceName);

					return nullptr;
				}
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


