// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CMirroredServiceCollectionControllerComp.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Standard includes
#include <cmath>

// Qt includes
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

// ImtCore includes
#include <iqt/iqt.h>
#include <imtservice/CUrlConnectionParam.h>
#include <imtservice/CUrlConnectionLinkParam.h>

// Agentino includes
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/ServiceEndpointId.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>
#include <agentinodata/agentinodata.h>


namespace agentinogql
{


namespace
{


const char* const c_translationContext = "agentinogql::CMirroredServiceCollectionControllerComp";


/** "host (http: 7222; ws: 7223)" — the form used across the Info panel. */
QString FormatEndpoint(const imtcom::IServerConnectionInterface& connection)
{
	return QStringLiteral("%1 (http: %2; ws: %3)")
				.arg(connection.GetHost())
				.arg(connection.GetPort(imtcom::IServerConnectionInterface::PT_HTTP))
				.arg(connection.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET));
}


/**
	Join non-empty unique lines for a meta-info field, or return \a emptyPlaceholder
	when nothing useful would be shown (avoids blank "Extern Paths:" rows).
*/
QString FormatMetaInfoLines(const QStringList& values, const QString& emptyPlaceholder)
{
	QStringList cleaned;
	cleaned.reserve(values.size());
	for (const QString& value : values){
		const QString trimmed = value.trimmed();
		if (!trimmed.isEmpty() && !cleaned.contains(trimmed)){
			cleaned << trimmed;
		}
	}

	if (cleaned.isEmpty()){
		return emptyPlaceholder;
	}

	return cleaned.join(QLatin1Char('\n'));
}


/**
	First input connection of \a serviceInfo, or an invalid pointer when it publishes none.
	Returns the owning DataPtr (not a bare pointer into it) — the caller must keep it alive
	for as long as it dereferences the returned connection (see GetConsumersOfConnection).
*/
imtbase::IObjectCollection::DataPtr GetFirstInputConnectionData(agentinodata::IServiceInfo& serviceInfo)
{
	imtbase::IObjectCollection* inputConnectionsPtr = serviceInfo.GetInputConnections();
	if (inputConnectionsPtr == nullptr){
		return imtbase::IObjectCollection::DataPtr();
	}

	const imtbase::ICollectionInfo::Ids connectionIds = inputConnectionsPtr->GetElementIds();
	if (connectionIds.isEmpty()){
		return imtbase::IObjectCollection::DataPtr();
	}

	imtbase::IObjectCollection::DataPtr connectionDataPtr;
	inputConnectionsPtr->GetObjectData(connectionIds.first(), connectionDataPtr);
	return connectionDataPtr;
}


/** Ids of the endpoints that \a serviceInfo consumes (its outgoing links). */
QByteArrayList GetConsumedEndpointIds(agentinodata::IServiceInfo& serviceInfo)
{
	QByteArrayList retVal;

	imtbase::IObjectCollection* linksPtr = serviceInfo.GetDependantServiceConnections();
	if (linksPtr == nullptr){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids linkIds = linksPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& linkId : linkIds){
		imtbase::IObjectCollection::DataPtr linkDataPtr;
		if (!linksPtr->GetObjectData(linkId, linkDataPtr)){
			continue;
		}

		const imtservice::CUrlConnectionLinkParam* linkPtr =
					dynamic_cast<const imtservice::CUrlConnectionLinkParam*>(linkDataPtr.GetPtr());
		if (linkPtr != nullptr){
			retVal << linkPtr->GetDependantServiceConnectionId();
		}
	}

	return retVal;
}


} // namespace


// protected methods

