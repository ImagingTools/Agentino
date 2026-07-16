// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceControllerProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QDebug>
#include <QtCore/QString>

// ACF includes
#include <istd/CChangeGroup.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>

// ImtCore includes
#include <imtgql/CGqlContext.h>


namespace agentinogql
{


// reimplemented (IServiceCollectionSynchronizer)

bool CServiceControllerProxyComp::SyncServiceInMirror(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			QString& errorMessage)
{
	// Idempotent: proxy write path and agent push may both call this for the same change.
	// ServiceExists ? SetService : AddService converges either way.
	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	// Build a 'GetService' request addressed to the owning agent and fetch the fresh data,
	// exactly as OnAddService does after forwarding an add.
	imtgql::CGqlRequest getGqlRequest;

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	getGqlRequest.SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::GetServiceRequestArguments getArguments;
	getArguments.input.emplace();
	getArguments.input->id = serviceId;

	if (!sdl::V1_0::agentino::CGetServiceGqlRequest::SetupGqlRequest(getGqlRequest, getArguments)){
		errorMessage = QString("Unable to build 'GetService' request for '%1'").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	// SetupGqlRequest does not attach selection set; request full ServiceData fields.
	AppendServiceDataFields(getGqlRequest);

	sdl::V1_0::agentino::CServiceData serviceData = SendModelRequest<sdl::V1_0::agentino::CServiceData>(getGqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	serviceInfoPtr->SetObjectUuid(serviceId);

	QString serviceName = serviceInfoPtr->GetServiceName();
	QString serviceDescription = serviceInfoPtr->GetServiceDescription();
	const QByteArray servicePath = serviceInfoPtr->GetServicePath();

	if (m_serviceManagerCompPtr->ServiceExists(agentId, serviceId)){
		// Notify (beQuiet=false) so GUI receives OnServicesCollectionChanged for agent-side edits.
		// beQuiet=true remains only in OnGetService (cache refresh during a read).
		if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr, serviceName, serviceDescription, false)){
			errorMessage = QString("Unable to update service '%1' in the server mirror").arg(qPrintable(serviceId));
			SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

			return false;
		}
	}
	else{
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

		if (!m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr.PopPtr(), serviceId, serviceName, serviceDescription)){
			errorMessage = QString("Unable to add service '%1' to the server mirror").arg(qPrintable(serviceId));
			SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

			return false;
		}
	}

	// Recreate with a new UUID (same path) must not leave the previous mirror entry behind.
	RemoveStaleMirrorServicesByPath(agentId, serviceId, servicePath);

	return true;
}


bool CServiceControllerProxyComp::RemoveServicesInMirror(
			const QByteArray& agentId,
			const QByteArrayList& serviceIds,
			QString& errorMessage)
{
	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		errorMessage = QString("Unable to remove service(s) '%1' from the server mirror").arg(qPrintable(serviceIds.join(';')));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	RemoveServiceStatuses(serviceIds);

	return true;
}


