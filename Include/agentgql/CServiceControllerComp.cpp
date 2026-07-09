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
	}

	return response;
}


sdl::agentino::Services::CServiceSettingsPayload CServiceControllerComp::OnGetServiceSettings(
			const sdl::agentino::Services::CGetServiceSettingsGqlRequest& getServiceSettingsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CServiceSettingsPayload response;
	response.Version_1_0.emplace();
	response.Version_1_0->exists = false;

	sdl::agentino::Services::GetServiceSettingsRequestArguments arguments = getServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input.Version_1_0->serviceId.has_value()){
		errorMessage = QString("Unable to get service settings with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	response.Version_1_0->serviceId = serviceId;

	QString settingsPath = GetServiceSettingsFilePath(serviceId, errorMessage);
	if (settingsPath.isEmpty()){
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");
		}

		return response;
	}

	response.Version_1_0->path = settingsPath;

	QFileInfo fileInfo(settingsPath);
	if (!fileInfo.exists()){
		// No settings file yet: this is not an error, the editor can create one.
		response.Version_1_0->exists = false;
		response.Version_1_0->content = QString();

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

	response.Version_1_0->exists = true;
	response.Version_1_0->content = QString::fromUtf8(rawContent);

	return response;
}


sdl::agentino::Services::CServiceSettingsPayload CServiceControllerComp::OnUpdateServiceSettings(
			const sdl::agentino::Services::CUpdateServiceSettingsGqlRequest& updateServiceSettingsRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CServiceSettingsPayload response;
	response.Version_1_0.emplace();
	response.Version_1_0->exists = false;

	sdl::agentino::Services::UpdateServiceSettingsRequestArguments arguments = updateServiceSettingsRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input.Version_1_0->serviceId.has_value()){
		errorMessage = QString("Unable to update service settings with empty ID");
		SendErrorMessage(0, errorMessage, "CServiceControllerComp");

		return response;
	}

	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	response.Version_1_0->serviceId = serviceId;

	QString content;
	if (arguments.input.Version_1_0->content.has_value()){
		content = *arguments.input.Version_1_0->content;
	}

	QString settingsPath = GetServiceSettingsFilePath(serviceId, errorMessage);
	if (settingsPath.isEmpty()){
		if (!errorMessage.isEmpty()){
			SendErrorMessage(0, errorMessage, "CServiceControllerComp");
		}

		return response;
	}

	response.Version_1_0->path = settingsPath;

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

	response.Version_1_0->exists = true;
	response.Version_1_0->content = content;

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
	if (servicePath.isEmpty()){
		errorMessage = QString("Service '%1' has no path configured").arg(QString::fromUtf8(serviceId));

		return QString();
	}

	// The settings file must be located inside the service directory to avoid
	// writing outside of the service folder (e.g. through '..' segments).
	QDir serviceDir(QFileInfo(QString::fromUtf8(servicePath)).absolutePath());
	QString canonicalServiceDir = serviceDir.absolutePath();

	QFileInfo settingsFileInfo(QString::fromUtf8(settingsPath));
	if (!settingsFileInfo.isAbsolute()){
		settingsFileInfo.setFile(serviceDir, QString::fromUtf8(settingsPath));
	}

	QString resolvedSettingsPath = QDir::cleanPath(settingsFileInfo.absoluteFilePath());

	QString containmentPrefix = canonicalServiceDir.endsWith(QLatin1Char('/'))
		? canonicalServiceDir
		: canonicalServiceDir + QLatin1Char('/');
	if (resolvedSettingsPath != canonicalServiceDir && !resolvedSettingsPath.startsWith(containmentPrefix)){
		errorMessage = QString("Service settings path '%1' is outside of the service directory").arg(resolvedSettingsPath);

		return QString();
	}

	return resolvedSettingsPath;
}


} // namespace agentgql


