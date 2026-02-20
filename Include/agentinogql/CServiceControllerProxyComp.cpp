// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceControllerProxyComp.h>


// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CAgentInfo.h>
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/CServiceStatusInfo.h>

// ImtCore includes
#include <imtgql/CGqlContext.h>


namespace agentinogql
{


// protected methods

sdl::agentino::Services::CServiceData CServiceControllerProxyComp::OnGetService(
			const sdl::agentino::Services::CGetServiceGqlRequest& getServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}

	sdl::agentino::Services::GetServiceRequestArguments arguments = getServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CServiceData();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	if (inputParamPtr == nullptr){
		return sdl::agentino::Services::CServiceData();
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}

	sdl::agentino::Services::CServiceData response = SendModelRequest<sdl::agentino::Services::CServiceData>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return response;
	}

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (!serviceInfoPtr.IsValid()){
		errorMessage = QString("Unable to get service '%1'. Error: Service not exists").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}

	// Syncronise service with agent
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, *response.Version_1_0, errorMessage)){
		errorMessage = QString("Unable to get service '%1'. Error: %2").arg(qPrintable(serviceId), errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}

	if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr, "", "", true)){
		errorMessage = QString("Unable to set service '%1'. Error: Set service to collection failed").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}

	if (response.Version_1_0->outputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::COutputConnection::V1_0>> outputConnections = *response.Version_1_0->outputConnections;
		for (istd::TSharedNullable<sdl::agentino::Services::COutputConnection::V1_0>& outputConnection : *outputConnections){
			QByteArray id = *outputConnection->id;
			outputConnection->dependantConnectionList = GetConnectionsModel(id);

			istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr = GetDependantServerConnectionParam(id);
			if (serverConnectionInterfacePtr.IsValid()){
				sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 urlRepresentation;
				if (agentinodata::GetRepresentationFromServerConnectionParam(*serverConnectionInterfacePtr.GetPtr(), urlRepresentation)){
					outputConnection->connectionParam = urlRepresentation;
				}
			}
		}

		response.Version_1_0->outputConnections = outputConnections;
	}

	return response;
}


sdl::imtbase::ImtCollection::CUpdatedNotificationPayload CServiceControllerProxyComp::OnUpdateService(
			const sdl::agentino::Services::CUpdateServiceGqlRequest& updateServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	sdl::agentino::Services::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	if (arguments.input.Version_1_0->item){
		serviceData = *arguments.input.Version_1_0->item;
	}

	sdl::imtbase::ImtCollection::CUpdatedNotificationPayload retVal =
			SendModelRequest<sdl::imtbase::ImtCollection::CUpdatedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	istd::TDelPtr<agentinodata::CServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetCastedOrRemove(m_serviceManagerCompPtr->GetService(agentId, serviceId));
	if (!serviceInfoPtr.IsValid()){
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	// Обновление всех сервисов зависимых от портов текущего
	sdl::agentino::Services::CServiceData::V1_0 currentServiceData;
	if (agentinodata::GetRepresentationFromService(currentServiceData, *serviceInfoPtr.GetPtr())){
		QByteArrayList connectionIds = GetChangedConnectionUrl(currentServiceData, serviceData);

		for (const QByteArray& connectionId : connectionIds){
			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 newConnectionParam = GetServerConnectionParam(serviceData, connectionId);
			QByteArrayList dependentServiceIds = GetServiceIdsByDependantId(connectionId);
			for (const QByteArray& dependantServiceId : dependentServiceIds){
				UpdateConnectionForService(dependantServiceId, agentId, connectionId, newConnectionParam);
			}
		}
	}

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr.GetPtr(), serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation");
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	QString serviceName;
	QString serviceDescription;

	if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr.PopPtr(), serviceName, serviceDescription)){
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}

	return retVal;
}


