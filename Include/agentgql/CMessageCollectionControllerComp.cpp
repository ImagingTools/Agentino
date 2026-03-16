// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CMessageCollectionControllerComp.h>


// ACF includes
#include <ilog/CMessage.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


// reimplemented (sdl::agentino::ServiceLog::CServiceLogCollectionControllerCompBase)

bool CMessageCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::agentino::ServiceLog::CGetServiceLogGqlRequest& getServiceLogRequest,
			sdl::imtbase::ImtCollection::CMessageItem::V1_0& representationObject,
			QString& errorMessage) const
{
	QByteArray objectId = objectCollectionIterator.GetObjectId();
	sdl::agentino::ServiceLog::GetServiceLogRequestInfo requestInfo = getServiceLogRequest.GetRequestInfo();

	const ilog::CMessage* messagePtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (objectCollectionIterator.GetObjectData(dataPtr)){
		messagePtr = dynamic_cast<const ilog::CMessage*>(dataPtr.GetPtr());
	}

	if (messagePtr == nullptr){
		return false;
	}

	QByteArray serviceId;
	const imtgql::IGqlContext* gqlContextPtr = getServiceLogRequest.GetRequestContext();
	if (gqlContextPtr != nullptr){
		imtgql::IGqlContext::Headers headers = gqlContextPtr->GetHeaders();

		serviceId = headers["serviceid"];
	}

	istd::TUniqueInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(serviceId, errorMessage);
	if (!messageCollectionPtr.IsValid()){
		SendErrorMessage(0, errorMessage, "CObjectCollectionControllerCompBase");

		return false;
	}

	if (requestInfo.items.isIdRequested){
		representationObject.id = objectId;
	}

	if (requestInfo.items.isTypeIdRequested){
		representationObject.typeId = messageCollectionPtr->GetObjectTypeId(objectCollectionIterator.GetObjectId());
	}

	if (requestInfo.items.isInfoIdRequested){
		representationObject.infoId = messagePtr->GetInformationId();
	}

	if (requestInfo.items.isCategoryRequested){
		representationObject.category = messagePtr->GetInformationCategory();
	}

	if (requestInfo.items.isSourceRequested){
		representationObject.source = messagePtr->GetInformationSource();
	}

	if (requestInfo.items.isTextRequested){
		representationObject.text = messagePtr->GetInformationDescription();
	}

	if (requestInfo.items.isTimestampRequested){
		representationObject.timestamp = messagePtr->GetInformationTimeStamp().toString("dd.MM.yyyy hh:mm:ss.zzz");;
	}

	return true;
}


imtbase::CTreeItemModel* CMessageCollectionControllerComp::ListObjects(
	const imtgql::CGqlRequest& gqlRequest,
	QString &errorMessage) const
{
	const imtgql::CGqlParamObject& inputParams = gqlRequest.GetParams();

	istd::TDelPtr<imtbase::CTreeItemModel> rootModelPtr(new imtbase::CTreeItemModel());

	imtbase::CTreeItemModel* dataModel = new imtbase::CTreeItemModel();
	imtbase::CTreeItemModel* itemsModel = new imtbase::CTreeItemModel();
	imtbase::CTreeItemModel* notificationModel = new imtbase::CTreeItemModel();
	dataModel->SetExternTreeModel("items", itemsModel);
	dataModel->SetExternTreeModel("notification", notificationModel);
	rootModelPtr->SetExternTreeModel("data", dataModel);

	QByteArray serviceid = gqlRequest.GetHeader("serviceid");
	const imtgql::CGqlParamObject* viewParamsGql = nullptr;
	const imtgql::CGqlParamObject* inputObject = inputParams.GetParamArgumentObjectPtr("input");
	if (inputObject != nullptr){
		viewParamsGql = inputObject->GetParamArgumentObjectPtr("viewParams");
	}

	istd::TUniqueInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(serviceid, errorMessage);
	if (!messageCollectionPtr.IsValid()){
		SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");

		return rootModelPtr.PopPtr();
	}

	iprm::CParamsSet filterParams;

	int offset = 0, count = -1;

	if (viewParamsGql != nullptr){
		offset = viewParamsGql->GetParamArgumentValue("offset").toInt();
		count = viewParamsGql->GetParamArgumentValue("count").toInt();
		PrepareFilters(gqlRequest, *viewParamsGql, filterParams);
	}

	int elementsCount = messageCollectionPtr->GetElementsCount(&filterParams);

	int pagesCount = std::ceil(elementsCount / (double)count);
	if (pagesCount <= 0){
		pagesCount = 1;
	}

	notificationModel->SetData("pagesCount", pagesCount);
	notificationModel->SetData("totalCount", elementsCount);

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


// reimplemented (icomp::CComponentBase)

void CMessageCollectionControllerComp::OnComponentDestroyed()
{
	m_pluginMap.clear();

	BaseClass::OnComponentDestroyed();
}


istd::TUniqueInterfacePtr<imtbase::IObjectCollection> CMessageCollectionControllerComp::GetMessageCollection(const QByteArray& serviceId, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QString("Internal error").toUtf8();
		return nullptr;
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		const agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
		if (serviceInfoPtr != nullptr){
			QByteArray serviceName = serviceInfoPtr->GetServiceTypeId().toUtf8();
			QString servicePath = serviceInfoPtr->GetServicePath();
			QByteArray serviceTypeName = serviceInfoPtr->GetServiceTypeId().toUtf8();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			if (!m_pluginMap.contains(serviceName)){
				istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
				pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

				if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")){
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
						if (pluginPtr->GetPluginName() != fileInfo.baseName() + "Log"){
							continue;
						}

						messageCollectionFactoryPtr = pluginPtr->GetObjectCollectionFactory();

						break;
					}
				}
				if (messageCollectionFactoryPtr != nullptr){
					return messageCollectionFactoryPtr->CreateInstance();
				}
			}
		}
	}

	return nullptr;
}


} // namespace agentgql


