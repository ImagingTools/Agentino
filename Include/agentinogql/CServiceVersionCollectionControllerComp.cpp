// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceVersionCollectionControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

// ImtCore includes
#include <imtbase/IObjectCollectionIterator.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


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

	const QString version = m_objectCollectionCompPtr->GetElementInfo(objectId, imtbase::ICollectionInfo::EIT_NAME).toString();
	const QStringList agentNames = GetVersionAgentNames(GetServiceTypeId(gqlRequest.GetRequestContext()), version);

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

	if (requestInfo.items.isUploadDateRequested){
		const QDateTime addedTime = objectCollectionIterator.GetElementInfo("Added").toDateTime();
		if (addedTime.isValid()){
			representationObject.uploadDate = addedTime.toLocalTime().toString("dd.MM.yyyy hh:mm:ss");
		}
	}

	if (requestInfo.items.isInstallationCountRequested){
		const QByteArray serviceTypeId = GetServiceTypeId(serviceVersionsListRequest.GetRequestContext());
		representationObject.installationCount = GetVersionAgentNames(serviceTypeId, version).count();
	}

	return true;
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


QStringList CServiceVersionCollectionControllerComp::GetVersionAgentNames(const QByteArray& serviceTypeId, const QString& version) const
{
	QStringList retVal;

	if (serviceTypeId.isEmpty() || version.isEmpty() || !m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		return retVal;
	}

	const QString serviceType = QString::fromUtf8(serviceTypeId);

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
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


} // namespace agentinogql
