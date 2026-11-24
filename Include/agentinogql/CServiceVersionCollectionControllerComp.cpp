// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceVersionCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// STL includes
#include <algorithm>
#include <cmath>

// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>

// ACF includes
#include <iprm/CParamsSet.h>
#include <iprm/TParamsPtr.h>

// ImtCore includes
#include <imtbase/IComplexCollectionFilter.h>
#include <imtbase/IObjectCollectionIterator.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>
#include <agentinodata/CServiceVersionInfo.h>
#include <agentinogql/CServiceUpdateControllerComp.h>


namespace agentinogql
{


// public methods

// reimplemented (imtservergql::CObjectCollectionControllerCompBase)

sdl::V1_0::imtbase::CGetElementMetaInfoPayload CServiceVersionCollectionControllerComp::OnGetElementMetaInfo(
			const sdl::V1_0::imtbase::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the meta info of a service version. Error: Component reference 'ObjectCollection' was not set");
		SendErrorMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

		return sdl::V1_0::imtbase::CGetElementMetaInfoPayload();
	}

	const sdl::V1_0::imtbase::GetElementMetaInfoRequestArguments arguments = getElementMetaInfoRequest.GetRequestedArguments();

	QByteArray objectId;
	if (arguments.input.has_value() && arguments.input->elementId.has_value()){
		objectId = *arguments.input->elementId;
	}

	QByteArray serviceTypeId = GetServiceTypeId(gqlRequest.GetRequestContext());
	QString version = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();
	QString operatingSystem;

	imtbase::IObjectCollection::DataPtr dataPtr;
	if (m_objectCollectionCompPtr->GetObjectData(objectId, dataPtr)){
		const agentinodata::IServiceVersionInfo* versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(dataPtr.GetPtr());
		if (versionInfoPtr != nullptr){
			serviceTypeId = versionInfoPtr->GetServiceTypeId();
			version = versionInfoPtr->GetVersion();
			operatingSystem = versionInfoPtr->GetOperatingSystem();
		}
	}

	const QStringList agentNames = GetVersionAgentNames(serviceTypeId, version, operatingSystem);

	// "StringList" is shown as a list by MetaInfo.qml (MetaInfoStringListDelegate).
	sdl::V1_0::imtbase::CParameter agentsParameter;
	agentsParameter.id = QByteArrayLiteral("Agents");
	agentsParameter.name = QStringLiteral("Agents");
	agentsParameter.description = QString();
	agentsParameter.typeId = QByteArrayLiteral("StringList");
	agentsParameter.data = QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(agentNames)).toJson(QJsonDocument::Compact));

	sdl::V1_0::imtbase::CElementMetaInfo elementMetaInfo;
	elementMetaInfo.infoParams.Emplace();
	elementMetaInfo.infoParams->FromList(QList<sdl::V1_0::imtbase::CParameter>() << agentsParameter);

	sdl::V1_0::imtbase::CGetElementMetaInfoPayload payload;
	payload.elementMetaInfo = elementMetaInfo;

	return payload;
}


// protected methods

// reimplemented (sdl::V1_0::agentino::CServiceVersionCollectionControllerCompBase)

