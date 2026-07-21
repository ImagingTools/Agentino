// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtBaseTypes.h>
#include <GeneratedFiles/imtbasesdl/SDL/1.0/CPP/ImtCollection.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

// ImtCore includes
#include <imtcom/CServerConnectionInterfaceParam.h>

// Agentino includes
#include <agentinodata/agentinodata.h>
#include <agentinodata/CServiceInfo.h>


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
	if (!m_serviceControllerCompPtr->StartService(serviceId)){
		return response;
	}

	agentinodata::IServiceStatusInfo::ServiceStatus state =  m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	response.status = state == agentinodata::IServiceStatusInfo::SS_RUNNING ?
			sdl::V1_0::agentino::ServiceStatus::RUNNING :
			sdl::V1_0::agentino::ServiceStatus::STARTING;

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

	response.status = sdl::V1_0::agentino::ServiceStatus::STOPPING;

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to stop service with empty ID");
		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	if (!m_serviceControllerCompPtr->StopService(serviceId)){
		errorMessage = QString("Unable to stop service '%1'").arg(QString::fromUtf8(serviceId));
		// Still report current supervisor state (may already be Stopped).
	}

	// Never C-cast IServiceStatusInfo ↔ SDL ServiceStatus (ordinals can drift).
	const agentinodata::IServiceStatusInfo::ServiceStatus state =
				m_serviceControllerCompPtr->GetServiceStatus(serviceId);
	switch (state){
	case agentinodata::IServiceStatusInfo::SS_RUNNING:
		response.status = sdl::V1_0::agentino::ServiceStatus::RUNNING;
		break;
	case agentinodata::IServiceStatusInfo::SS_STARTING:
		response.status = sdl::V1_0::agentino::ServiceStatus::STARTING;
		break;
	case agentinodata::IServiceStatusInfo::SS_STOPPING:
		response.status = sdl::V1_0::agentino::ServiceStatus::STOPPING;
		break;
	case agentinodata::IServiceStatusInfo::SS_NOT_RUNNING:
		response.status = sdl::V1_0::agentino::ServiceStatus::NOT_RUNNING;
		break;
	case agentinodata::IServiceStatusInfo::SS_RUNNING_IMPOSSIBLE:
		response.status = sdl::V1_0::agentino::ServiceStatus::RUNNING_IMPOSSIBLE;
		break;
	default:
		response.status = sdl::V1_0::agentino::ServiceStatus::UNDEFINED;
		break;
	}

	return response;
}


