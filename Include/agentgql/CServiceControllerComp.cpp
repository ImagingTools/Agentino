// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// ImtCore includes
#include <imtcom/CServerConnectionInterfaceParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentgql
{


// protected methods

// reimplemented (sdl::V1_0::agentino::CServicesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerComp::OnStartService(
			const sdl::V1_0::agentino::CStartServiceGqlRequest& startServiceRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceStatusResponse response;

	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");

		return response;
	}

	sdl::V1_0::agentino::StartServiceRequestArguments arguments = startServiceRequest.GetRequestedArguments();

	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	response.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}
	QByteArray serviceId = *arguments.input->serviceId;
	m_serviceControllerCompPtr->StartService(serviceId);
	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	response.status = (sdl::V1_0::agentino::ServiceStatus) state;

	return response;
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerComp::OnStopService(
			const sdl::V1_0::agentino::CStopServiceGqlRequest& stopServiceRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceStatusResponse response;

	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");

		return response;
	}

	sdl::V1_0::agentino::StopServiceRequestArguments arguments = stopServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	response.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to start service with empty ID");
		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	m_serviceControllerCompPtr->StopService(serviceId);
	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	response.status = (sdl::V1_0::agentino::ServiceStatus) state;

	return response;
}


sdl::V1_0::imtbase::CRemovedNotificationPayload CServiceControllerComp::OnServicesRemove(
	const sdl::V1_0::agentino::CServicesRemoveGqlRequest& /*removeServiceRequest*/,
	const ::imtgql::CGqlRequest& /*gqlRequest*/,
	QString& /*errorMessage*/) const
{
	return sdl::V1_0::imtbase::CRemovedNotificationPayload();
}


sdl::V1_0::agentino::CServiceStatusResponse CServiceControllerComp::OnGetServiceStatus(
			const sdl::V1_0::agentino::CGetServiceStatusGqlRequest& getServiceStatusRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceStatusResponse response;
	if (!m_serviceControllerCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attribute 'ServiceController' was not set", "CServiceControllerComp");

		return response;
	}

	sdl::V1_0::agentino::GetServiceStatusRequestArguments arguments = getServiceStatusRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input->id.has_value()){
		errorMessage = QString("Unable to start service with empty ID");

		return response;
	}


	QByteArray serviceId = *arguments.input->id;
	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	response.status = (sdl::V1_0::agentino::ServiceStatus) state;

	return response;
}


sdl::V1_0::agentino::CUpdateConnectionUrlResponse CServiceControllerComp::OnUpdateConnectionUrl(
			const sdl::V1_0::agentino::CUpdateConnectionUrlGqlRequest& updateConnectionUrlRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CUpdateConnectionUrlResponse response;
	if (!m_connectionCollectionProviderCompPtr.IsValid()){
		return response;
	}

	response.succesful = false;
	sdl::V1_0::agentino::UpdateConnectionUrlRequestArguments arguments = updateConnectionUrlRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("ServiceId is invalid");

		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	QByteArray connectionId = *arguments.input->connectionId;
	sdl::V1_0::imtbase::CServerConnectionParam connectionParam = *arguments.input->connectionParam;

	imtcom::CServerConnectionInterfaceParam serverConnectionParam;
	serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_HTTP);
	serverConnectionParam.RegisterProtocol(imtcom::IServerConnectionInterface::PT_WEBSOCKET);

	if (!agentinodata::GetServerConnectionParamFromRepresentation(serverConnectionParam, connectionParam)){
		errorMessage = QString("ServerConnectionParam is invalid");

		return response;
	}

	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServiceId(serviceId);
	if (connectionCollectionPtr.IsValid()){
		bool ok = connectionCollectionPtr->SetServerConnectionInterface(connectionId, serverConnectionParam);

		response.succesful = ok;
	}

	return response;
}


sdl::V1_0::agentino::CPluginInfo CServiceControllerComp::OnLoadPlugin(
			const sdl::V1_0::agentino::CLoadPluginGqlRequest& loadPluginRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CPluginInfo response;
	if (!m_connectionCollectionProviderCompPtr.IsValid()){
		Q_ASSERT(false);

		return response;
	}

	sdl::V1_0::agentino::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}

	if (!arguments.input->servicePath.has_value()){
		errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

		return response;
	}


	QString servicePath = *arguments.input->servicePath;
	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServicePath(servicePath);
	if (connectionCollectionPtr.IsValid()){
		sdl::V1_0::agentino::CPluginInfo pluginRepresentation;
		if (!agentinodata::GetRepresentationFromConnectionCollection(*connectionCollectionPtr, pluginRepresentation)){
			errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

			return response;
		}

		response = pluginRepresentation;
		response.servicePath = servicePath;
	}

	return response;
}


} // namespace agentgql


