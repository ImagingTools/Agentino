// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CMessageCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMutexLocker>

// ACF includes
#include <ilog/CMessage.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


namespace agentgql
{


// reimplemented (sdl::V1_0::agentino::CServiceLogCollectionControllerCompBase)

bool CMessageCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::V1_0::agentino::CGetServiceLogGqlRequest& getServiceLogRequest,
			sdl::V1_0::imtbase::CMessageItem& representationObject,
			QString& errorMessage) const
{
	QByteArray objectId = objectCollectionIterator.GetObjectId();
	sdl::V1_0::agentino::GetServiceLogRequestInfo requestInfo = getServiceLogRequest.GetRequestInfo();

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

		serviceId = headers.value("serviceid");
	}
	if (serviceId.isEmpty()){
		errorMessage = QStringLiteral("GetServiceLog request has no 'serviceid' header");
		return false;
	}

	istd::TSharedInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(serviceId, errorMessage);
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


QJsonObject CMessageCollectionControllerComp::GetObjectListFromRequest(
	const imtgql::CGqlRequest& gqlRequest,
	QString &errorMessage) const
{
	const QByteArray serviceId = gqlRequest.GetHeader("serviceid");
	if (serviceId.isEmpty()){
		errorMessage = QStringLiteral("GetServiceLog request has no 'serviceid' header");
		SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");
		return QJsonObject();
	}

	istd::TSharedInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr = GetMessageCollection(serviceId, errorMessage);
	if (!messageCollectionPtr.IsValid()){
		SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");

		QJsonObject rootObj;
		QJsonObject dataObj;
		dataObj.insert(QStringLiteral("items"), QJsonArray());
		dataObj.insert(QStringLiteral("notification"), QJsonObject());
		rootObj.insert(QStringLiteral("data"), dataObj);
		return rootObj;
	}

	iprm::CParamsSet filterParams;

	int offset = 0;
	int count = -1;

	const imtgql::CGqlParamObject* inputParamsPtr = gqlRequest.GetParamObject("input");
	const imtgql::CGqlParamObject* selectionParamsPtr = inputParamsPtr->GetParamArgumentObjectPtr("selectionParams");
	if (selectionParamsPtr != nullptr){
		offset = inputParamsPtr->GetParamArgumentValue("offset").toInt();
		count = inputParamsPtr->GetParamArgumentValue("count").toInt();
		PrepareFilters(gqlRequest, *selectionParamsPtr, filterParams);
	}

	if (count == 0){
		count = -1;
	}

	QJsonArray itemsArray;
	int elementsCount = 0;
	int pagesCount = 0;

	istd::TDelPtr<imtbase::IObjectCollectionIterator> iterator =
		messageCollectionPtr->CreateObjectCollectionIterator(QByteArray(), offset, count, &filterParams);
	if (iterator.IsValid()){

		elementsCount = iterator->GetElementsCount();

		pagesCount = std::ceil(elementsCount / (double)count);
		if (pagesCount <= 0){
			pagesCount = 1;
		}

		const GqlItemSetupContext setupContext = CreateGqlItemSetupContext(gqlRequest, errorMessage);
		if (!errorMessage.isEmpty()){
			return QJsonObject();
		}

		while (iterator.IsValid() && iterator->Next()){
			QJsonObject itemObj;
			if (!SetupGqlItemWithContext(gqlRequest, setupContext, itemObj, iterator.GetPtr(), errorMessage)){
				SendWarningMessage(0, errorMessage, "CMessageCollectionControllerComp");
			}
			itemsArray.append(itemObj);
		}
	}

	QJsonObject rootObj;
	QJsonObject dataObj;
	QJsonObject notificationObj;
	notificationObj.insert(QStringLiteral("pagesCount"), pagesCount);
	notificationObj.insert(QStringLiteral("totalCount"), elementsCount);
	dataObj.insert(QStringLiteral("items"), itemsArray);
	dataObj.insert(QStringLiteral("notification"), notificationObj);
	rootObj.insert(QStringLiteral("data"), dataObj);
	return rootObj;
}


