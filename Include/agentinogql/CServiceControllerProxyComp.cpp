// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceControllerProxyComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


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
		return sdl::V1_0::agentino::CServiceData();
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
		errorMessage = QString("Unable to get service from representation");
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
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	SetServiceStatus(serviceId, agentinodata::IServiceStatusInfo::SS_NOT_RUNNING);

	sdl::V1_0::agentino::CServiceData serviceData;
	if (arguments.input->item){
		serviceData = *arguments.input->item;
	}

	// We take the document from the agent immediately after adding it to download the connection information.
	sdl::V1_0::agentino::GetServiceRequestArguments getArguments;
	getArguments.input.emplace();
	getArguments.input->id = serviceId;

	imtgql::CGqlRequest getGqlRequest;

	imtgql::IGqlContextSharedPtr contextPtr;
	contextPtr.MoveCastedPtr(gqlRequest.GetRequestContext()->CloneMe());
	getGqlRequest.SetGqlContext(contextPtr);

	if (sdl::V1_0::agentino::CGetServiceGqlRequest::SetupGqlRequest(getGqlRequest, getArguments)){
		sdl::V1_0::agentino::CServiceData getServiceResponse = SendModelRequest<sdl::V1_0::agentino::CServiceData>(getGqlRequest, errorMessage);
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

			return sdl::V1_0::imtbase::CAddedNotificationPayload();
		}
	}

	istd::TDelPtr<agentinodata::CIdentifiableServiceInfo> serviceInfoPtr;
	serviceInfoPtr.SetPtr(new agentinodata::CIdentifiableServiceInfo);

	if (!agentinodata::GetServiceFromRepresentation(*serviceInfoPtr, serviceData, errorMessage)){
		errorMessage = QString("Unable to get service from representation");
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");

		return sdl::V1_0::imtbase::CAddedNotificationPayload();
	}

	serviceInfoPtr->SetObjectUuid(serviceId);

	QString serviceName = serviceInfoPtr->GetServiceName();
	QString serviceDescription = serviceInfoPtr->GetServiceDescription();

	if (!m_serviceManagerCompPtr->AddService(agentId, *serviceInfoPtr.PopPtr(), serviceId, serviceName, serviceDescription)){
		errorMessage = QString("Unable to add service '%1'").arg(qPrintable(serviceId));
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
	if (!errorMessage.isEmpty()){
		SendErrorMessage(0, errorMessage, "CServiceStatusControllerProxyComp");
		return sdl::V1_0::imtbase::CRemovedNotificationPayload();
	}

	if (!m_serviceManagerCompPtr->RemoveServices(agentId, imtbase::ICollectionInfo::Ids(serviceIds.begin(), serviceIds.end()))){
		SendErrorMessage(
			0,
			QString("Unable to remove service '%1' from agent '%2'").arg(qPrintable(serviceIds.join(';')), qPrintable(agentId)),
			"CServiceControllerProxyComp");
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

	return QJsonObject();
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
					sdl::V1_0::imtbase::CServerConnectionParam sdlRepresentation;
					if (agentinodata::GetRepresentationFromServerConnectionParam(*connectionParamPtr, sdlRepresentation)){
						sdl::V1_0::agentino::CDependantConnectionInfo dependantConnectionInfo;

						QString host = connectionParamPtr->GetHost();
						if (host.isEmpty()){
							host = "localhost";
						}

						int httpPort = connectionParamPtr->GetPort(imtcom::IServerConnectionInterface::PT_HTTP);

						dependantConnectionInfo.id = connectionElementId;
						dependantConnectionInfo.name = host + "/" + QString::number(httpPort);
						dependantConnectionInfo.connectionParam = sdlRepresentation;

						*retVal << dependantConnectionInfo;

						imtservice::IServiceConnectionParam::IncomingConnectionList incomingConnections = connectionParamPtr->GetIncomingConnections();

						for (const imtservice::IServiceConnectionParam::IncomingConnectionParam& incomingConnection : incomingConnections){
							sdl::V1_0::agentino::CDependantConnectionInfo incomingConnectionInfo;
							incomingConnectionInfo.id = incomingConnection.GetObjectUuid();

							QString incomingHost = incomingConnection.GetHost();
							int incommingHttpPort = incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_HTTP);
							incomingConnectionInfo.name = incomingHost + "/" + QString::number(incommingHttpPort);

							sdl::V1_0::imtbase::CServerConnectionParam connectionParam;
							connectionParam.host = incomingHost;
							connectionParam.wsPort = incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_WEBSOCKET);
							connectionParam.httpPort = incomingConnection.GetPort(imtcom::IServerConnectionInterface::PT_HTTP);

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


} // namespace agentinogql