bool CServiceVersionCollectionControllerComp::CreateRepresentationFromObject(
			const ::imtbase::IObjectCollectionIterator& objectCollectionIterator,
			const sdl::V1_0::agentino::CServiceVersionsListGqlRequest& serviceVersionsListRequest,
			sdl::V1_0::agentino::CServiceVersionItem& representationObject,
			QString& /*errorMessage*/) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ObjectCollection' was not set", "CServiceVersionCollectionControllerComp");
		return false;
	}

	const QByteArray objectId = objectCollectionIterator.GetObjectId();
	const QString version = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();

	const sdl::V1_0::agentino::ServiceVersionsListRequestInfo requestInfo = serviceVersionsListRequest.GetRequestInfo();

	if (requestInfo.items.isIdRequested){
		representationObject.id = objectId;
	}

	if (requestInfo.items.isTypeIdRequested){
		representationObject.typeId = m_objectCollectionCompPtr->GetObjectTypeId(objectId);
	}

	if (requestInfo.items.isNameRequested){
		representationObject.name = version;
	}

	if (requestInfo.items.isVersionRequested){
		representationObject.version = version;
	}

	const agentinodata::IServiceVersionInfo* versionInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr dataPtr;
	if (objectCollectionIterator.GetObjectData(dataPtr)){
		versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(dataPtr.GetPtr());
	}

	if (requestInfo.items.isUploadDateRequested){
		QDateTime uploadDate;
		if (versionInfoPtr != nullptr){
			uploadDate = versionInfoPtr->GetUploadDate();
		}
		if (!uploadDate.isValid()){
			uploadDate = objectCollectionIterator.GetElementInfo("Added").toDateTime();
		}

		if (uploadDate.isValid()){
			representationObject.uploadDate = uploadDate.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
		}
	}

	if (requestInfo.items.isPathRequested){
		representationObject.path = (versionInfoPtr != nullptr) ? QDir::toNativeSeparators(versionInfoPtr->GetPath()) : QString();
	}

	if (requestInfo.items.isOperatingSystemRequested){
		representationObject.operatingSystem = (versionInfoPtr != nullptr) ? versionInfoPtr->GetOperatingSystem() : QString();
	}

	if (requestInfo.items.isInstallationCountRequested){
		QByteArray serviceTypeId = GetServiceTypeId(serviceVersionsListRequest.GetRequestContext());
		QString operatingSystem;
		if (versionInfoPtr != nullptr){
			serviceTypeId = versionInfoPtr->GetServiceTypeId();
			operatingSystem = versionInfoPtr->GetOperatingSystem();
		}

		representationObject.installationCount = GetVersionAgentNames(serviceTypeId, version, operatingSystem).count();
	}

	return true;
}