bool CServiceControllerProxyComp::SyncAgentServicesInMirror(
			const QByteArray& agentId,
			QString& errorMessage)
{
	if (m_agentsBeingReconciled.contains(agentId)){
		// Nested call from SendModelRequest local event loop - skip re-entry for this agent.
		return true;
	}

	if (!m_serviceManagerCompPtr.IsValid()){
		errorMessage = "Attribute 'ServiceManager' was not set";
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	m_agentsBeingReconciled.insert(agentId);

	// Pull the authoritative service id set from the agent.
	imtgql::CGqlRequest listGqlRequest;

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	listGqlRequest.SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::ServicesListRequestArguments listArguments;
	listArguments.input.emplace();

	sdl::V1_0::agentino::ServicesListRequestInfo listInfo;
	// Ids are required for the diff; other fields are optional for reconciliation.
	listInfo.items.isIdRequested = true;

	if (!sdl::V1_0::agentino::CServicesListGqlRequest::SetupGqlRequest(listGqlRequest, listArguments, listInfo)){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QString("Unable to build 'ServicesList' request for agent '%1'").arg(qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	// Critical: SetupGqlRequest ignores requestInfo and attaches no selection set.
	// Without "items { id ... }", the agent ListObjects path returns empty item objects
	// (no ids) and the mirror would never gain services (or would wipe on false empty set).
	AppendServicesListFields(listGqlRequest);

	QString listError;
	sdl::V1_0::agentino::CServiceListPayload listPayload =
			SendModelRequest<sdl::V1_0::agentino::CServiceListPayload>(listGqlRequest, listError);
	if (!listError.isEmpty()){
		// Agent unreachable - do not touch the mirror.
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = listError;
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	QByteArrayList agentServiceIds;
	int rawItemCount = 0;
	if (listPayload.items.has_value()){
		rawItemCount = listPayload.items->size();
		for (const istd::TNullableValue<sdl::V1_0::agentino::CServiceItem>& item : *listPayload.items){
			if (item.has_value() && item->id.has_value() && !item->id->isEmpty()){
				agentServiceIds << *item->id;
			}
		}
	}

	// Protocol safety: items present but no ids → broken selection set / parse. Never wipe mirror.
	if (rawItemCount > 0 && agentServiceIds.isEmpty()){
		m_agentsBeingReconciled.remove(agentId);
		errorMessage = QString(
					"ServicesList for agent '%1' returned %2 item(s) without ids; mirror left unchanged")
								.arg(qPrintable(agentId))
								.arg(rawItemCount);
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	const QByteArrayList mirrorIds = GetMirrorServiceIds(agentId);

	QByteArrayList staleIds;
	for (const QByteArray& mirrorId : mirrorIds){
		if (!agentServiceIds.contains(mirrorId)){
			staleIds << mirrorId;
		}
	}

	// Batch CF_SERVICE_* / status notifications into one GUI poke.
	istd::CChangeGroup serviceChangeGroup(m_serviceManagerCompPtr.GetPtr());
	istd::CChangeGroup statusChangeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	QStringList aggregatedErrors;

	if (!staleIds.isEmpty()){
		QString removeError;
		if (!RemoveServicesInMirror(agentId, staleIds, removeError)){
			aggregatedErrors << removeError;
		}
	}

	for (const QByteArray& serviceId : agentServiceIds){
		QString syncError;
		if (!SyncServiceInMirror(agentId, serviceId, syncError)){
			aggregatedErrors << syncError;
			// Continue: one broken service must not abort the full reconcile.
		}
	}

	m_agentsBeingReconciled.remove(agentId);

	if (!aggregatedErrors.isEmpty()){
		errorMessage = aggregatedErrors.join("; ");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return false;
	}

	errorMessage.clear();

	return true;
}


// protected methods

sdl::V1_0::agentino::CServiceData CServiceControllerProxyComp::OnGetService(
			const sdl::V1_0::agentino::CGetServiceGqlRequest& getServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceData();
	}

	sdl::V1_0::agentino::GetServiceRequestArguments arguments = getServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceData();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	if (inputParamPtr == nullptr){
		return sdl::V1_0::agentino::CServiceData();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	sdl::V1_0::agentino::CServiceData response = SendModelRequest<sdl::V1_0::agentino::CServiceData>(gqlRequest, errorMessage);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (!serviceInfoPtr.IsValid()){
		if (errorMessage.isEmpty()){
			errorMessage = QString("Unable to get service '%1'. Error: Service not exists").arg(qPrintable(serviceId));
		}
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CServiceData();
	}

	if (!errorMessage.isEmpty()){
		// The Agent may no longer have this service (e.g. it was removed there directly and
		// the mirror was never reconciled) - fall back to the last known mirror representation
		// so the item stays viewable/removable instead of becoming permanently stuck.
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		errorMessage.clear();

		if (!agentinodata::GetRepresentationFromService(response, *serviceInfoPtr.GetPtr())){
			errorMessage = QString("Unable to get service '%1'. Error: Service is unavailable on the agent").arg(qPrintable(serviceId));
			SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

			return sdl::V1_0::agentino::CServiceData();
		}

		response.id = serviceId;

		return response;
	}

	// Syncronise service with agent
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, response, errorMessage)){
		errorMessage = QString("Unable to get service '%1'. Error: %2").arg(qPrintable(serviceId), errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CServiceData();
	}

	if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr, "", "", true)){
		errorMessage = QString("Unable to set service '%1'. Error: Set service to collection failed").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CServiceData();
	}

	if (response.outputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::COutputConnection>> outputConnections = *response.outputConnections;
		for (istd::TNullableValue<sdl::V1_0::agentino::COutputConnection>& outputConnection : *outputConnections){
			QByteArray id = *outputConnection->id;
			outputConnection->dependantConnectionList = GetConnectionsModel(id);

			istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr = GetDependantServerConnectionParam(id);
			if (serverConnectionInterfacePtr.IsValid()){
				sdl::V1_0::imtbase::CServerConnectionParam urlRepresentation;
				if (agentinodata::GetRepresentationFromServerConnectionParam(*serverConnectionInterfacePtr.GetPtr(), urlRepresentation)){
					outputConnection->connectionParam = urlRepresentation;
				}
			}
		}

		response.outputConnections = outputConnections;
	}

	return response;
}


sdl::V1_0::imtbase::CUpdatedNotificationPayload CServiceControllerProxyComp::OnUpdateService(
			const sdl::V1_0::agentino::CUpdateServiceGqlRequest& updateServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	sdl::V1_0::agentino::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::agentino::CServiceData serviceData;
	if (arguments.input->item){
		serviceData = *arguments.input->item;
	}

	sdl::V1_0::imtbase::CUpdatedNotificationPayload retVal =
			SendModelRequest<sdl::V1_0::imtbase::CUpdatedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (!serviceInfoPtr.IsValid()){
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	// Обновление всех сервисов зависимых от портов текущего
	sdl::V1_0::agentino::CServiceData currentServiceData;
	if (agentinodata::GetRepresentationFromService(currentServiceData, *serviceInfoPtr.GetPtr())){
		QByteArrayList connectionIds = GetChangedConnectionUrl(currentServiceData, serviceData);

		for (const QByteArray& connectionId : connectionIds){
			sdl::V1_0::imtbase::CServerConnectionParam newConnectionParam = GetServerConnectionParam(serviceData, connectionId);
			QByteArrayList dependentServiceIds = GetServiceIdsByDependantId(connectionId);
			for (const QByteArray& dependantServiceId : dependentServiceIds){
				UpdateConnectionForService(dependantServiceId, agentId, connectionId, newConnectionParam);
			}
		}
	}

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr.GetPtr(), serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	QString serviceName;
	QString serviceDescription;

	if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr.PopPtr(), serviceName, serviceDescription)){
		return sdl::V1_0::imtbase::CUpdatedNotificationPayload();
	}

	return retVal;
}


sdl::V1_0::imtbase::CAddedNotificationPayload CServiceControllerProxyComp::OnAddService(
			const sdl::V1_0::agentino::CAddServiceGqlRequest& addServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	sdl::V1_0::agentino::AddServiceRequestArguments arguments = addServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input->id){
		serviceId = *arguments.input->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::imtbase::CAddedNotificationPayload retVal = SendModelRequest<sdl::V1_0::imtbase::CAddedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	// Prefer the id returned by the agent when the client did not supply one.
	if (serviceId.isEmpty() && retVal.id.has_value()){
		serviceId = *retVal.id;
	}

	// Authoritative snapshot from the agent (connections, ports, paths). Do NOT build the
	// mirror from the AddService input only — that payload is often incomplete and left
	// CServerConnectionInterfaceParam maps in a state that crashed AutoPersistence serialize.
	QString syncError;
	if (!const_cast<CServiceControllerProxyComp*>(this)->SyncServiceInMirror(agentId, serviceId, syncError)){
		errorMessage = syncError;
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	return retVal;
}


// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnStartService(
			const sdl::V1_0::agentino::CStartServiceGqlRequest& startServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::StartServiceRequestArguments arguments = startServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	QByteArray serviceId;
	if (arguments.input->serviceId){
		serviceId = *arguments.input->serviceId;
	}

	istd::CChangeGroup changeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_STARTING);

	sdl::V1_0::agentino::CServiceStatusResponse retVal = SendModelRequest<sdl::V1_0::agentino::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);

		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	if (retVal.status.has_value()){
		SetServiceStatus(serviceId, *retVal.status);
	}

	return retVal;
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnStopService(
			const sdl::V1_0::agentino::CStopServiceGqlRequest& stopServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::StopServiceRequestArguments arguments = stopServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	QByteArray serviceId;
	if (arguments.input->serviceId){
		serviceId = *arguments.input->serviceId;
	}

	istd::CChangeGroup changeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_STOPPING);

	sdl::V1_0::agentino::CServiceStatusResponse retVal = SendModelRequest<sdl::V1_0::agentino::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	if (retVal.status.has_value()){
		SetServiceStatus(serviceId, *retVal.status);
	}

	return retVal;
}