sdl::V1_0::imtbase::CGetElementMetaInfoPayload CMirroredServiceCollectionControllerComp::OnGetElementMetaInfo(
			const sdl::V1_0::imtbase::CGetElementMetaInfoGqlRequest& getElementMetaInfoRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::imtbase::CGetElementMetaInfoPayload response;

	const sdl::V1_0::imtbase::GetElementMetaInfoRequestArguments arguments =
				getElementMetaInfoRequest.GetRequestedArguments();
	if (!arguments.input || !arguments.input->elementId){
		return response;
	}

	const QByteArray serviceId = *arguments.input->elementId;
	const QByteArray agentId = gqlRequest.GetHeader("clientid");

	imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);
	if (serviceCollectionPtr == nullptr){
		return response;
	}

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
		return response;
	}

	agentinodata::IServiceInfo* serviceInfoPtr =
				dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
	if (serviceInfoPtr == nullptr){
		return response;
	}

	QList<sdl::V1_0::imtbase::CParameter> infoParams;

	// What this service publishes, who consumes it, and where it is reachable from outside.
	imtbase::IObjectCollection* inputConnectionsPtr = serviceInfoPtr->GetInputConnections();
	const imtbase::ICollectionInfo::Ids publishedIds =
				(inputConnectionsPtr != nullptr) ? inputConnectionsPtr->GetElementIds() : imtbase::ICollectionInfo::Ids();
	if (!publishedIds.isEmpty()){
		QStringList publishedEndpoints;
		QStringList consumers;
		QStringList externPaths;

		for (const imtbase::ICollectionInfo::Id& connectionId : publishedIds){
			imtbase::IObjectCollection::DataPtr connectionDataPtr;
			if (!inputConnectionsPtr->GetObjectData(connectionId, connectionDataPtr)){
				continue;
			}

			const imtservice::CUrlConnectionParam* connectionPtr =
						dynamic_cast<const imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
			if (connectionPtr == nullptr){
				continue;
			}

			publishedEndpoints << FormatEndpoint(*connectionPtr);
			// Consumer links store the full endpoint id, so that is what must be searched.
			consumers << GetConsumersOfConnection(
						agentinodata::ServiceEndpointId::Make(serviceId, connectionId));

			const imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections =
						connectionPtr->GetIncomingConnections();
			for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incoming : incomingConnections){
				consumers << GetConsumersOfConnection(
							agentinodata::ServiceEndpointId::Make(serviceId, incoming.GetObjectUuid()));

				QUrl url;
				if (incoming.GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, url) && url.isValid()){
					externPaths << url.toDisplayString();
				}
			}
		}

		sdl::V1_0::imtbase::CParameter incomingParameter;
		incomingParameter.id = QByteArrayLiteral("IncomingConnections");
		incomingParameter.typeId = incomingParameter.id;
		incomingParameter.name = Translate(gqlRequest, QT_TR_NOOP("Incoming Connections"));
		incomingParameter.data = FormatMetaInfoLines(
					publishedEndpoints,
					Translate(gqlRequest, QT_TR_NOOP("No Incoming Connections")));
		infoParams << incomingParameter;

		sdl::V1_0::imtbase::CParameter externPathsParameter;
		externPathsParameter.id = QByteArrayLiteral("ExternPaths");
		externPathsParameter.typeId = externPathsParameter.id;
		externPathsParameter.name = Translate(gqlRequest, QT_TR_NOOP("Extern Paths"));
		// Empty join used to ship "" and look broken in MetaInfo; show an explicit empty state.
		externPathsParameter.data = FormatMetaInfoLines(
					externPaths,
					Translate(gqlRequest, QT_TR_NOOP("No Extern Paths")));
		infoParams << externPathsParameter;

		// Always list the field so the Info panel layout stays stable when nothing depends
		// on this service's ports yet.
		sdl::V1_0::imtbase::CParameter consumersParameter;
		consumersParameter.id = QByteArrayLiteral("DependantConnections");
		consumersParameter.typeId = consumersParameter.id;
		consumersParameter.name = Translate(gqlRequest, QT_TR_NOOP("Dependant services on port"));
		consumersParameter.data = FormatMetaInfoLines(
					consumers,
					Translate(gqlRequest, QT_TR_NOOP("No Dependant Services")));
		infoParams << consumersParameter;
	}

	// What this service consumes — resolved across all agents.
	const QByteArrayList consumedEndpointIds = GetConsumedEndpointIds(*serviceInfoPtr);
	if (!consumedEndpointIds.isEmpty()){
		QStringList dependencies;
		for (const QByteArray& endpointId : consumedEndpointIds){
			const istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> connectionPtr =
						FindInputConnection(endpointId);
			if (connectionPtr.IsValid()){
				dependencies << FormatEndpoint(*connectionPtr);
			}
		}

		sdl::V1_0::imtbase::CParameter dependsParameter;
		dependsParameter.id = QByteArrayLiteral("ServiceDependencies");
		dependsParameter.typeId = dependsParameter.id;
		dependsParameter.name = Translate(gqlRequest, QT_TR_NOOP("Service depends on"));
		// Prefer a distinct id from IncomingConnections so MetaInfo can keep both rows.
		dependsParameter.data = FormatMetaInfoLines(
					dependencies,
					Translate(gqlRequest, QT_TR_NOOP("No Service Dependencies")));
		infoParams << dependsParameter;
	}

	// Running, but a dependency is not: explain why it still cannot do its job.
	const agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus =
				m_serviceCompositeInfoCompPtr->GetServiceStatus(serviceId);
	const agentinodata::IServiceCompositeInfo::StateOfRequiredServices requiredState =
				m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(serviceId);
	if (serviceStatus == agentinodata::IServiceStatusInfo::SS_RUNNING
				&& requiredState != agentinodata::IServiceCompositeInfo::SORS_RUNNING){
		const QStringList statusInfo = GetDependantStatusInfo(serviceId);
		sdl::V1_0::imtbase::CParameter informationParameter;
		informationParameter.id = QByteArrayLiteral("Information");
		informationParameter.typeId = informationParameter.id;
		informationParameter.name = Translate(gqlRequest, QT_TR_NOOP("Information"));
		// Prefer a single "; "-joined line for multiple reasons (legacy format).
		const QString statusText = FormatMetaInfoLines(
					statusInfo,
					Translate(gqlRequest, QT_TR_NOOP("No Additional Information")));
		informationParameter.data = statusText.contains(QLatin1Char('\n'))
					? statusText.split(QLatin1Char('\n')).join(QStringLiteral("; "))
					: statusText;
		infoParams << informationParameter;
	}

	sdl::V1_0::imtbase::CElementMetaInfo elementMetaInfo;
	elementMetaInfo.infoParams.Emplace();
	elementMetaInfo.infoParams->FromList(infoParams);
	response.elementMetaInfo = elementMetaInfo;

	return response;
}