istd::IChangeableUniquePtr CServiceVersionCollectionControllerComp::CreateObjectFromRepresentation(
			const sdl::V1_0::agentino::CServiceVersionData& serviceVersionDataRepresentation,
			QByteArray& newObjectId,
			QString& errorMessage) const
{
	const QByteArray serviceTypeId = serviceVersionDataRepresentation.serviceTypeId ? *serviceVersionDataRepresentation.serviceTypeId : QByteArray();
	if (serviceTypeId.isEmpty()){
		errorMessage = QStringLiteral("Unable to add a service version. Error: The service type is not set");
		SendErrorMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

		return nullptr;
	}

	QString path = serviceVersionDataRepresentation.path ? *serviceVersionDataRepresentation.path : QString();
	path = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));

	const QFileInfo folderInfo(path);
	if (path.isEmpty() || !folderInfo.isDir()){
		errorMessage = QStringLiteral("Unable to add a service version. Error: The folder '%1' does not exist").arg(QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

		return nullptr;
	}

	// Versions differ by the operating system family: "Windows", "Linux", "macOS" or "Other".
	const QString operatingSystem = serviceVersionDataRepresentation.operatingSystem ? serviceVersionDataRepresentation.operatingSystem->trimmed() : QString();
	if (operatingSystem.isEmpty()){
		errorMessage = QStringLiteral("Unable to add a service version. Error: The operating system is not set");
		SendErrorMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

		return nullptr;
	}

	// The name of the folder is the version.
	QString version = serviceVersionDataRepresentation.version ? serviceVersionDataRepresentation.version->trimmed() : QString();
	if (version.isEmpty()){
		version = folderInfo.fileName();
	}

	if (version.isEmpty()){
		errorMessage = QStringLiteral("Unable to add a service version. Error: The folder '%1' has no name to be used as the version").arg(QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

		return nullptr;
	}

	const QList<VersionElement> elements = GetVersionElements(serviceTypeId, &operatingSystem);
	for (const VersionElement& element : elements){
		if (element.version.compare(version, Qt::CaseInsensitive) == 0){
			errorMessage = QStringLiteral("The version '%1' of the service '%2' for %3 already exists").arg(version, QString::fromUtf8(serviceTypeId), operatingSystem);
			SendWarningMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");

			return nullptr;
		}
	}

	agentinodata::CServiceVersionInfo* versionInfoPtr = new agentinodata::CServiceVersionInfo;
	istd::IChangeableUniquePtr retVal(versionInfoPtr);

	versionInfoPtr->SetServiceTypeId(serviceTypeId);
	versionInfoPtr->SetOperatingSystem(operatingSystem);
	versionInfoPtr->SetVersion(version);
	versionInfoPtr->SetPath(path);
	versionInfoPtr->SetUploadDate(QDateTime::currentDateTimeUtc());

	if (serviceVersionDataRepresentation.id && !serviceVersionDataRepresentation.id->isEmpty()){
		newObjectId = *serviceVersionDataRepresentation.id;
	}
	else{
		newObjectId = QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
	}

	return retVal;
}


// reimplemented (imtservergql::CObjectCollectionControllerCompBase)

QJsonObject CServiceVersionCollectionControllerComp::GetObjectListFromRequest(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!m_objectCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to list the service versions. Component reference 'ObjectCollection' was not set");
		SendCriticalMessage(0, errorMessage);

		return QJsonObject();
	}

	// Paging and the client's filter (text filter, sorting), read as by the base implementation.
	iprm::CParamsSet filterParams;
	int offset = 0;
	int count = -1;

	const imtgql::CGqlParamObject* inputParamsPtr = gqlRequest.GetParamObject("input");
	if (inputParamsPtr != nullptr){
		const imtgql::CGqlParamObject* viewParamsPtr = inputParamsPtr->GetParamArgumentObjectPtr("viewParams");
		if (viewParamsPtr != nullptr){
			offset = viewParamsPtr->GetParamArgumentValue("offset").toInt();
			count = viewParamsPtr->GetParamArgumentValue("count").toInt();

			PrepareFilters(gqlRequest, *viewParamsPtr, filterParams);
		}
		else{
			const imtgql::CGqlParamObject* selectionParamsPtr = inputParamsPtr->GetParamArgumentObjectPtr("selectionParams");
			if (selectionParamsPtr != nullptr){
				offset = inputParamsPtr->GetParamArgumentValue("offset").toInt();
				count = inputParamsPtr->GetParamArgumentValue("count").toInt();

				PrepareFilters(gqlRequest, *selectionParamsPtr, filterParams);
			}
		}
	}

	if (count <= 0){
		count = -1;
	}

	offset = qMax(0, offset);

	QString textFilter;
	QByteArray sortingFieldId;
	imtbase::IComplexCollectionFilter::SortingOrder sortingOrder = imtbase::IComplexCollectionFilter::SO_NO_ORDER;

	iprm::TParamsPtr<imtbase::IComplexCollectionFilter> complexFilterPtr(&filterParams, "ComplexFilter");
	if (complexFilterPtr.IsValid()){
		textFilter = complexFilterPtr->GetTextFilter().trimmed();

		const imtbase::IComplexCollectionFilter::Fields& fields = complexFilterPtr->GetFields();
		for (const imtbase::IComplexCollectionFilter::FieldInfo& fieldInfo : fields){
			if (fieldInfo.metaInfo.sortingOrder != imtbase::IComplexCollectionFilter::SO_NO_ORDER){
				sortingFieldId = fieldInfo.id;
				sortingOrder = fieldInfo.metaInfo.sortingOrder;

				break;
			}
		}
	}

	// The versions of the requested service type (and operating system family, if one is requested) only;
	// the text filter applies to the version.
	const QString operatingSystem = GetOperatingSystem(gqlRequest.GetRequestContext());
	QList<VersionElement> elements = GetVersionElements(
				GetServiceTypeId(gqlRequest.GetRequestContext()),
				operatingSystem.isEmpty() ? nullptr : &operatingSystem);
	if (!textFilter.isEmpty()){
		elements.erase(
					std::remove_if(elements.begin(), elements.end(), [&textFilter](const VersionElement& element){
						return !element.version.contains(textFilter, Qt::CaseInsensitive);
					}),
					elements.end());
	}

	// Sorted by the operating system, by the version or by the upload date (by default, the latest first).
	const bool sortByOperatingSystem = (sortingFieldId == QByteArrayLiteral("operatingSystem"));
	const bool sortByVersion = (sortingFieldId == QByteArrayLiteral("version"));
	const bool ascending = (sortingOrder == imtbase::IComplexCollectionFilter::SO_ASC);
	std::stable_sort(elements.begin(), elements.end(), [sortByOperatingSystem, sortByVersion, ascending](const VersionElement& element1, const VersionElement& element2){
		int result = sortByOperatingSystem ? element1.operatingSystem.compare(element2.operatingSystem, Qt::CaseInsensitive) : 0;
		if (result == 0 && (sortByOperatingSystem || sortByVersion)){
			result = CompareVersions(element1.version, element2.version);
		}
		if (result == 0){
			result = (element1.uploadDate < element2.uploadDate) ? -1 : ((element2.uploadDate < element1.uploadDate) ? 1 : 0);
		}

		return ascending ? (result < 0) : (result > 0);
	});

	const int elementsCount = elements.count();

	imtbase::ICollectionInfo::Ids pageIds;
	const int pageEnd = (count < 0) ? elementsCount : qMin(elementsCount, offset + count);
	for (int i = offset; i < pageEnd; ++i){
		pageIds << elements[i].id;
	}

	int pagesCount = (count > 0) ? int(std::ceil(elementsCount / double(count))) : 1;
	if (pagesCount <= 0){
		pagesCount = 1;
	}

	QJsonObject notificationObj;
	notificationObj.insert(QStringLiteral("pagesCount"), pagesCount);
	notificationObj.insert(QStringLiteral("totalCount"), elementsCount);

	QJsonObject dataObj;
	dataObj.insert(QStringLiteral("notification"), notificationObj);

	const GqlItemSetupContext setupContext = CreateGqlItemSetupContext(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return QJsonObject();
	}

	QJsonArray itemsArray;

	CElementListIterator iterator(*m_objectCollectionCompPtr, pageIds, elementsCount);
	while (iterator.Next()){
		QJsonObject itemObj;
		if (!SetupGqlItemWithContext(gqlRequest, setupContext, itemObj, &iterator, errorMessage)){
			SendWarningMessage(0, errorMessage, "CServiceVersionCollectionControllerComp");
			errorMessage.clear();

			continue;
		}

		itemsArray.append(itemObj);
	}

	dataObj.insert(QStringLiteral("items"), itemsArray);

	QJsonObject rootObj;
	rootObj.insert(QStringLiteral("data"), dataObj);

	return rootObj;
}