sdl::V1_0::imtbase::CRemovedNotificationPayload CServiceControllerComp::OnServicesRemove(
			const sdl::V1_0::agentino::CServicesRemoveGqlRequest& removeServiceRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::imtbase::CRemovedNotificationPayload response;

	if (!m_serviceCollectionCompPtr.IsValid()){
		errorMessage = QString("Unable to remove service(s). Component reference 'ServiceCollection' was not set");
		SendCriticalMessage(0, errorMessage);

		return response;
	}

	sdl::V1_0::agentino::ServicesRemoveRequestArguments arguments = removeServiceRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->elementIds.has_value()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArrayList elementIds = arguments.input->elementIds->ToList();
	if (elementIds.isEmpty()){
		errorMessage = QString("Unable to remove service(s). Error: Element-IDs not provided");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	imtbase::ICollectionInfo::Ids allElementIds = m_serviceCollectionCompPtr->GetElementIds();
	for (const QByteArray& elementId: elementIds){
		if (!allElementIds.contains(elementId)){
			errorMessage = QString("Unable to remove service. Service with ID '%1' does not exists").arg(qPrintable(elementId));
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");

			return response;
		}
	}

	if (m_serviceControllerCompPtr.IsValid()){
		for (const QByteArray& elementId: elementIds){
			agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceControllerCompPtr->GetServiceStatus(elementId);
			if (status == agentinodata::IServiceStatusInfo::SS_RUNNING || status == agentinodata::IServiceStatusInfo::SS_STARTING){
				if (!m_serviceControllerCompPtr->StopService(elementId)){
					errorMessage = QString("Unable to remove service. Service with ID '%1' cannot be stopped").arg(qPrintable(elementId));
					SendErrorMessage(0, errorMessage, "CServiceControllerComp");

					return response;
				}
			}
		}
	}

	if (!m_serviceCollectionCompPtr->RemoveElements(elementIds, nullptr)){
		errorMessage = QString("Unable to remove service(s). Error: Can't remove object with ID: '%1'").arg(qPrintable(elementIds.join(';')));
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	response.elementIds.Emplace();
	response.elementIds->FromList(elementIds);

	return response;
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


sdl::V1_0::agentino::CSetOutputConnectionResponse CServiceControllerComp::OnSetOutputConnection(
			const sdl::V1_0::agentino::CSetOutputConnectionGqlRequest& setOutputConnectionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CSetOutputConnectionResponse response;
	response.succesful = false;

	if (!m_serviceCollectionCompPtr.IsValid() || !m_connectionCollectionProviderCompPtr.IsValid()){
		Q_ASSERT_X(false, "Attributes 'ServiceCollection'/'ConnectionCollectionProvider' were not set", "CServiceControllerComp");
		errorMessage = QString("Service collection or connection collection provider is not available");

		return response;
	}

	sdl::V1_0::agentino::SetOutputConnectionRequestArguments arguments = setOutputConnectionRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->serviceId.has_value() || !arguments.input->connectionId.has_value()){
		errorMessage = QString("SetOutputConnection input is invalid");

		return response;
	}

	const QByteArray serviceId = *arguments.input->serviceId;
	const QByteArray connectionId = *arguments.input->connectionId;

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		errorMessage = QString("Service '%1' was not found").arg(QString::fromUtf8(serviceId));

		return response;
	}

	agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
	if (serviceInfoPtr == nullptr){
		errorMessage = QString("Service '%1' has no service info").arg(QString::fromUtf8(serviceId));

		return response;
	}

	imtbase::IObjectCollection* dependantConnectionsPtr = serviceInfoPtr->GetDependantServiceConnections();
	if (dependantConnectionsPtr == nullptr){
		errorMessage = QString("Service '%1' has no output connections").arg(QString::fromUtf8(serviceId));

		return response;
	}

	imtbase::IObjectCollection::DataPtr connectionDataPtr;
	if (!dependantConnectionsPtr->GetObjectData(connectionId, connectionDataPtr)){
		errorMessage = QString("Output connection '%1' was not found").arg(QString::fromUtf8(connectionId));

		return response;
	}

	imtservice::CUrlConnectionLinkParam* linkParamPtr =
				dynamic_cast<imtservice::CUrlConnectionLinkParam*>(connectionDataPtr.GetPtr());
	if (linkParamPtr == nullptr){
		errorMessage = QString("Output connection '%1' has an unexpected type").arg(QString::fromUtf8(connectionId));

		return response;
	}

	QString dependantConnectionId;
	if (arguments.input->dependantConnectionId.has_value()){
		dependantConnectionId = *arguments.input->dependantConnectionId;
	}

	sdl::V1_0::agentino::COutputConnection outputConnectionRepresentation;
	outputConnectionRepresentation.dependantConnectionId = dependantConnectionId;

	if (arguments.input->connectionParam.has_value()){
		// Pre-resolved by the server proxy, which can see producers across the whole fleet.
		outputConnectionRepresentation.connectionParam = *arguments.input->connectionParam;
	}
	else if (!dependantConnectionId.isEmpty()){
		// No pre-resolved param (e.g. a direct call to this agent): fall back to resolving
		// against this agent's own services only, same as OnAvailableConnections.
		QList<sdl::V1_0::agentino::CDependantConnectionInfo> candidates;
		agentinodata::AppendAvailableConnectionsFromServiceCollection(
					*m_serviceCollectionCompPtr.GetPtr(), connectionId, candidates);

		bool found = false;
		for (const sdl::V1_0::agentino::CDependantConnectionInfo& candidate : candidates){
			if (candidate.id.has_value() && QString::fromUtf8(*candidate.id) == dependantConnectionId){
				outputConnectionRepresentation.connectionParam = candidate.connectionParam;
				found = true;

				break;
			}
		}

		if (!found){
			errorMessage = QString("Producer connection '%1' is not available").arg(dependantConnectionId);

			return response;
		}
	}
	else{
		// Clearing the selection.
		sdl::V1_0::imtbase::CServerConnectionParam clearedParam;
		clearedParam.host = QStringLiteral("localhost");
		clearedParam.httpPort = -1;
		clearedParam.wsPort = -1;
		outputConnectionRepresentation.connectionParam = clearedParam;
	}

	if (!agentinodata::GetUrlConnectionLinkFromRepresentation(*linkParamPtr, outputConnectionRepresentation)){
		errorMessage = QString("Invalid connection parameters");

		return response;
	}

	dependantConnectionsPtr->SetObjectData(connectionId, *linkParamPtr);
	m_serviceCollectionCompPtr->SetObjectData(serviceId, *serviceInfoPtr);

	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr =
				m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServiceId(serviceId);
	if (connectionCollectionPtr.IsValid()){
		connectionCollectionPtr->SetServerConnectionInterface(connectionId, *linkParamPtr);
	}

	response.succesful = true;
	response.connectionParam = outputConnectionRepresentation.connectionParam;

	return response;
}


sdl::V1_0::agentino::CAvailableConnectionsPayload CServiceControllerComp::OnAvailableConnections(
			const sdl::V1_0::agentino::CAvailableConnectionsGqlRequest& availableConnectionsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& /*errorMessage*/) const
{
	sdl::V1_0::agentino::CAvailableConnectionsPayload response;
	response.outputConnections.Emplace();

	const sdl::V1_0::agentino::AvailableConnectionsRequestArguments arguments =
				availableConnectionsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()
				|| !arguments.input->connectionUsageIds.has_value()
				|| !m_serviceCollectionCompPtr.IsValid()){
		return response;
	}

	QList<sdl::V1_0::agentino::COutputConnectionCandidates> groups;
	const QByteArrayList usageIds = (*arguments.input->connectionUsageIds).ToList();
	for (const QByteArray& usageId : usageIds){
		QList<sdl::V1_0::agentino::CDependantConnectionInfo> candidates;
		agentinodata::AppendAvailableConnectionsFromServiceCollection(
					*m_serviceCollectionCompPtr.GetPtr(), usageId, candidates);

		sdl::V1_0::agentino::COutputConnectionCandidates group;
		group.connectionUsageId = usageId;
		group.candidates.Emplace();
		group.candidates->FromList(candidates);
		groups << group;
	}

	response.outputConnections->FromList(groups);

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
		errorMessage = QString("Unable to load plugin. Error: Component reference 'ConnectionCollectionProvider' was not set");

		return response;
	}

	sdl::V1_0::agentino::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.has_value() || !arguments.input->servicePath.has_value()){
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

		// Candidate lists are not filled here — the editor asks for them lazily via the
		// AvailableConnections query once the output slots are known.
	}
	else{
		// GetConnectionCollectionByServicePath() has no error-message out-param, so recover
		// the most likely reason here instead of silently returning a response with the
		// required 'servicePath' field unset - that used to fail JSON serialization further
		// up the call chain with a generic "Internal error. Unable to create response for
		// command-ID: 'LoadPlugin'" instead of a message that actually explains the problem.
		QFileInfo fileInfo(servicePath);
		if (!fileInfo.exists()){
			errorMessage = QString("Unable to load plugin. Error: Service path '%1' does not exist").arg(servicePath);
		}
		else{
			errorMessage = QString("Unable to load plugin. Error: No connection settings plugin found next to '%1'").arg(servicePath);
		}

		return response;
	}

	return response;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerComp::OnGetServiceSettings(
			const sdl::V1_0::agentino::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceSettingsPayload response;
	response.exists = false;

	sdl::V1_0::agentino::GetServiceSettingsRequestArguments arguments = getServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to get service settings with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	response.serviceId = serviceId;

	QString settingsPath = GetServiceSettingsFilePath(serviceId, errorMessage);
	if (settingsPath.isEmpty()){
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");
		}

		return response;
	}

	response.path = settingsPath;

	QFileInfo fileInfo(settingsPath);
	if (!fileInfo.exists()){
		// No settings file yet: this is not an error, the editor can create one.
		response.exists = false;
		response.content = QString();

		return response;
	}

	QFile file(settingsPath);
	if (!file.open(QIODevice::ReadOnly)){
		errorMessage = QString("Unable to read service settings file '%1'").arg(settingsPath);
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray rawContent = file.readAll();
	file.close();

	response.exists = true;
	response.content = QString::fromUtf8(rawContent);

	return response;
}