// reimplemented (agentgql::CServiceCollectionControllerComp)

sdl::V1_0::imtbase::CVisualStatus CMirroredServiceCollectionControllerComp::OnGetObjectVisualStatus(
			const sdl::V1_0::imtbase::CGetObjectVisualStatusGqlRequest& getObjectVisualStatusRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::imtbase::CVisualStatus retVal = BaseClass::OnGetObjectVisualStatus(getObjectVisualStatusRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return sdl::V1_0::imtbase::CVisualStatus();
	}

	sdl::V1_0::imtbase::CVisualStatus& response = retVal;

	// The inherited implementation (agentgql::CServiceCollectionControllerComp) reads the
	// service straight out of m_objectCollectionCompPtr, which on the agent holds the services
	// themselves — but here it holds *agent records* (see class doc), so that lookup always
	// misses and text/description stay empty. Look the service up in this agent's mirror
	// instead, the same way every other read in this class does.
	if (response.objectId){
		const QByteArray agentId = gqlRequest.GetHeader("clientid");
		imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);
		if (serviceCollectionPtr != nullptr){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (serviceCollectionPtr->GetObjectData(*response.objectId, serviceDataPtr)){
				const agentinodata::IServiceInfo* serviceInfoPtr =
							dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
				if (serviceInfoPtr != nullptr){
					response.text = serviceInfoPtr->GetServiceName();
					response.description = serviceInfoPtr->GetServiceDescription();
				}
			}
		}
	}

	return retVal;
}


// reimplemented (imtgql::CObjectCollectionControllerCompBase)