// private methods

QByteArray CServiceVersionCollectionControllerComp::GetServiceTypeId(const imtgql::IGqlContext* gqlContextPtr)
{
	if (gqlContextPtr == nullptr){
		return QByteArray();
	}

	// Header names are stored in lower case (see imtgql::CGqlRequest::GetHeader).
	return gqlContextPtr->GetHeaders().value(QByteArrayLiteral("servicetypeid"));
}


QString CServiceVersionCollectionControllerComp::GetOperatingSystem(const imtgql::IGqlContext* gqlContextPtr)
{
	if (gqlContextPtr == nullptr){
		return QString();
	}

	// Header names are stored in lower case (see imtgql::CGqlRequest::GetHeader).
	return QString::fromUtf8(gqlContextPtr->GetHeaders().value(QByteArrayLiteral("operatingsystem"))).trimmed();
}


QString CServiceVersionCollectionControllerComp::GetAgentOperatingSystem(const QByteArray& agentId) const
{
	// The agent reports its OS with every announce; the enrollment store keeps the latest value.
	if (!m_enrollmentControllerCompPtr.IsValid()){
		return QString();
	}

	return CServiceUpdateControllerComp::GetOperatingSystemFamily(m_enrollmentControllerCompPtr->Get(agentId).os);
}


QStringList CServiceVersionCollectionControllerComp::GetVersionAgentNames(const QByteArray& serviceTypeId, const QString& version, const QString& operatingSystem) const
{
	QStringList retVal;

	if (serviceTypeId.isEmpty() || version.isEmpty() || !m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		return retVal;
	}

	const QString serviceType = QString::fromUtf8(serviceTypeId);

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		// A version of one operating system is installed on the agents of that operating system only.
		if (!operatingSystem.isEmpty() && GetAgentOperatingSystem(agentId) != operatingSystem){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			QString type = serviceInfoPtr->GetServiceTypeId();
			if (type.isEmpty()){
				type = serviceInfoPtr->GetServiceName();
			}

			if (type == serviceType && serviceInfoPtr->GetServiceVersion() == version){
				retVal << m_agentCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();

				break;
			}
		}
	}

	retVal.sort(Qt::CaseInsensitive);

	return retVal;
}