sdl::V1_0::imtbase::CRemovedNotificationPayload CServiceControllerProxyComp::OnServicesRemove(
			const sdl::V1_0::agentino::CServicesRemoveGqlRequest& removeServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	sdl::V1_0::agentino::ServicesRemoveRequestArguments arguments = removeServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	QByteArrayList serviceIds;
	if (arguments.input->elementIds){
		serviceIds = (*arguments.input->elementIds).ToList();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::V1_0::imtbase::CRemovedNotificationPayload retVal = SendModelRequest<sdl::V1_0::imtbase::CRemovedNotificationPayload>(gqlRequest, errorMessage);
	bool agentForwardFailed = !errorMessage.isEmpty();
	if (agentForwardFailed){
		// The Agent may already be missing this service - still purge it from the mirror
		// below instead of leaving a stuck entry that can neither be opened nor removed.
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		errorMessage.clear();
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		errorMessage = QString("Unable to remove service '%1' from agent '%2'").arg(qPrintable(serviceIds.join(';')), qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	RemoveServiceStatuses(serviceIds);

	errorMessage.clear();

	if (agentForwardFailed){
		imtsdl::TElementList<QByteArray> removedIds;
		for (const QByteArray& serviceId: serviceIds){
			removedIds << serviceId;
		}
		retVal.elementIds = removedIds;
	}

	return retVal;
}


sdl::V1_0::imtbase::CRemoveElementsPayload CServiceControllerProxyComp::OnRemoveElements(
			const sdl::V1_0::imtbase::CRemoveElementsGqlRequest& removeElementsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::imtbase::CRemoveElementsPayload response;
	response.success = false;

	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return response;
	}

	sdl::V1_0::imtbase::RemoveElementsRequestArguments arguments = removeElementsRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->elementIds.has_value()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		return response;
	}

	QByteArrayList serviceIds = (*arguments.input->elementIds).ToList();
	if (serviceIds.isEmpty()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		return response;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	// Forward the original 'RemoveElements' request to the owning Agent unchanged - the
	// Agent's ServiceCollectionController owns the real 'ServiceRepository' collection and
	// handles this generic command correctly (unlike the server-side local mirror).
	sdl::V1_0::imtbase::CRemoveElementsPayload retVal = SendModelRequest<sdl::V1_0::imtbase::CRemoveElementsPayload>(gqlRequest, errorMessage);
	bool agentForwardFailed = !errorMessage.isEmpty();
	if (agentForwardFailed){
		// The Agent may already be missing this service (e.g. it was removed there directly,
		// and the mirror was never reconciled) - still purge it from the mirror below instead
		// of leaving a stuck entry that can neither be opened nor removed.
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		errorMessage.clear();
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		errorMessage = QString("Unable to remove service '%1' from agent '%2'").arg(qPrintable(serviceIds.join(';')), qPrintable(agentId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return response;
	}

	RemoveServiceStatuses(serviceIds);

	errorMessage.clear();

	if (agentForwardFailed){
		retVal.success = true;
	}

	return retVal;
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerProxyComp::OnGetServiceStatus(
			const sdl::V1_0::agentino::CGetServiceStatusGqlRequest& getServiceStatusRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	sdl::V1_0::agentino::CServiceStatusResponse retVal = SendModelRequest<sdl::V1_0::agentino::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CServiceStatusResponse();
	}

	return retVal;
}


sdl::V1_0::agentino::CUpdateConnectionUrlResponse CServiceControllerProxyComp::OnUpdateConnectionUrl(
			const sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest& /*updateConnectionUrlRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	return sdl::V1_0::agentino::CUpdateConnectionUrlResponse();
}


sdl::V1_0::agentino::CPluginInfo CServiceControllerProxyComp::OnLoadPlugin(
			const sdl::V1_0::agentino::CLoadPluginGqlRequest& loadPluginRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

		return sdl::V1_0::agentino::CPluginInfo();
	}

	sdl::V1_0::agentino::CPluginInfo retVal = SendModelRequest<sdl::V1_0::agentino::CPluginInfo>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::agentino::CPluginInfo();
	}

	// Update dependant connections elements
	if (retVal.outputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::COutputConnection>> outputConnections = *retVal.outputConnections;
		for (istd::TNullableValue<sdl::V1_0::agentino::COutputConnection>& outputConnection : *outputConnections){
			QByteArray id = *outputConnection->id;
			outputConnection->dependantConnectionList = GetConnectionsModel(id);
		}

		retVal.outputConnections = outputConnections;
	}

	return retVal;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerProxyComp::OnGetServiceSettings(
			const sdl::V1_0::agentino::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::GetServiceSettingsRequestArguments arguments = getServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	sdl::V1_0::agentino::CServiceSettingsPayload retVal = SendModelRequest<sdl::V1_0::agentino::CServiceSettingsPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	return retVal;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerProxyComp::OnUpdateServiceSettings(
			const sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::UpdateServiceSettingsRequestArguments arguments = updateServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	sdl::V1_0::agentino::CServiceSettingsPayload retVal = SendModelRequest<sdl::V1_0::agentino::CServiceSettingsPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");
		return sdl::V1_0::agentino::CServiceSettingsPayload();
	}

	return retVal;
}


// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)

QJsonObject CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	if (sdl::V1_0::agentino::CGetServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CGetServiceGqlRequest,
			sdl::V1_0::agentino::CServiceData>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CUpdateServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CUpdateServiceGqlRequest,
			sdl::V1_0::imtbase::CUpdatedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CAddServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CAddServiceGqlRequest,
			sdl::V1_0::imtbase::CAddedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnAddService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CStartServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CStartServiceGqlRequest,
			sdl::V1_0::agentino::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStartService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CStopServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CStopServiceGqlRequest,
			sdl::V1_0::agentino::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStopService(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CServicesRemoveGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CServicesRemoveGqlRequest,
			sdl::V1_0::imtbase::CRemovedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnServicesRemove(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::imtbase::CRemoveElementsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::imtbase::CRemoveElementsGqlRequest,
			sdl::V1_0::imtbase::CRemoveElementsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnRemoveElements(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CLoadPluginGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CLoadPluginGqlRequest,
			sdl::V1_0::agentino::CPluginInfo>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnLoadPlugin(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CGetServiceSettingsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CGetServiceSettingsGqlRequest,
			sdl::V1_0::agentino::CServiceSettingsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetServiceSettings(req, gqlReq, err);
			});
	}
	if (sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest,
			sdl::V1_0::agentino::CServiceSettingsPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateServiceSettings(req, gqlReq, err);
			});
	}
	if (commandId == QByteArrayLiteral("NotifyAgentServicesCollectionChanged")){
		return HandleAgentServiceCollectionNotify(gqlRequest, errorMessage);
	}

	return QJsonObject();
}


bool CServiceControllerProxyComp::IsRequestSupported(const imtgql::CGqlRequest& gqlRequest) const
{
	if (BaseClass::IsRequestSupported(gqlRequest)){
		return true;
	}

	if (gqlRequest.GetCommandId() == QByteArrayLiteral("NotifyAgentServicesCollectionChanged")){
		return true;
	}

	// The generic 'RemoveElements' command (imtbase collection schema) isn't part of
	// Services.sdl, so CServicesGqlHandlerCompBase never recognizes it - accept it here
	// too, but only for the 'Services' collection, so this proxy doesn't steal the
	// command from unrelated collections.
	if (gqlRequest.GetCommandId() != sdl::V1_0::imtbase::CRemoveElementsGqlRequest::GetCommandId()){
		return false;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	if (inputParamPtr == nullptr){
		return false;
	}

	return inputParamPtr->GetParamArgumentValue("collectionId").toByteArray() == QByteArrayLiteral("Services");
}


QJsonObject CServiceControllerProxyComp::HandleAgentServiceCollectionNotify(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QJsonObject response;
	response.insert(QStringLiteral("success"), false);

	// Agent identity: WebSocket clientid header (agent is the caller).
	QByteArray agentId = gqlRequest.GetHeader(QByteArrayLiteral("clientid"));
	if (agentId.isEmpty()){
		const imtgql::IGqlContext* contextPtr = gqlRequest.GetRequestContext();
		if (contextPtr != nullptr){
			agentId = contextPtr->GetHeaders().value(QByteArrayLiteral("clientid"));
		}
	}

	if (agentId.isEmpty()){
		errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing clientid (agent id)");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return response;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject(QByteArrayLiteral("input"));
	if (inputParamPtr == nullptr){
		errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing input");
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return response;
	}

	const QString typeOperation = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("typeOperation")).toString();
	const QByteArray itemId = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("itemId")).toByteArray();
	const QString itemIdsJoined = inputParamPtr->GetParamArgumentValue(QByteArrayLiteral("itemIds")).toString();

	qDebug() << "CServiceControllerProxyComp::HandleAgentServiceCollectionNotify"
			 << agentId << typeOperation << itemId << itemIdsJoined;

	QString syncError;
	bool ok = false;

	if (typeOperation == QStringLiteral("removed")){
		QByteArrayList serviceIds;
		const QStringList parts = itemIdsJoined.split(QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const QString& part: parts){
			serviceIds << part.toUtf8();
		}
		if (serviceIds.isEmpty() && !itemId.isEmpty()){
			serviceIds << itemId;
		}

		if (serviceIds.isEmpty()){
			// Empty ids (historical CChangeNotifier copy bug) — fall back to full agent reconcile
			// so topology does not keep ghost services with status UNDEFINED.
			SendErrorMessage(
						0,
						QStringLiteral("NotifyAgentServicesCollectionChanged: removed without item ids — reconciling agent"),
						"CServiceControllerProxyComp");
			ok = const_cast<CServiceControllerProxyComp*>(this)->SyncAgentServicesInMirror(agentId, syncError);
		}
		else{
			// const_cast: Sync methods are non-const on the synchronizer interface used by live path.
			ok = const_cast<CServiceControllerProxyComp*>(this)->RemoveServicesInMirror(agentId, serviceIds, syncError);
		}
	}
	else{
		// inserted / updated / default
		if (itemId.isEmpty()){
			errorMessage = QStringLiteral("NotifyAgentServicesCollectionChanged: missing itemId");
			SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

			return response;
		}

		ok = const_cast<CServiceControllerProxyComp*>(this)->SyncServiceInMirror(agentId, itemId, syncError);
	}

	if (!ok){
		errorMessage = syncError;
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		// Empty object → GQL transport layer treats as handler error with errorMessage.
		return QJsonObject();
	}

	errorMessage.clear();
	response.insert(QStringLiteral("success"), true);

	return response;
}


// private methods

template<class SdlGqlRequest, class SdlResponse>
QJsonObject CServiceControllerProxyComp::CreateResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage,
			std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	SdlGqlRequest serviceGqlRequest(gqlRequest, true);

	Q_ASSERT(serviceGqlRequest.IsValid());
	if (!serviceGqlRequest.IsValid()){
		return QJsonObject();
	}

	SdlResponse retVal = func(serviceGqlRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return QJsonObject();
	}

	QJsonObject resultObj;
	if (!retVal.WriteToJsonObject(resultObj)){
		errorMessage = QString("Unable to create response for command '%1'. Error: Writing to JSON object failed").arg(qPrintable(commandId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return QJsonObject();
	}

	return resultObj;
}


bool CServiceControllerProxyComp::SetServiceStatus(const QByteArray& serviceId, agentinodata::IServiceStatusInfo::ServiceStatus status) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceStatusCollection' was not set", "CServiceStatusControllerProxyComp");
		return false;
	}

	QByteArrayList serviceIds = m_serviceStatusCollectionCompPtr->GetElementIds();
	if (serviceIds.contains(serviceId)){
		imtbase::IObjectCollection::DataPtr dataPtr;
		if (m_serviceStatusCollectionCompPtr->GetObjectData(serviceId, dataPtr)){
			agentinodata::CServiceStatusInfo* serviceStatusInfoPtr = dynamic_cast<agentinodata::CServiceStatusInfo*>(dataPtr.GetPtr());
			serviceStatusInfoPtr->SetServiceStatus(status);

			if (!m_serviceStatusCollectionCompPtr->SetObjectData(serviceId, *serviceStatusInfoPtr)){
				return false;
			}
		}
	}
	else{
		istd::TDelPtr<agentinodata::CServiceStatusInfo> serviceStatusInfoPtr;
		serviceStatusInfoPtr.SetPtr(new agentinodata::CServiceStatusInfo);

		serviceStatusInfoPtr->SetServiceStatus(status);
		serviceStatusInfoPtr->SetServiceId(serviceId);

		QByteArray retVal = m_serviceStatusCollectionCompPtr->InsertNewObject("ServiceStatusInfo", "", "", serviceStatusInfoPtr.PopPtr(), serviceId);
		if (retVal.isEmpty()){
			return false;
		}
	}

	return true;
}


bool CServiceControllerProxyComp::SetServiceStatus(const QByteArray& serviceId, sdl::V1_0::agentino::ServiceStatus status) const
{
	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	if (status == sdl::V1_0::agentino::ServiceStatus::STARTING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STARTING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::NOT_RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_RUNNING;
	}
	else if (status == sdl::V1_0::agentino::ServiceStatus::STOPPING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STOPPING;
	}

	return SetServiceStatus(serviceId, serviceStatus);
}


istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CDependantConnectionInfo>> CServiceControllerProxyComp::GetConnectionsModel(
			const QByteArray& connectionUsageId) const
{
	istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CDependantConnectionInfo>> retVal;
	retVal.Emplace();

	if (!m_agentCollectionCompPtr.IsValid()){
		return retVal;
	}

	// Server: available connections from every agent's mirrored services.
	QList<sdl::V1_0::agentino::CDependantConnectionInfo> available;
	const imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (!m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			continue;
		}

		agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
		if (agentInfoPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		agentinodata::AppendAvailableConnectionsFromServiceCollection(
					*serviceCollectionPtr,
					connectionUsageId,
					available);
	}

	retVal->FromList(available);
	return retVal;
}


bool CServiceControllerProxyComp::GetConnectionObjectData(
			const imtbase::IObjectCollection::Id& connectionId,
			imtbase::IObjectCollection::DataPtr& connectionDataPtr) const
{
	if (connectionId.isEmpty()){
		return false;
	}

	if (m_agentCollectionCompPtr.IsValid()){
		imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
			imtbase::IObjectCollection::DataPtr agentDataPtr;
			if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
				agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
				if (agentInfoPtr != nullptr){
					// Get Services
					imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
					if (serviceCollectionPtr != nullptr){
						imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
						for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
							imtbase::IObjectCollection::DataPtr serviceDataPtr;
							if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
								agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
								if (serviceInfoPtr != nullptr){
									// Get Connections
									imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
									if (connectionCollectionPtr != nullptr){
										imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
										for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
											if (connectionElementId == connectionId){
												return connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}


void CServiceControllerProxyComp::UpdateUrlFromDependantConnection(sdl::V1_0::agentino::CServiceData& serviceData) const
{
	if (serviceData.outputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::COutputConnection>> connections = *serviceData.outputConnections;

		for (int i = 0; i < connections->size(); i++){
			istd::TNullableValue<sdl::V1_0::agentino::COutputConnection> connection = (*connections)[i];

			QByteArray dependantConnectionId;
			if (connection->dependantConnectionId){
				dependantConnectionId = (*connection->dependantConnectionId).toUtf8();
			}

			istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr = GetDependantServerConnectionParam(dependantConnectionId);

			sdl::V1_0::imtbase::CServerConnectionParam serverConnectionRepresentation;
			if (agentinodata::GetRepresentationFromServerConnectionParam(*serverConnectionInterfacePtr.GetPtr(), serverConnectionRepresentation)){
				connection->connectionParam = serverConnectionRepresentation;
				(*connections)[i] = connection;
			}
		}

		serviceData.outputConnections = connections;
	}
}


istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> CServiceControllerProxyComp::GetDependantServerConnectionParam(const QByteArray& dependantId) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'AgentCollection' was not set", "CServiceControllerProxyComp");
		return nullptr;
	}

	imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId: agentIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}

					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						imtbase::IObjectCollection::DataPtr connectionParamDataPtr;
						if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionParamDataPtr)){
							imtservice::CUrlConnectionParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionParamDataPtr.GetPtr());
							if (connectionParamPtr == nullptr){
								break;
							}

							if (connectionElementId == dependantId){
								istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr;
								serverConnectionInterfacePtr.MoveCastedPtr(connectionParamPtr->CloneMe());

								return serverConnectionInterfacePtr;
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}


sdl::V1_0::imtbase::CServerConnectionParam CServiceControllerProxyComp::GetServerConnectionParam(
			const sdl::V1_0::agentino::CServiceData& serviceData,
			const QByteArray& connectionId) const
{
	if (serviceData.inputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections = *serviceData.inputConnections;

		for (const istd::TNullableValue<sdl::V1_0::agentino::CInputConnection>& connection : *connections){
			if (connectionId == *connection->id){
				return *connection->connectionParam;
			}
		}
	}

	return sdl::V1_0::imtbase::CServerConnectionParam();
}


QByteArrayList CServiceControllerProxyComp::GetChangedConnectionUrl(
			const sdl::V1_0::agentino::CServiceData& serviceData1,
			const sdl::V1_0::agentino::CServiceData& serviceData2) const
{
	QByteArrayList retVal;

	if (serviceData1.inputConnections && serviceData2.inputConnections){
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections1 = *serviceData1.inputConnections;
		istd::TNullableValue<imtsdl::TElementList<sdl::V1_0::agentino::CInputConnection>> connections2 = *serviceData2.inputConnections;

		for (int i = 0; i < connections1->size(); i++){
			istd::TNullableValue<sdl::V1_0::agentino::CInputConnection> connection1 = (*connections1)[i];
			istd::TNullableValue<sdl::V1_0::agentino::CInputConnection> connection2 = (*connections2)[i];

			sdl::V1_0::imtbase::CServerConnectionParam connectionParam1 = *connection1->connectionParam;
			sdl::V1_0::imtbase::CServerConnectionParam connectionParam2 = *connection2->connectionParam;

			if (*connectionParam1.host != *connectionParam2.host ||
				*connectionParam1.httpPort != *connectionParam2.httpPort ||
				*connectionParam1.wsPort != *connectionParam2.wsPort){
				retVal << *connection1->id;
			}
		}
	}

	return retVal;
}


QByteArrayList CServiceControllerProxyComp::GetServiceIdsByDependantId(const QByteArray& dependantId) const
{
	QByteArrayList retVal;
	imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId: agentIds){
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
			agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
			if (agentInfoPtr == nullptr){
				continue;
			}

			imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
			if (serviceCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
				imtbase::IObjectCollection::DataPtr serviceDataPtr;
				if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
					agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
					if (serviceInfoPtr == nullptr){
						continue;
					}

					imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetDependantServiceConnections();
					if (connectionCollectionPtr == nullptr){
						continue;
					}

					imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
					for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
						imtbase::IObjectCollection::DataPtr connectionParamDataPtr;
						if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionParamDataPtr)){
							imtservice::CUrlConnectionLinkParam* connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionParamDataPtr.GetPtr());
							if (connectionParamPtr == nullptr){
								break;
							}

							if (dependantId == connectionParamPtr->GetDependantServiceConnectionId()){
								retVal << serviceElementId;
							}
						}
					}
				}
			}
		}
	}

	return retVal;
}


bool CServiceControllerProxyComp::UpdateConnectionForService(
			const QByteArray& serviceId,
			const QByteArray& agentId,
			const QByteArray& connectionId,
			const sdl::V1_0::imtbase::CServerConnectionParam& connectionParam) const
{
	sdl::V1_0::agentino::UpdateConnectionUrlRequestArguments arguments;
	arguments.input = sdl::V1_0::agentino::CConnectionUrlInput();
	arguments.input->serviceId = serviceId;
	arguments.input->connectionId = connectionId;
	arguments.input->connectionParam = connectionParam;

	imtgql::CGqlRequest gqlRequest;
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		return false;
	}

	QString errorMessage;
	sdl::V1_0::agentino::CUpdateConnectionUrlResponse retVal = SendModelRequest<sdl::V1_0::agentino::CUpdateConnectionUrlResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return false;
	}

	if (retVal.succesful.has_value()){
		return *retVal.succesful;
	}

	return false;
}


QByteArrayList CServiceControllerProxyComp::GetMirrorServiceIds(const QByteArray& agentId) const
{
	QByteArrayList retVal;

	if (!m_agentCollectionCompPtr.IsValid() || agentId.isEmpty()){
		return retVal;
	}

	imtbase::IObjectCollection::DataPtr agentDataPtr;
	if (!m_agentCollectionCompPtr->GetObjectData(agentId, agentDataPtr)){
		return retVal;
	}

	agentinodata::CAgentInfo* agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
	if (agentInfoPtr == nullptr){
		return retVal;
	}

	imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
	if (serviceCollectionPtr == nullptr){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids ids = serviceCollectionPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& id : ids){
		retVal << id;
	}

	return retVal;
}


void CServiceControllerProxyComp::RemoveServiceStatuses(const QByteArrayList& serviceIds) const
{
	if (!m_serviceStatusCollectionCompPtr.IsValid() || serviceIds.isEmpty()){
		return;
	}

	const QByteArrayList existingStatusIds = m_serviceStatusCollectionCompPtr->GetElementIds();
	imtbase::ICollectionInfo::Ids toRemove;
	for (const QByteArray& serviceId : serviceIds){
		if (existingStatusIds.contains(serviceId)){
			toRemove << serviceId;
		}
	}

	if (!toRemove.isEmpty()){
		m_serviceStatusCollectionCompPtr->RemoveElements(toRemove);
	}
}


void CServiceControllerProxyComp::RemoveStaleMirrorServicesByPath(
			const QByteArray& agentId,
			const QByteArray& keepServiceId,
			const QByteArray& servicePath) const
{
	if (!m_serviceManagerCompPtr.IsValid() || servicePath.isEmpty() || agentId.isEmpty()){
		return;
	}

	const QByteArrayList mirrorIds = GetMirrorServiceIds(agentId);
	QByteArrayList staleIds;
	for (const QByteArray& mirrorId : mirrorIds){
		if (mirrorId == keepServiceId){
			continue;
		}

		const agentinodata::IServiceInfo* otherPtr = m_serviceManagerCompPtr->GetService(agentId, mirrorId);
		if (otherPtr == nullptr){
			continue;
		}

		const QByteArray otherPath = otherPtr->GetServicePath();
		if (otherPath.isEmpty()){
			continue;
		}

		if (QString::fromUtf8(otherPath).compare(QString::fromUtf8(servicePath), Qt::CaseInsensitive) == 0){
			staleIds << mirrorId;
		}
	}

	if (staleIds.isEmpty()){
		return;
	}

	qDebug() << "CServiceControllerProxyComp::RemoveStaleMirrorServicesByPath"
			 << "agent=" << agentId
			 << "keep=" << keepServiceId
			 << "path=" << servicePath
			 << "stale=" << staleIds;

	QString removeError;
	if (!const_cast<CServiceControllerProxyComp*>(this)->RemoveServicesInMirror(agentId, staleIds, removeError)){
		SendErrorMessage(
					0,
					QString("Unable to drop stale mirror services for path '%1': %2")
								.arg(QString::fromUtf8(servicePath), removeError),
					"CServiceControllerProxyComp");
	}
}


void CServiceControllerProxyComp::AppendServicesListFields(imtgql::CGqlRequest& gqlRequest)
{
	// Matches agent ListObjects/GetInformationIds("items") expectations.
	imtgql::CGqlFieldObject itemsFields;
	itemsFields.InsertField("id");
	itemsFields.InsertField("typeId");
	itemsFields.InsertField("name");
	itemsFields.InsertField("description");
	itemsFields.InsertField("path");
	itemsFields.InsertField("status");
	itemsFields.InsertField("version");
	gqlRequest.AddField("items", itemsFields);
}


void CServiceControllerProxyComp::AppendServiceDataFields(imtgql::CGqlRequest& gqlRequest)
{
	// Minimal set required by GetServiceFromRepresentation + mirror storage.
	gqlRequest.AddSimpleField("id");
	gqlRequest.AddSimpleField("name");
	gqlRequest.AddSimpleField("description");
	gqlRequest.AddSimpleField("path");
	gqlRequest.AddSimpleField("arguments");
	gqlRequest.AddSimpleField("serviceTypeId");
	gqlRequest.AddSimpleField("enableVerbose");
	gqlRequest.AddSimpleField("isAutoStart");
	gqlRequest.AddSimpleField("tracingLevel");
	gqlRequest.AddSimpleField("startScript");
	gqlRequest.AddSimpleField("stopScript");
	gqlRequest.AddSimpleField("settingsPath");
	gqlRequest.AddSimpleField("version");
	gqlRequest.AddSimpleField("status");

	imtgql::CGqlFieldObject connectionParamFields;
	connectionParamFields.InsertField("host");
	connectionParamFields.InsertField("httpPort");
	connectionParamFields.InsertField("wsPort");

	imtgql::CGqlFieldObject inputConnectionFields;
	inputConnectionFields.InsertField("id");
	inputConnectionFields.InsertField("connectionName");
	inputConnectionFields.InsertField("description");
	inputConnectionFields.InsertField("serviceTypeId");
	inputConnectionFields.InsertField("connectionParam", connectionParamFields);

	imtgql::CGqlFieldObject outputConnectionFields;
	outputConnectionFields.InsertField("id");
	outputConnectionFields.InsertField("connectionName");
	outputConnectionFields.InsertField("description");
	outputConnectionFields.InsertField("serviceName");
	outputConnectionFields.InsertField("serviceTypeId");
	outputConnectionFields.InsertField("dependantConnectionId");
	outputConnectionFields.InsertField("connectionParam", connectionParamFields);

	gqlRequest.AddField("inputConnections", inputConnectionFields);
	gqlRequest.AddField("outputConnections", outputConnectionFields);
}


} // namespace agentinogql