bool CMirroredServiceCollectionControllerComp::SetupGqlItem(
		const imtgql::CGqlRequest& gqlRequest,
		QJsonObject& itemObj,
		const QByteArray& collectionId,
		QString& errorMessage) const
{
	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceCompositeInfoCompPtr.IsValid()){
		Q_ASSERT(false);

		return false;
	}

	const QByteArrayList informationIds = GetInformationIds(gqlRequest, "items");
	if (informationIds.isEmpty()){
		errorMessage = "Unable to get object data from object collection";

		return false;
	}

	const QByteArray agentId = gqlRequest.GetHeader("clientid");
	imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);
	if (serviceCollectionPtr == nullptr){
		errorMessage = QStringLiteral("Unable to get list objects. Internal error.");
		SendErrorMessage(0, errorMessage, "CMirroredServiceCollectionControllerComp");

		return false;
	}

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	agentinodata::CServiceInfo* serviceInfoPtr = nullptr;
	if (serviceCollectionPtr->GetObjectData(collectionId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		errorMessage = "Unable to get object data from object collection";

		return false;
	}

	for (const QByteArray& informationId : informationIds){
		QVariant elementInformation;

		if (informationId == "id"){
			elementInformation = collectionId;
		}
		else if (informationId == "typeId"){
			elementInformation = serviceCollectionPtr->GetObjectTypeId(collectionId);
		}
		else if (informationId == "name"){
			elementInformation = serviceInfoPtr->GetServiceName();
		}
		else if (informationId == "description"){
			elementInformation = serviceInfoPtr->GetServiceDescription();
		}
		else if (informationId == "path"){
			elementInformation = QString::fromUtf8(serviceInfoPtr->GetServicePath());
		}
		else if (informationId == "startScript"){
			elementInformation = QString::fromUtf8(serviceInfoPtr->GetStartScriptPath());
		}
		else if (informationId == "stopScript"){
			elementInformation = QString::fromUtf8(serviceInfoPtr->GetStopScriptPath());
		}
		else if (informationId == "settingsPath"){
			elementInformation = QString::fromUtf8(serviceInfoPtr->GetServiceSettingsPath());
		}
		else if (informationId == "arguments"){
			elementInformation = serviceInfoPtr->GetServiceArguments().join(' ');
		}
		else if (informationId == "isAutoStart"){
			elementInformation = serviceInfoPtr->IsAutoStart();
		}
		else if (informationId == "tracingLevel"){
			elementInformation = serviceInfoPtr->GetTracingLevel();
		}
		else if (informationId == "version"){
			elementInformation = serviceInfoPtr->GetServiceVersion();
		}
		else if (informationId == "type"){
			elementInformation = serviceInfoPtr->GetSettingsType() == agentinodata::IServiceInfo::ST_PLUGIN
						? QStringLiteral("ACF")
						: QStringLiteral("None");
		}
		else if (informationId == "status" || informationId == "statusName"){
			// The agent is the source of truth for status; the status collection is the
			// projection CAgentChangeObserver keeps current from the agent's pushes.
			agentinodata::IServiceStatusInfo::ServiceStatus status =
						agentinodata::IServiceStatusInfo::SS_UNDEFINED;

			imtbase::IObjectCollection::DataPtr statusDataPtr;
			if (m_serviceStatusCollectionCompPtr.IsValid()
						&& m_serviceStatusCollectionCompPtr->GetObjectData(collectionId, statusDataPtr)){
				const agentinodata::CServiceStatusInfo* statusInfoPtr =
							dynamic_cast<const agentinodata::CServiceStatusInfo*>(statusDataPtr.GetPtr());
				if (statusInfoPtr != nullptr){
					status = statusInfoPtr->GetServiceStatus();
				}
			}

			const agentinodata::ProcessStateEnum processState =
						agentinodata::GetProcceStateRepresentation(status);
			elementInformation = (informationId == "status") ? processState.id : processState.name;
		}
		else if (informationId == "dependencyStatus"){
			const agentinodata::IServiceCompositeInfo::StateOfRequiredServices state =
						m_serviceCompositeInfoCompPtr->GetStateOfRequiredServices(collectionId);
			elementInformation = agentinodata::IServiceCompositeInfo::ToString(state);
		}
		else if (informationId == "dependantStatusInfo"){
			elementInformation = GetDependantStatusInfo(collectionId).join("; ");
		}

		if (elementInformation.isNull()){
			elementInformation = "";
		}

		itemObj.insert(QString::fromUtf8(informationId), QJsonValue::fromVariant(elementInformation));
	}

	return true;
}


