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
	
	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	if (inputParamPtr == nullptr){
		return sdl::agentino::Services::CServiceData();
	}
	
	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}
	
	sdl::agentino::Services::CServiceData::V1_0 response;
	if (!SendModelRequest<
			sdl::agentino::Services::CServiceData::V1_0,
			sdl::agentino::Services::CServiceData>(gqlRequest, response, errorMessage)){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
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
	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, response, errorMessage)){
		errorMessage = QString("Unable to get service '%1'. Error: %2").arg(qPrintable(serviceId), errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}
	
	if (!m_serviceManagerCompPtr->SetService(agentId, serviceId, *serviceInfoPtr, "", "", true)){
		errorMessage = QString("Unable to set service '%1'. Error: Set service to collection failed").arg(qPrintable(serviceId));
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceData();
	}
	
	if (response.outputConnections){
		QList<sdl::agentino::Services::COutputConnection::V1_0> outputConnections = *response.outputConnections;
		for (sdl::agentino::Services::COutputConnection::V1_0& outputConnection : outputConnections){
			QByteArray id = *outputConnection.id;
			QByteArray usageId = (*outputConnection.usageId).toUtf8();
			outputConnection.elements = GetConnectionsModel(usageId);
			
			QUrl url = GetDependantConnectionUrl(id);
			if (url.isValid()){
				sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
				if (agentinodata::GetRepresentationFromUrlParam(url, urlRepresentation)){
					outputConnection.url = urlRepresentation;
				}
			}
		}
		
		response.outputConnections = outputConnections;
	}
	
	sdl::agentino::Services::CServiceData retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
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
	
	sdl::imtbase::ImtCollection::CUpdatedNotificationPayload::V1_0 response;
	
	sdl::agentino::Services::UpdateServiceRequestArguments arguments = updateServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CUpdatedNotificationPayload();
	}
	
	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}
	
	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);

	QByteArray agentId = gqlRequest.GetHeader("clientid");

	sdl::agentino::Services::CServiceData::V1_0 serviceData;
	if (arguments.input.Version_1_0->item){
		serviceData = *arguments.input.Version_1_0->item;
	}

	if (!SendModelRequest<
			sdl::imtbase::ImtCollection::CUpdatedNotificationPayload::V1_0,
			sdl::imtbase::ImtCollection::CUpdatedNotificationPayload>(gqlRequest, response, errorMessage)){
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
			sdl::agentino::Services::CUrlParameter::V1_0 newUrlParam = GetUrlParam(serviceData, connectionId);
			QByteArrayList dependentServiceIds = GetServiceIdsByDependantId(connectionId);
			for (const QByteArray& dependantServiceId : dependentServiceIds){
				UpdateConnectionUrlForService(dependantServiceId, agentId, connectionId, newUrlParam);
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
	
	sdl::imtbase::ImtCollection::CUpdatedNotificationPayload retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
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
	
	sdl::imtbase::ImtCollection::CAddedNotificationPayload::V1_0 response;
	
	sdl::agentino::Services::AddServiceRequestArguments arguments = addServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CAddedNotificationPayload();
	}
	
	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}
	
	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	
	QByteArray agentId = gqlRequest.GetHeader("clientid");
	
	if (!SendModelRequest<
			sdl::imtbase::ImtCollection::CAddedNotificationPayload::V1_0,
			sdl::imtbase::ImtCollection::CAddedNotificationPayload>(gqlRequest, response, errorMessage)){
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
	getGqlRequest.SetGqlContext(dynamic_cast<imtgql::IGqlContext*>(gqlRequest.GetRequestContext()->CloneMe()));
	if (sdl::agentino::Services::CGetServiceGqlRequest::SetupGqlRequest(getGqlRequest, getArguments)){
		sdl::agentino::Services::CServiceData::V1_0 getServiceResponse;
		if (!SendModelRequest<
				sdl::agentino::Services::CServiceData::V1_0,
				sdl::agentino::Services::CServiceData>(getGqlRequest, serviceData, errorMessage)){
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
	
	sdl::imtbase::ImtCollection::CAddedNotificationPayload retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
}


// reimplemented (sdl::agentino::Services::CGraphQlHandlerCompBase)


sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnStartService(
	const sdl::agentino::Services::CStartServiceGqlRequest& startServiceRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse::V1_0 response;
	
	response.status = sdl::agentino::Services::ServiceStatus::UNDEFINED;
	
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
	
	if (!SendModelRequest<
			sdl::agentino::Services::CServiceStatusResponse::V1_0,
			sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, response, errorMessage)){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);

		return sdl::agentino::Services::CServiceStatusResponse();
	}
	
	if (response.status.has_value()){
		SetServiceStatus(serviceId, *response.status);
	}
	
	sdl::agentino::Services::CServiceStatusResponse retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnStopService(
	const sdl::agentino::Services::CStopServiceGqlRequest& stopServiceRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse::V1_0 response;
	response.status = sdl::agentino::Services::ServiceStatus::UNDEFINED;
	
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
	
	if (!SendModelRequest<
			sdl::agentino::Services::CServiceStatusResponse::V1_0,
			sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, response, errorMessage)){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_UNDEFINED);
		return sdl::agentino::Services::CServiceStatusResponse();
	}
	
	if (response.status.has_value()){
		SetServiceStatus(serviceId, *response.status);
	}
	
	sdl::agentino::Services::CServiceStatusResponse retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
}


