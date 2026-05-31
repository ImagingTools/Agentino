// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceControllerComp.h>


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
		errorMessage = QString("ServiceId is invalid");

		return response;
	}

	QByteArray serviceId = *arguments.input.Version_1_0->serviceId;
	QByteArray connectionId = *arguments.input.Version_1_0->connectionId;
	sdl::imtbase::ImtBaseTypes::CServerConnectionParam::V1_0 connectionParam = *arguments.input.Version_1_0->connectionParam;

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

		response.Version_1_0->succesful = ok;
	}

	return response;
}


sdl::agentino::Services::CPluginInfo CServiceControllerComp::OnLoadPlugin(
			const sdl::agentino::Services::CLoadPluginGqlRequest& loadPluginRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::agentino::Services::CPluginInfo response;
	if (!m_connectionCollectionProviderCompPtr.IsValid()){
		Q_ASSERT(false);

		return response;
	}

	sdl::agentino::Services::LoadPluginRequestArguments arguments = loadPluginRequest.GetRequestedArguments();
	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");

		return response;
	}

	if (!arguments.input.Version_1_0.has_value()){
		Q_ASSERT_X(false, "Version 1.0 is invalid", "CServiceControllerComp");
		return response;
	}

	if (!arguments.input.Version_1_0->servicePath.has_value()){
		errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

		return response;
	}

	response.Version_1_0.emplace();

	QString servicePath = *arguments.input.Version_1_0->servicePath;
	imtservice::IConnectionCollectionSharedPtr connectionCollectionPtr = m_connectionCollectionProviderCompPtr->GetConnectionCollectionByServicePath(servicePath);
	if (connectionCollectionPtr.IsValid()){
		sdl::agentino::Services::CPluginInfo::V1_0 pluginRepresentation;
		if (!agentinodata::GetRepresentationFromConnectionCollection(*connectionCollectionPtr, pluginRepresentation)){
			errorMessage = QString("Unable to load plugin. Error: Service path is invalid");

			return response;
		}

		response.Version_1_0 = pluginRepresentation;
		response.Version_1_0->servicePath = servicePath;
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
		errorMessage = QString("Service '%1' was not found").arg(qPrintable(QString::fromUtf8(serviceId)));

		return QString();
	}

	QByteArray settingsPath = serviceInfoPtr->GetServiceSettingsPath();
	if (settingsPath.isEmpty()){
		errorMessage = QString("Service '%1' has no settings path configured").arg(qPrintable(QString::fromUtf8(serviceId)));

		return QString();
	}

	QByteArray servicePath = serviceInfoPtr->GetServicePath();
	if (servicePath.isEmpty()){
		errorMessage = QString("Service '%1' has no path configured").arg(qPrintable(QString::fromUtf8(serviceId)));

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