sdl::imtbase::ImtCollection::CAddedNotificationPayload CServiceControllerProxyComp::OnAddService(
			const sdl::agentino::Services::CAddServiceGqlRequest& addServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}

	sdl::agentino::Services::AddServiceRequestArguments arguments = addServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::imtbase::ImtCollection::CAddedNotificationPayload retVal = SendModelRequest<sdl::imtbase::ImtCollection::CAddedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	if (arguments.input.Version_1_0->item){
		serviceData = *arguments.input.Version_1_0->item;
	}

	// We take the document from the agent immediately after adding it to download the connection information.
	sdl::agentino::Services::GetServiceRequestArguments getArguments;
	getArguments.input.Version_1_0.emplace();
	getArguments.input.Version_1_0->id = serviceId;

	imtgql::CGqlRequest getGqlRequest;

	imtgql::IGqlContextSharedPtr contextPtr;
	contextPtr.MoveCastedPtr(gqlRequest.GetRequestContext()->CloneMe());
	getGqlRequest.SetGqlContext(contextPtr);

	if (sdl::agentino::Services::CGetServiceGqlRequest::SetupGqlRequest(getGqlRequest, getArguments)){
		sdl::agentino::Services::CServiceData getServiceResponse = SendModelRequest<sdl::agentino::Services::CServiceData>(getGqlRequest, errorMessage);
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

			return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
		}
	}

	istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation");
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}

	serviceInfoPtr->SetObjectUuid(serviceId);

	QString serviceName = serviceInfoPtr->GetServiceName();
	QString serviceDescription = serviceInfoPtr->GetServiceDescription();

	if (!m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr.PopPtr(), serviceId, serviceName, serviceDescription)){
		errorMessage = QString("Unable to add service '%1'").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}

	return retVal;
}


// reimplemented (sdl::agentino::Services::CGraphQlHandlerCompBase)

sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnStartService(
			const sdl::agentino::Services::CStartServiceGqlRequest& startServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Services::StartServiceRequestArguments arguments = startServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CServiceStatusResponse();
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->serviceId){
		serviceId = *arguments.input.Version_1_0->serviceId;
	}

	istd::CChangeGroup changeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_STARTING);

	sdl::agentino::Services::CServiceStatusResponse retVal = SendModelRequest<sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);

		return sdl::agentino::Services::CServiceStatusResponse();
	}

	if (retVal.Version_1_0->status.has_value()){
		SetServiceStatus(serviceId, *retVal.Version_1_0->status);
	}

	return retVal;
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnStopService(
			const sdl::agentino::Services::CStopServiceGqlRequest& stopServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Services::StopServiceRequestArguments arguments = stopServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CServiceStatusResponse();
	}

	QByteArray serviceId;
	if (arguments.input.Version_1_0->serviceId){
		serviceId = *arguments.input.Version_1_0->serviceId;
	}

	istd::CChangeGroup changeGroup(m_serviceStatusCollectionCompPtr.GetPtr());

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_STOPPING);

	sdl::agentino::Services::CServiceStatusResponse retVal = SendModelRequest<sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
		return sdl::agentino::Services::CServiceStatusResponse();
	}

	if (retVal.Version_1_0->status.has_value()){
		SetServiceStatus(serviceId, *retVal.Version_1_0->status);
	}

	return retVal;
}


sdl::imtbase::ImtCollection::CRemovedNotificationPayload CServiceControllerProxyComp::OnServicesRemove(
			const sdl::agentino::Services::CServicesRemoveGqlRequest& removeServiceRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}

	sdl::agentino::Services::ServicesRemoveRequestArguments arguments = removeServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}

	QByteArrayList serviceIds;
	if (arguments.input.Version_1_0->elementIds){
		serviceIds = (*arguments.input.Version_1_0->elementIds).ToList();
	}

	const imtgql::CGqlParamObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::imtbase::ImtCollection::CRemovedNotificationPayload retVal = SendModelRequest<sdl::imtbase::ImtCollection::CRemovedNotificationPayload>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		SendErrorMessage(
			0,
			QString("Unable to remove service '%1' from agent '%2'").arg(qPrintable(serviceIds.join(';')), qPrintable(agentId)),
			"CServiceControllerProxyComp");
	}

	return retVal;
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnGetServiceStatus(
			const sdl::agentino::Services::CGetServiceStatusGqlRequest& getServiceStatusRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Services::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CServiceStatusResponse();
	}

	sdl::agentino::Services::CServiceStatusResponse retVal = SendModelRequest<sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceStatusResponse();
	}

	return retVal;
}