sdl::imtbase::ImtCollection::CRemovedNotificationPayload CServiceControllerProxyComp::OnServicesRemove(
	const sdl::agentino::Services::CServicesRemoveGqlRequest& removeServiceRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::imtbase::ImtCollection::CRemovedNotificationPayload::V1_0 response;
	
	if (!m_serviceManagerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceManager' was not set", "CServiceControllerProxyComp");
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}
	
	sdl::agentino::Services::ServicesRemoveRequestArguments arguments = removeServiceRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}
	
	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}
	
	const imtgql::CGqlObject* inputParamPtr = gqlRequest.GetParamObject("input");
	Q_ASSERT(inputParamPtr != nullptr);
	
	QByteArray agentId = gqlRequest.GetHeader("clientid");
	
	if (!SendModelRequest<
			sdl::imtbase::ImtCollection::CRemovedNotificationPayload::V1_0,
			sdl::imtbase::ImtCollection::CRemovedNotificationPayload>(gqlRequest, response, errorMessage)){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
	}
	
	if (!m_serviceManagerCompPtr->RemoveService(agentId, serviceId)){
		SendErrorMessage(
			0,
			QString("Unable to remove service '%1' from agent '%2'").arg(qPrintable(serviceId), qPrintable(agentId)),
			"CServiceControllerProxyComp");
	}
	
	sdl::imtbase::ImtCollection::CRemovedNotificationPayload retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerProxyComp::OnGetServiceStatus(
	const sdl::agentino::Services::CGetServiceStatusGqlRequest& getServiceStatusRequest,
	const ::imtgql::CGqlRequest& gqlRequest,
	QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse::V1_0 response;
	
	sdl::agentino::Services::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		return sdl::agentino::Services::CServiceStatusResponse();
	}
	
	QByteArray serviceId;
	if (arguments.input.Version_1_0->id){
		serviceId = *arguments.input.Version_1_0->id;
	}
	
	if (!SendModelRequest<
			sdl::agentino::Services::CServiceStatusResponse::V1_0,
			sdl::agentino::Services::CServiceStatusResponse>(gqlRequest, response, errorMessage)){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::agentino::Services::CServiceStatusResponse();
	}
	
	sdl::agentino::Services::CServiceStatusResponse retVal;
	retVal.Version_1_0 = std::make_optional(response);
	
	return retVal;
}