QList<CServiceVersionCollectionControllerComp::VersionElement> CServiceVersionCollectionControllerComp::GetVersionElements(
			const QByteArray& serviceTypeId,
			const QString* operatingSystemPtr) const
{
	QList<VersionElement> retVal;

	if (!m_objectCollectionCompPtr.IsValid()){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids elementIds = m_objectCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId : elementIds){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (!m_objectCollectionCompPtr->GetObjectData(elementId, dataPtr)){
			continue;
		}

		const agentinodata::IServiceVersionInfo* versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(dataPtr.GetPtr());
		if (versionInfoPtr == nullptr || versionInfoPtr->GetServiceTypeId() != serviceTypeId){
			continue;
		}

		if (operatingSystemPtr != nullptr && versionInfoPtr->GetOperatingSystem().compare(*operatingSystemPtr, Qt::CaseInsensitive) != 0){
			continue;
		}

		VersionElement element;
		element.id = elementId;
		element.operatingSystem = versionInfoPtr->GetOperatingSystem();
		element.version = versionInfoPtr->GetVersion();
		element.uploadDate = versionInfoPtr->GetUploadDate();

		retVal << element;
	}

	return retVal;
}


int CServiceVersionCollectionControllerComp::CompareVersions(const QString& version1, const QString& version2)
{
	const QStringList parts1 = version1.split(QLatin1Char('.'));
	const QStringList parts2 = version2.split(QLatin1Char('.'));

	const int partsCount = qMax(parts1.count(), parts2.count());
	for (int i = 0; i < partsCount; ++i){
		const QString part1 = (i < parts1.count()) ? parts1[i] : QString();
		const QString part2 = (i < parts2.count()) ? parts2[i] : QString();

		bool isNumber1 = false;
		bool isNumber2 = false;
		const qulonglong number1 = part1.toULongLong(&isNumber1);
		const qulonglong number2 = part2.toULongLong(&isNumber2);

		int result = 0;
		if (isNumber1 && isNumber2){
			result = (number1 < number2) ? -1 : ((number1 > number2) ? 1 : 0);
		}
		else{
			result = part1.compare(part2, Qt::CaseInsensitive);
		}

		if (result != 0){
			return result;
		}
	}

	return 0;
}


// public methods of the embedded class CElementListIterator

CServiceVersionCollectionControllerComp::CElementListIterator::CElementListIterator(
			const imtbase::IObjectCollection& collection,
			const imtbase::ICollectionInfo::Ids& ids,
			int elementsCount)
	:m_collection(collection),
	m_ids(ids),
	m_elementsCount(elementsCount),
	m_index(-1)
{
}


// reimplemented (imtbase::IObjectCollectionIterator)

bool CServiceVersionCollectionControllerComp::CElementListIterator::Next() const
{
	if (m_index < m_ids.count() - 1){
		++m_index;

		return true;
	}

	return false;
}


bool CServiceVersionCollectionControllerComp::CElementListIterator::Previous() const
{
	if (m_index > 0){
		--m_index;

		return true;
	}

	return false;
}


QByteArray CServiceVersionCollectionControllerComp::CElementListIterator::GetObjectId() const
{
	return IsValid() ? m_ids[m_index] : QByteArray();
}


QByteArray CServiceVersionCollectionControllerComp::CElementListIterator::GetObjectTypeId() const
{
	return IsValid() ? m_collection.GetObjectTypeId(m_ids[m_index]) : QByteArray();
}


bool CServiceVersionCollectionControllerComp::CElementListIterator::GetObjectData(imtbase::IObjectCollection::DataPtr& dataPtr) const
{
	return IsValid() && m_collection.GetObjectData(m_ids[m_index], dataPtr);
}


idoc::MetaInfoPtr CServiceVersionCollectionControllerComp::CElementListIterator::GetDataMetaInfo() const
{
	return IsValid() ? m_collection.GetDataMetaInfo(m_ids[m_index]) : idoc::MetaInfoPtr();
}


int CServiceVersionCollectionControllerComp::CElementListIterator::GetElementsCount() const
{
	return m_elementsCount;
}


idoc::MetaInfoPtr CServiceVersionCollectionControllerComp::CElementListIterator::GetCollectionMetaInfo() const
{
	return IsValid() ? m_collection.GetElementMetaInfo(m_ids[m_index]) : idoc::MetaInfoPtr();
}


QVariant CServiceVersionCollectionControllerComp::CElementListIterator::GetElementInfo(int infoType) const
{
	return IsValid() ? m_collection.GetElementInfo(m_ids[m_index], infoType) : QVariant();
}


QVariant CServiceVersionCollectionControllerComp::CElementListIterator::GetElementInfo(QByteArray /*infoId*/) const
{
	return QVariant();
}


// private methods of the embedded class CElementListIterator

bool CServiceVersionCollectionControllerComp::CElementListIterator::IsValid() const
{
	return (m_index >= 0) && (m_index < m_ids.count());
}


} // namespace agentinogql
