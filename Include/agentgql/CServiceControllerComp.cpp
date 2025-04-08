#include <agentgql/CServiceControllerComp.h>


// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentgql
{


// protected methods

// reimplemented (sdl::agentino::Services::CGraphQlHandlerCompBase)


sdl::agentino::Services::CServiceStatusResponse CServiceControllerComp::OnStartService(
			const sdl::agentino::Services::CStartServiceGqlRequest& startServiceRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse response;
	
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");
		return response;
	}
	
	sdl::agentino::Services::StartServiceRequestArguments arguments = startServiceRequest.GetRequestedArguments();
	
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}
	
	response.Version_1_0.emplace();
	
	response.Version_1_0->status = sdl::agentino::Services::ServiceStatus::UNDEFINED;

	if (!arguments.input.Version_1_0->serviceId.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");
		return response;
	}
	
	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	
	m_serviceControllerCompPtr->StartService(serviceId);

	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus) state;
	
	return response;
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerComp::OnStopService(
			const sdl::agentino::Services::CStopServiceGqlRequest& stopServiceRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse response;
	
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");
		return response;
	}
	
	sdl::agentino::Services::StopServiceRequestArguments arguments = stopServiceRequest.GetRequestedArguments();
	
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}
	
	response.Version_1_0.emplace();
	response.Version_1_0->status = sdl::agentino::Services::ServiceStatus::UNDEFINED;
	
	if (!arguments.input.Version_1_0->serviceId.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		return response;
	}
	
	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	
	m_serviceControllerCompPtr->StopService(serviceId);

	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus) state;

	return response;
}


sdl::imtbase::ImtCollection::CRemovedNotificationPayload CServiceControllerComp::OnServicesRemove(
	const sdl::agentino::Services::CServicesRemoveGqlRequest& /*removeServiceRequest*/,
	const ::imtgql::CGqlRequest& /*gqlRequest*/,
	QString& /*errorMessage*/) const
{
	return sdl::imtbase::ImtCollection::CRemovedNotificationPayload();
}


sdl::agentino::Services::CServiceStatusResponse CServiceControllerComp::OnGetServiceStatus(
			const sdl::agentino::Services::CGetServiceStatusGqlRequest& getServiceStatusRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CServiceStatusResponse response;
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");
		return response;
	}
	
	sdl::agentino::Services::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}
	
	if (!arguments.input.Version_1_0->id.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		return response;
	}
	
	response.Version_1_0.emplace();
	
	QByteArray serviceId = *arguments.input.Version_1_0->id;
	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	response.Version_1_0->status = (sdl::agentino::Services::ServiceStatus) state;
	
	return response;
}


sdl::agentino::Services::CUpdateConnectionUrlResponse CServiceControllerComp::OnUpdateConnectionUrl(
			const sdl::agentino::Services::CUpdateConnectionUrlGqlRequest& updateConnectionUrlRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CUpdateConnectionUrlResponse response;
	
	if (!m_connectionCollectionProviderCompPtr.IsValid()){
		return response;
	}
	
	response.Version_1_0.emplace();
	response.Version_1_0->succesful = false;
	
	sdl::agentino::Services::UpdateConnectionUrlRequestArguments arguments = updateConnectionUrlRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}
	
	if (!arguments.input.Version_1_0->serviceId.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		return response;
	}
	
	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	QByteArray connectionId = *arguments.input.Version_1_0->connectionId;
	sdl::agentino::Services::CUrlParameter::V1_0 urlParam = *arguments.input.Version_1_0->url;
	
	QUrl url;
	if (!agentinodata::GetUrlParamFromRepresentation(url, urlParam)){
		errorMessage = QString("Unable to start service with empty ID");
		return response;
	}
	
	std::shared_ptr<imtservice::IConnectionCollection> connectionCollectionPtr = m_connectionCollectionProviderCompPtr->GetConnectionCollection(serviceId);
	if (connectionCollectionPtr != nullptr){
		bool ok = connectionCollectionPtr->SetUrl(connectionId, url);
		
		response.Version_1_0->succesful = ok;
	}
	
	return response;
}


} // namespace agentgql