sdl::agentino::Services::CUpdateConnectionUrlResponse CServiceControllerProxyComp::OnUpdateConnectionUrl(
	const sdl::agentino::Services::CUpdateConnectionUrlGqlRequest& /*updateConnectionUrlRequest*/,
	const ::imtgql::CGqlRequest& /*gqlRequest*/,
	QString& /*errorMessage*/) const
{
	return sdl::agentino::Services::CUpdateConnectionUrlResponse();
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
	else if (sdl::agentino::Services::CUpdateServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CUpdateServiceGqlRequest,
			sdl::imtbase::ImtCollection::CUpdatedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnUpdateService(req, gqlReq, err);
			});
	}
	else if (sdl::agentino::Services::CAddServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CAddServiceGqlRequest,
			sdl::imtbase::ImtCollection::CAddedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnAddService(req, gqlReq, err);
			});
	}
	else if (sdl::agentino::Services::CStartServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CStartServiceGqlRequest,
			sdl::agentino::Services::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStartService(req, gqlReq, err);
			});
	}
	else if (sdl::agentino::Services::CStopServiceGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CStopServiceGqlRequest,
			sdl::agentino::Services::CServiceStatusResponse>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnStopService(req, gqlReq, err);
			});
	}
	else if (sdl::agentino::Services::CServicesRemoveGqlRequest::GetCommandId() == commandId){
		return CreateResponse<
			sdl::agentino::Services::CServicesRemoveGqlRequest,
			sdl::imtbase::ImtCollection::CRemovedNotificationPayload>(
			gqlRequest,
			errorMessage,
			[&](const auto& req, const auto& gqlReq, QString& err){
				return OnServicesRemove(req, gqlReq, err);
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


QList<sdl::agentino::Services::CElement::V1_0> CServiceControllerProxyComp::GetConnectionsModel(
	const QByteArray& connectionUsageId) const
{
	QList<sdl::agentino::Services::CElement::V1_0> retVal;
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
				
				if (connectionParamPtr->GetUsageId() == connectionUsageId && connectionParamPtr->GetConnectionType() == imtservice::IServiceConnectionInfo::CT_INPUT){
					sdl::agentino::Services::CElement::V1_0 element;
						QUrl url = connectionParamPtr->GetUrl();
						url.setHost("localhost");
						
						QString urlStr = url.toString();
						
						element.id = connectionElementId;
						element.name = urlStr;
						
						sdl::agentino::Services::CUrlParameter::V1_0 urlParam;
						urlParam.host = url.host();
						urlParam.port = url.port();
						urlParam.scheme = url.scheme();
						element.url = urlParam;
						
						retVal << element;
						
						QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
						
						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							sdl::agentino::Services::CElement::V1_0 incomingElement;
							
							incomingElement.id = incomingConnection.id;
							incomingElement.name = incomingConnection.url.toString();
							
							sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
							
							if (agentinodata::GetRepresentationFromUrlParam(incomingConnection.url, urlRepresentation)){
								incomingElement.url = urlRepresentation;
								
								retVal << incomingElement;
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
		QList<sdl::agentino::Services::COutputConnection::V1_0> connections = *serviceData.outputConnections;
		
		for (int i = 0; i < connections.size(); i++){
			sdl::agentino::Services::COutputConnection::V1_0 connection = connections[i];
			
			QByteArray dependantConnectionId;
			if (connection.dependantConnectionId){
				dependantConnectionId = (*connection.dependantConnectionId).toUtf8();
			}
			
			QUrl url = GetDependantConnectionUrl(dependantConnectionId);
			
			sdl::agentino::Services::CUrlParameter::V1_0 urlRepresentation;
			urlRepresentation.host = url.host();
			urlRepresentation.port = url.port();
			urlRepresentation.scheme = url.scheme();
			
			connection.url = urlRepresentation;
			
			connections[i] = connection;
		}
		
		serviceData.outputConnections = connections;
	}
}


QUrl CServiceControllerProxyComp::GetDependantConnectionUrl(const QByteArray& dependantId) const
{
	if (!m_agentCollectionCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'AgentCollection' was not set", "CServiceControllerProxyComp");
		return QUrl();
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
								return connectionParamPtr->GetUrl();
							}
							
							QList<imtservice::IServiceConnectionParam::IncomingConnectionParam> incomingConnections = connectionParamPtr->GetIncomingConnections();
							for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
								if (incomingConnection.id == dependantId){
									return incomingConnection.url;
								}
							}
						}
					}
				}
			}
		}
	}
	
	return QUrl();
}