QJsonObject CMirroredServiceCollectionControllerComp::ListObjects(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	const imtgql::CGqlParamObject* inputObjectPtr = gqlRequest.GetParams().GetParamArgumentObjectPtr("input");
	const imtgql::CGqlParamObject* viewParamsPtr =
				(inputObjectPtr != nullptr) ? inputObjectPtr->GetParamArgumentObjectPtr("viewParams") : nullptr;

	int offset = 0;
	int count = -1;
	if (viewParamsPtr != nullptr){
		offset = viewParamsPtr->GetParamArgumentValue("offset").toInt();
		count = viewParamsPtr->GetParamArgumentValue("count").toInt();
	}

	const QByteArray agentId = gqlRequest.GetHeader("clientid");
	imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);

	if (serviceCollectionPtr == nullptr){
		if (agentId.isEmpty()){
			errorMessage = QStringLiteral("Unable to get list objects. Internal error.");
			SendErrorMessage(0, errorMessage, "CMirroredServiceCollectionControllerComp");

			return QJsonObject();
		}

		// Known agent that has not reconciled yet (e.g. right after a server restart):
		// an empty list is the truthful answer, not an error.
		QJsonObject notificationObj;
		notificationObj.insert(QStringLiteral("pagesCount"), 1);
		notificationObj.insert(QStringLiteral("totalCount"), 0);

		QJsonObject dataObj;
		dataObj.insert(QStringLiteral("items"), QJsonArray());
		dataObj.insert(QStringLiteral("notification"), notificationObj);

		QJsonObject rootObj;
		rootObj.insert(QStringLiteral("data"), dataObj);

		return rootObj;
	}

	iprm::CParamsSet filterParams;
	if (viewParamsPtr != nullptr){
		PrepareFilters(gqlRequest, *viewParamsPtr, filterParams);
	}

	const int elementsCount = serviceCollectionPtr->GetElementsCount(&filterParams);
	int pagesCount = (count > 0) ? int(std::ceil(elementsCount / double(count))) : 1;
	if (pagesCount <= 0){
		pagesCount = 1;
	}

	QJsonArray itemsArray;
	const imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds(offset, count, &filterParams);
	for (const imtbase::ICollectionInfo::Id& id : ids){
		QJsonObject itemObj;
		if (!SetupGqlItem(gqlRequest, itemObj, id, errorMessage)){
			SendErrorMessage(0, errorMessage, "CMirroredServiceCollectionControllerComp");

			return QJsonObject();
		}

		itemsArray.append(itemObj);
	}

	QJsonObject notificationObj;
	notificationObj.insert(QStringLiteral("pagesCount"), pagesCount);
	notificationObj.insert(QStringLiteral("totalCount"), elementsCount);

	QJsonObject dataObj;
	dataObj.insert(QStringLiteral("items"), itemsArray);
	dataObj.insert(QStringLiteral("notification"), notificationObj);

	QJsonObject rootObj;
	rootObj.insert(QStringLiteral("data"), dataObj);

	return rootObj;
}


// private methods

imtbase::IObjectCollection* CMirroredServiceCollectionControllerComp::GetMirroredServices(
			const QByteArray& agentId) const
{
	if (!m_serviceManagerCompPtr.IsValid() || agentId.isEmpty()){
		return nullptr;
	}

	return m_serviceManagerCompPtr->GetServiceCollection(agentId);
}


void CMirroredServiceCollectionControllerComp::ForEachMirroredService(
			const std::function<bool(
						const QByteArray& agentId,
						const QString& agentName,
						const QByteArray& serviceId,
						agentinodata::IServiceInfo& serviceInfo)>& visitor) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		return;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		const QString agentName =
					m_agentCollectionCompPtr->GetElementInfo(agentId, imtbase::IObjectCollection::EIT_NAME).toString();

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			agentinodata::IServiceInfo* serviceInfoPtr =
						dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			if (visitor(agentId, agentName, serviceId, *serviceInfoPtr)){
				return;
			}
		}
	}
}


QStringList CMirroredServiceCollectionControllerComp::GetConsumersOfConnection(
			const QByteArray& endpointId) const
{
	QStringList retVal;
	if (endpointId.isEmpty()){
		return retVal;
	}

	ForEachMirroredService(
				[&retVal, &endpointId](
							const QByteArray& /*agentId*/,
							const QString& agentName,
							const QByteArray& /*serviceId*/,
							agentinodata::IServiceInfo& serviceInfo){
					if (!GetConsumedEndpointIds(serviceInfo).contains(endpointId)){
						return false;
					}

					// Label the consumer by where it can itself be reached.
					// Keep the owning DataPtr alive for as long as inputConnectionPtr is used.
					const imtbase::IObjectCollection::DataPtr inputConnectionDataPtr =
								GetFirstInputConnectionData(serviceInfo);
					const imtservice::CUrlConnectionParam* inputConnectionPtr =
								dynamic_cast<const imtservice::CUrlConnectionParam*>(inputConnectionDataPtr.GetPtr());
					if (inputConnectionPtr != nullptr){
						retVal << serviceInfo.GetServiceTypeId()
									+ "@" + agentName
									+ " " + FormatEndpoint(*inputConnectionPtr);
					}

					// Keep scanning: several services may consume the same endpoint.
					return false;
				});

	return retVal;
}


istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> CMirroredServiceCollectionControllerComp::FindInputConnection(
			const QByteArray& endpointId) const
{
	istd::TSharedInterfacePtr<imtcom::IServerConnectionInterface> retVal;

	// The endpoint id names its producing service, so there is no need to scan the
	// fleet: locate that one service in the mirror and pick the addressed connection.
	QByteArray producerServiceId;
	QByteArray connectionId;
	if (!agentinodata::ServiceEndpointId::Parse(endpointId, producerServiceId, connectionId)
				|| !m_agentCollectionCompPtr.IsValid()){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		imtbase::IObjectCollection* serviceCollectionPtr = GetMirroredServices(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!serviceCollectionPtr->GetObjectData(producerServiceId, serviceDataPtr)){
			continue;
		}

		// Producer located — whatever we find (or fail to find) inside it is the answer.
		agentinodata::IServiceInfo* serviceInfoPtr =
					dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			return retVal;
		}

		imtbase::IObjectCollection* inputConnectionsPtr = serviceInfoPtr->GetInputConnections();
		if (inputConnectionsPtr == nullptr){
			return retVal;
		}

		// Direct input-connection element...
		imtbase::IObjectCollection::DataPtr connectionDataPtr;
		if (inputConnectionsPtr->GetObjectData(connectionId, connectionDataPtr)){
			retVal.MoveCastedPtr(connectionDataPtr.GetPtr()->CloneMe());

			return retVal;
		}

		// ...or the uuid of one of its incoming connections (alternative published address).
		const imtbase::ICollectionInfo::Ids inputIds = inputConnectionsPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& inputId : inputIds){
			imtbase::IObjectCollection::DataPtr inputDataPtr;
			if (!inputConnectionsPtr->GetObjectData(inputId, inputDataPtr)){
				continue;
			}

			const imtservice::CUrlConnectionParam* inputParamPtr =
						dynamic_cast<const imtservice::CUrlConnectionParam*>(inputDataPtr.GetPtr());
			if (inputParamPtr == nullptr){
				continue;
			}

			const imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections =
						inputParamPtr->GetIncomingConnections();
			for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incoming : incomingConnections){
				if (incoming.GetObjectUuid() == connectionId){
					retVal.MoveCastedPtr(incoming.CloneMe());

					return retVal;
				}
			}
		}

		return retVal;
	}

	return retVal;
}


QStringList CMirroredServiceCollectionControllerComp::GetDependantStatusInfo(
			const QByteArray& serviceId) const
{
	if (!m_serviceCompositeInfoCompPtr.IsValid()){
		Q_ASSERT(false);

		return QStringList();
	}

	QStringList retVal;

	ForEachMirroredService(
				[this, &retVal, &serviceId](
							const QByteArray& /*agentId*/,
							const QString& /*agentName*/,
							const QByteArray& mirroredServiceId,
							agentinodata::IServiceInfo& serviceInfo){
					if (mirroredServiceId != serviceId){
						return false;
					}

					const QByteArrayList consumedEndpointIds = GetConsumedEndpointIds(serviceInfo);
					for (const QByteArray& endpointId : consumedEndpointIds){
						const QByteArray dependencyId = m_serviceCompositeInfoCompPtr->GetServiceId(endpointId);
						const agentinodata::IServiceStatusInfo::ServiceStatus dependencyStatus =
									m_serviceCompositeInfoCompPtr->GetServiceStatus(dependencyId);

						QString info;
						if (dependencyStatus == agentinodata::IServiceStatusInfo::SS_UNDEFINED){
							info = QString(QT_TR_NOOP("Service status of %1 undefined"));
						}
						else if (dependencyStatus == agentinodata::IServiceStatusInfo::SS_NOT_RUNNING){
							info = QString(QT_TR_NOOP("Service %1 not running"));
						}

						if (info.isEmpty()){
							continue;
						}

						const QString dependencyName =
									m_serviceCompositeInfoCompPtr->GetServiceName(dependencyId)
										+ "@" + m_serviceCompositeInfoCompPtr->GetServiceAgentName(dependencyId);
						info = info.arg(dependencyName);
						if (!retVal.contains(info)){
							retVal << info;
						}
					}

					// The service id is unique across the fleet — no need to keep scanning.
					return true;
				});

	return retVal;
}


QString CMirroredServiceCollectionControllerComp::Translate(
			const imtgql::CGqlRequest& gqlRequest,
			const char* sourceText) const
{
	if (!m_translationManagerCompPtr.IsValid()){
		return QString::fromUtf8(sourceText);
	}

	QByteArray languageId;
	const imtgql::IGqlContext* gqlContextPtr = gqlRequest.GetRequestContext();
	if (gqlContextPtr != nullptr){
		languageId = gqlContextPtr->GetLanguageId();
	}

	return iqt::GetTranslation(
				m_translationManagerCompPtr.GetPtr(),
				sourceText,
				languageId,
				c_translationContext);
}


} // namespace agentinogql