// reimplemented (icomp::CComponentBase)

void CMessageCollectionControllerComp::OnComponentDestroyed()
{
	{
		QMutexLocker pluginMapLocker(&m_pluginMapMutex);

		m_messageCollectionMap.clear();
		m_pluginMap.clear();
	}

	BaseClass::OnComponentDestroyed();
}


istd::TSharedInterfacePtr<imtbase::IObjectCollection> CMessageCollectionControllerComp::GetMessageCollection(const QByteArray& serviceId, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Service repository is not configured");
		return nullptr;
	}
	if (serviceId.isEmpty()){
		errorMessage = QStringLiteral("Service id is empty");
		return nullptr;
	}

	QMutexLocker pluginMapLocker(&m_pluginMapMutex);

	// Reuse the collection already opened for this service instead of asking the plug-in's
	// factory to create a new instance (e.g. re-opening a SQLite connection) on every call -
	// this method is called once per row (from CreateRepresentationFromObject()) in addition
	// to once from GetObjectListFromRequest(), so recreating the collection here previously reopened the
	// underlying storage for every single log line.
	MessageCollectionMap::const_iterator cachedIt = m_messageCollectionMap.constFind(serviceId);
	if (cachedIt != m_messageCollectionMap.constEnd() && cachedIt.value().IsValid()){
		return cachedIt.value();
	}

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (!m_objectCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
		errorMessage = QStringLiteral("Service '%1' was not found in the service repository")
				.arg(QString::fromUtf8(serviceId));
		return nullptr;
	}

	const agentinodata::CIdentifiableServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::CIdentifiableServiceInfo*>(dataPtr.GetPtr());
	if (serviceInfoPtr == nullptr){
		errorMessage = QStringLiteral("Service '%1' has an invalid descriptor")
				.arg(QString::fromUtf8(serviceId));
		return nullptr;
	}

	{
			QByteArray serviceName = serviceInfoPtr->GetServiceTypeId().toUtf8();
			QString servicePath = serviceInfoPtr->GetServicePath();

			QFileInfo fileInfo(servicePath);
			QString pluginPath = fileInfo.path() + "/Plugins";

			if (!m_pluginMap.contains(serviceName)){
				istd::TDelPtr<PluginManager>& pluginManagerPtr = m_pluginMap[serviceName];
				pluginManagerPtr.SetPtr(new PluginManager(IMT_CREATE_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), IMT_DESTROY_PLUGIN_INSTANCE_FUNCTION_NAME(ServiceLog), nullptr));

				if (!pluginManagerPtr->LoadPluginDirectory(pluginPath, "plugin", "ServiceLog")){
					errorMessage = QStringLiteral("Unable to load service-log plugins from '%1' for service '%2' (type '%3')")
							.arg(pluginPath, QString::fromUtf8(serviceId), QString::fromUtf8(serviceName));
					SendErrorMessage(0, errorMessage, "CMessageCollectionControllerComp");
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
					istd::TUniqueInterfacePtr<imtbase::IObjectCollection> messageCollection = messageCollectionFactoryPtr->CreateInstance();

					istd::TSharedInterfacePtr<imtbase::IObjectCollection> messageCollectionPtr;
					messageCollectionPtr.FromUnique(std::move(messageCollection));

					if (messageCollectionPtr.IsValid()){
						m_messageCollectionMap[serviceId] = messageCollectionPtr;
					}

					return messageCollectionPtr;
				}

				errorMessage = QString("Plugin directory '%1' loaded but no plugin named '%2' was found for service '%3' — GetServiceLog will return empty")
							.arg(pluginPath, fileInfo.baseName() + "Log", QString::fromUtf8(serviceId));
			}
	}

	return nullptr;
}


} // namespace agentgql