sdl::agentino::Services::CUrlParameter::V1_0 CServiceControllerProxyComp::GetUrlParam(
	const sdl::agentino::Services::CServiceData::V1_0& serviceData,
	const QByteArray& connectionId) const
{
	if (serviceData.inputConnections){
		QList<sdl::agentino::Services::CInputConnection::V1_0> connections = *serviceData.inputConnections;
		
		for (const sdl::agentino::Services::CInputConnection::V1_0& connection : connections){
			if (connectionId == *connection.id){
				return *connection.url;
			}
		}
	}
	
	return sdl::agentino::Services::CUrlParameter::V1_0();
}


QByteArrayList CServiceControllerProxyComp::GetChangedConnectionUrl(
	const sdl::agentino::Services::CServiceData::V1_0& serviceData1,
	const sdl::agentino::Services::CServiceData::V1_0& serviceData2) const
{
	QByteArrayList retVal;
	
	if (serviceData1.inputConnections && serviceData2.inputConnections){
		QList<sdl::agentino::Services::CInputConnection::V1_0> connections1 = *serviceData1.inputConnections;
		QList<sdl::agentino::Services::CInputConnection::V1_0> connections2 = *serviceData2.inputConnections;
		
		for (int i = 0; i < connections1.size(); i++){
			sdl::agentino::Services::CInputConnection::V1_0 connection1 = connections1[i];
			sdl::agentino::Services::CInputConnection::V1_0 connection2 = connections2[i];
			
			sdl::agentino::Services::CUrlParameter::V1_0 urlParam1 = *connection1.url;
			sdl::agentino::Services::CUrlParameter::V1_0 urlParam2 = *connection2.url;
			
			if (*urlParam1.scheme != *urlParam2.scheme ||
				*urlParam1.port != *urlParam2.port ||
				*urlParam1.host != *urlParam2.host){
				retVal << *connection1.id;
			}
		}
	}
	
	return retVal;
}


QByteArrayList CServiceControllerProxyComp::GetServiceIdsByDependantId(const QByteArray& connectionId) const
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

							if (connectionId == connectionParamPtr->GetDependantServiceConnectionId()){
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


bool CServiceControllerProxyComp::UpdateConnectionUrlForService(
	const QByteArray& serviceId,
	const QByteArray& agentId,
	const QByteArray& connectionId,
	const sdl::agentino::Services::CUrlParameter::V1_0& url) const
{
	sdl::agentino::Services::UpdateConnectionUrlRequestArguments arguments;
	arguments.input.Version_1_0 = sdl::agentino::Services::CConnectionUrlInput::V1_0();
	arguments.input.Version_1_0->serviceId = serviceId;
	arguments.input.Version_1_0->connectionId = connectionId;
	arguments.input.Version_1_0->url = url;
	
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
	sdl::agentino::Services::CUpdateConnectionUrlResponse::V1_0 response;
	if (!SendModelRequest<
			sdl::agentino::Services::CUpdateConnectionUrlResponse::V1_0,
			sdl::agentino::Services::CUpdateConnectionUrlResponse>(gqlRequest, response, errorMessage)){
		return false;
	}
	
	if (response.succesful.has_value()){
		return *response.succesful;
	}
	
	return false;
}


} // namespace agentinogql