sdl::agentino::Services::CUpdateConnectionUrlResponse CServiceControllerProxyComp::OnUpdateConnectionUrl(
			const sdl::agentino::Services::CUpdateConnectionUrlGqlRequest& /*updateConnectionUrlRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	return sdl::agentino::Services::CUpdateConnectionUrlResponse();
}


sdl::agentino::Services::CPluginInfo CServiceControllerProxyComp::OnLoadPlugin(
			const sdl::agentino::Services::CLoadPluginGqlRequest& loadPluginRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::agentino::Services::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CPluginInfo();
	}

	sdl::agentino::Services::CPluginInfo retVal = SendModelRequest<sdl::agentino::Services::CPluginInfo>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CPluginInfo();
	}

	// Update dependant connections elements
	if (retVal.Version_1_0->outputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::COutputConnection::V1_0>> outputConnections = *retVal.Version_1_0->outputConnections;
		for (istd::TSharedNullable<sdl::agentino::Services::COutputConnection::V1_0>& outputConnection : *outputConnections){
			QByteArray id = *outputConnection->id;
			outputConnection->dependantConnectionList = GetConnectionsModel(id);
		}

		retVal.Version_1_0->outputConnections = outputConnections;
	}

	return retVal;
}


// reimplemented (sdl::agentino::Services::CGraphQlHandlerCompBase)

imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateInternalResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	if (sdl::agentino::Services::CGetServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CGetServiceGqlRequest,
			sdl::agentino::Services::CServiceData>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnGetService(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CUpdateServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CUpdateServiceGqlRequest,
			sdl::imtbase::ImtCollection::CUpdatedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateService(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CAddServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CAddServiceGqlRequest,
			sdl::imtbase::ImtCollection::CAddedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnAddService(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CStartServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CStartServiceGqlRequest,
			sdl::agentino::Services::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStartService(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CStopServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CStopServiceGqlRequest,
			sdl::agentino::Services::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStopService(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CServicesRemoveGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CServicesRemoveGqlRequest,
			sdl::imtbase::ImtCollection::CRemovedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnServicesRemove(req, gqlReq, err);
			});
	}
	if (sdl::agentino::Services::CLoadPluginGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CLoadPluginGqlRequest,
			sdl::agentino::Services::CPluginInfo>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnLoadPlugin(req, gqlReq, err);
			});
	}

	return nullptr;
}


// private methods

template<class SdlGqlRequest, class SdlResponse>
imtbase::CTreeItemModel* CServiceControllerProxyComp::CreateResponse(
			const imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage,
			std::function<SdlResponse(const SdlGqlRequest&, const imtgql::CGqlRequest&, QString&)> func) const
{
	QByteArray commandId = gqlRequest.GetCommandId();

	SdlGqlRequest serviceGqlRequest(gqlRequest, true);

	Q_ASSERT(serviceGqlRequest.IsValid());
	if (!serviceGqlRequest.IsValid()){
		return nullptr;
	}

	SdlResponse retVal = func(serviceGqlRequest, gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return nullptr;
	}

	istd::TDelPtr<imtbase::CTreeItemModel> resultModelPtr(new imtbase::CTreeItemModel);
	if (!retVal.WriteToModel(*resultModelPtr.GetPtr())){
		errorMessage = QString("Unable to create response for command '%1'. Error: Writing to tree model failed").arg(qPrintable(commandId));
		SendErrorMessage(0, errorMessage, "CServiceControllerProxyComp");

		return nullptr;
	}

	return resultModelPtr.PopPtr();
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


bool CServiceControllerProxyComp::SetServiceStatus(const QByteArray& serviceId, sdl::agentino::Services::ServiceStatus status) const
{
	agentinodata::IServiceStatusInfo::ServiceStatus serviceStatus = agentinodata::IServiceStatusInfo::SS_UNDEFINED;
	if (status == sdl::agentino::Services::ServiceStatus::STARTING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STARTING;
	}
	else if (status == sdl::agentino::Services::ServiceStatus::NOT_RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_NOT_RUNNING;
	}
	else if (status == sdl::agentino::Services::ServiceStatus::RUNNING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_RUNNING;
	}
	else if (status == sdl::agentino::Services::ServiceStatus::STOPPING){
		serviceStatus = agentinodata::IServiceStatusInfo::SS_STOPPING;
	}

	return SetServiceStatus(serviceId, serviceStatus);
}


istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CDependantConnectionInfo::V1_0>> CServiceControllerProxyComp::GetConnectionsModel(
			const QByteArray& connectionUsageId) const
{
	istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CDependantConnectionInfo::V1_0>> retVal;
	retVal.Emplace();

	if (!m_agentCollectionCompPtr.IsValid()){
		return retVal;
	}

	imtbase::ICollectionInfo::Ids elementIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& elementId: elementIds){
		agentinodata::CAgentInfo* agentInfoPtr = nullptr;
		imtbase::IObjectCollection::DataPtr agentDataPtr;
		if (m_agentCollectionCompPtr->GetObjectData(elementId, agentDataPtr)){
			agentInfoPtr = dynamic_cast<agentinodata::CAgentInfo*>(agentDataPtr.GetPtr());
		}

		if (agentInfoPtr == nullptr){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = agentInfoPtr->GetServiceCollection();
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		imtbase::ICollectionInfo::Ids serviceElementIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceElementId: serviceElementIds){
			agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (serviceCollectionPtr->GetObjectData(serviceElementId, serviceDataPtr)){
				serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			}

			if (serviceInfoPtr == nullptr){
				continue;
			}

			// Get Connections
			imtbase::IObjectCollection* connectionCollectionPtr = serviceInfoPtr->GetInputConnections();
			if (connectionCollectionPtr == nullptr){
				continue;
			}

			imtbase::ICollectionInfo::Ids connectionElementIds = connectionCollectionPtr->GetElementIds();
			for (const imtbase::ICollectionInfo::Id& connectionElementId: connectionElementIds){
				imtservice::CUrlConnectionParam* connectionParamPtr = nullptr;
				imtbase::IObjectCollection::DataPtr connectionDataPtr;
				if (connectionCollectionPtr->GetObjectData(connectionElementId, connectionDataPtr)){
					connectionParamPtr = dynamic_cast<imtservice::CUrlConnectionParam*>(connectionDataPtr.GetPtr());
				}

				if (connectionParamPtr == nullptr){
					continue;
				}

				imtservice::IServiceConnectionInfo::ConnectionType connectionType = connectionParamPtr->GetConnectionType();
				if (connectionType == imtservice::IServiceConnectionInfo::CT_INPUT && connectionElementId == connectionUsageId){
					sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 sdlRepresentation;
					if (agentinodata::GetRepresentationFromServerConnectionParam(*connectionParamPtr, sdlRepresentation)){
						sdl::agentino::Services::CDependantConnectionInfo::V1_0 dependantConnectionInfo;

						QString host = connectionParamPtr->GetHost();
						if (host.isEmpty()){
							host = "localhost";
						}

						int httpPort = connectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);

						dependantConnectionInfo.id = connectionElementId;
						dependantConnectionInfo.name = host + "/" + QString::number(httpPort);
						dependantConnectionInfo.connectionParam = sdlRepresentation;

						*retVal << dependantConnectionInfo;

						QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();

						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							sdl::agentino::Services::CDependantConnectionInfo::V1_0 incomingConnectionInfo;
							incomingConnectionInfo.id = incomingConnection.id;

							QString incomingHost = incomingConnection.host;
							int incommingHttpPort = incomingConnection.httpPort;
							incomingConnectionInfo.name = incomingHost + "/" + QString::number(incommingHttpPort);

							sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParam;
							connectionParam.host = incomingHost;
							connectionParam.wsPort = incomingConnection.wsPort;
							connectionParam.httpPort = incomingConnection.httpPort;

							incomingConnectionInfo.connectionParam = connectionParam;

							*retVal << incomingConnectionInfo;
						}
					}
				}
			}
		}
	}

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


void CServiceControllerProxyComp::UpdateUrlFromDependantConnection(sdl::agentino::Services::CServiceData::V1_0& serviceData) const
{
	if (serviceData.outputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::COutputConnection::V1_0>> connections = *serviceData.outputConnections;

		for (int i = 0; i < connections->size(); i++){
			istd::TSharedNullable<sdl::agentino::Services::COutputConnection::V1_0> connection = (*connections)[i];

			QByteArray dependantConnectionId;
			if (connection->dependantConnectionId){
				dependantConnectionId = (*connection->dependantConnectionId).toUtf8();
			}

			istd::TSharedInterfacePtr<imtcom::CServerConnectionInterfaceParam> serverConnectionInterfacePtr = GetDependantServerConnectionParam(dependantConnectionId);

			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 serverConnectionRepresentation;
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


sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 CServiceControllerProxyComp::GetServerConnectionParam(
			const sdl::agentino::Services::CServiceData::V1_0& serviceData,
			const QByteArray& connectionId) const
{
	if (serviceData.inputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CInputConnection::V1_0>> connections = *serviceData.inputConnections;

		for (const istd::TSharedNullable<sdl::agentino::Services::CInputConnection::V1_0>& connection : *connections){
			if (connectionId == *connection->id){
				return *connection->connectionParam;
			}
		}
	}

	return sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0();
}


QByteArrayList CServiceControllerProxyComp::GetChangedConnectionUrl(
			const sdl::agentino::Services::CServiceData::V1_0& serviceData1,
			const sdl::agentino::Services::CServiceData::V1_0& serviceData2) const
{
	QByteArrayList retVal;

	if (serviceData1.inputConnections && serviceData2.inputConnections){
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CInputConnection::V1_0>> connections1 = *serviceData1.inputConnections;
		istd::TSharedNullable<imtsdl::TElementList<sdl::agentino::Services::CInputConnection::V1_0>> connections2 = *serviceData2.inputConnections;

		for (int i = 0; i < connections1->size(); i++){
			istd::TSharedNullable<sdl::agentino::Services::CInputConnection::V1_0> connection1 = (*connections1)[i];
			istd::TSharedNullable<sdl::agentino::Services::CInputConnection::V1_0> connection2 = (*connections2)[i];

			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParam1 = *connection1->connectionParam;
			sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParam2 = *connection2->connectionParam;

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
			const sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0& connectionParam) const
{
	sdl::agentino::Services::UpdateConnectionUrlRequestArguments arguments;
	arguments.input.Version_1_0 = sdl::agentino::Services::CConnectionUrlInput::V1_0();
	arguments.input.Version_1_0->serviceId = serviceId;
	arguments.input.Version_1_0->connectionId = connectionId;
	arguments.input.Version_1_0->connectionParam = connectionParam;

	imtgql::CGqlRequest gqlRequest;
	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert("clientid", agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	if (!sdl::agentino::Services::CUpdateConnectionUrlGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		return false;
	}

	QString errorMessage;
	sdl::agentino::Services::CUpdateConnectionUrlResponse retVal = SendModelRequest<sdl::agentino::Services::CUpdateConnectionUrlResponse>(gqlRequest, errorMessage);
	if (!errorMessage.isEmpty()){
		return false;
	}

	if (retVal.Version_1_0->succesful.has_value()){
		return *retVal.Version_1_0->succesful;
	}

	return false;
}


} // namespace agentinogql