sdl::V1_0::agentino::CServiceSettingsPayload CServiceControllerComp::OnUpdateServiceSettings(
			const sdl::V1_0::agentino::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceSettingsPayload response;
	response.exists = false;

	sdl::V1_0::agentino::UpdateServiceSettingsRequestArguments arguments = updateServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input->serviceId.has_value()){
		errorMessage = QString("Unable to update service settings with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray serviceId = *arguments.input->serviceId;
	response.serviceId = serviceId;

	QString content;
	if (arguments.input->content.has_value()){
		content = *arguments.input->content;
	}

	QString settingsPath = GetServiceSettingsFilePath(serviceId, errorMessage);
	if (settingsPath.isEmpty()){
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");
		}

		return response;
	}

	response.path = settingsPath;

	QFileInfo fileInfo(settingsPath);
	QDir settingsDir = fileInfo.absoluteDir();
	if (!settingsDir.exists()){
		if (!settingsDir.mkpath(QStringLiteral("."))){
			errorMessage = QString("Unable to create directory for service settings file '%1'").arg(settingsPath);
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");

			return response;
		}
	}

	// Write atomically through a temporary file, so a failed write never
	// corrupts the existing settings file.
	QSaveFile saveFile(settingsPath);
	if (!saveFile.open(QIODevice::WriteOnly)){
		errorMessage = QString("Unable to open service settings file '%1' for writing").arg(settingsPath);
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray rawContent = content.toUtf8();
	if (saveFile.write(rawContent) != rawContent.size() || !saveFile.commit()){
		errorMessage = QString("Unable to write service settings file '%1'").arg(settingsPath);
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	response.exists = true;
	response.content = content;

	return response;
}


// private methods

QString CServiceControllerComp::GetServiceSettingsFilePath(const QByteArray& serviceId, QString& errorMessage) const
{
	if (!m_serviceCollectionCompPtr.IsValid()){
		errorMessage = QString("Attribute 'ServiceCollection' was not set");

		return QString();
	}

	agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		errorMessage = QString("Service '%1' was not found").arg(QString::fromUtf8(serviceId));

		return QString();
	}

	QByteArray settingsPath = serviceInfoPtr->GetServiceSettingsPath();
	if (settingsPath.isEmpty()){
		errorMessage = QString("Service '%1' has no settings path configured").arg(QString::fromUtf8(serviceId));

		return QString();
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();

	QFileInfo settingsFileInfo(QString::fromUtf8(settingsPath));
	if (!settingsFileInfo.isAbsolute()){
		// Relative paths are resolved against the service directory
		if (servicePath.isEmpty()){
			errorMessage = QString("Service '%1' has no path configured, cannot resolve relative settings path").arg(QString::fromUtf8(serviceId));

			return QString();
		}

		QDir serviceDir(QFileInfo(QString::fromUtf8(servicePath)).absolutePath());
		settingsFileInfo.setFile(serviceDir, QString::fromUtf8(settingsPath));
	}

	QString resolvedSettingsPath = QDir::cleanPath(settingsFileInfo.absoluteFilePath());

	return resolvedSettingsPath;
}


} // namespace agentgql


