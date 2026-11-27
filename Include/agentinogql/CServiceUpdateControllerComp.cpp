// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceUpdateControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QUrl>

// ImtCore includes
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlRequest.h>
#include <imtrest/CHttpRequest.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// Operating system families shown on the "Update" page, in the order of the panel.
static const QString s_windowsFamily = QStringLiteral("Windows");
static const QString s_linuxFamily = QStringLiteral("Linux");
static const QString s_macOsFamily = QStringLiteral("macOS");
static const QString s_otherFamily = QStringLiteral("Other");

// Suffix of the archives of a version: the files of the folder of the version sent to the agents.
static const QString s_archiveSuffix = QStringLiteral(".zip");


// public methods

// reimplemented (imtrest::IHttpFileResolver)

imtrest::IProtocolEngine::StatusCode CServiceUpdateControllerComp::ResolveFile(const imtrest::CHttpRequest& request, FileDescription& file) const
{
	if (!m_serviceVersionCollectionCompPtr.IsValid()){
		SendErrorMessage(0, QStringLiteral("Unable to send the archive of a service version. Error: Component reference 'ServiceVersionCollection' was not set"), "CServiceUpdateControllerComp");

		return imtrest::IProtocolEngine::SC_INTERNAL_ERROR;
	}

	// The path of the request: <FileCommandId>/<version ID>/<file name> (decoded).
	QString requestPath = QString::fromUtf8(request.GetCommandId());
	while (requestPath.startsWith(QLatin1Char('/'))){
		requestPath.remove(0, 1);
	}

	const QString commandPrefix = GetFileCommandId() + QLatin1Char('/');
	if (!requestPath.startsWith(commandPrefix)){
		return imtrest::IProtocolEngine::SC_NOT_FOUND;
	}

	const QStringList parts = requestPath.mid(commandPrefix.length()).split(QLatin1Char('/'));
	if (parts.count() != 2 || parts[0].isEmpty() || !IsArchiveFileName(parts[1])){
		return imtrest::IProtocolEngine::SC_NOT_FOUND;
	}

	const QByteArray versionId = parts[0].toUtf8();
	const QString fileName = parts[1];

	// The archives are given to the approved agents installing the versions (header 'clientid').
	const QByteArray agentId = request.GetHeaderValue(QByteArrayLiteral("clientid"));
	if (agentId.isEmpty()){
		SendWarningMessage(
					0,
					QStringLiteral("The archive '%1' of the service version '%2' was refused to %3: the request is not sent by an agent (header 'clientid')")
								.arg(fileName, QString::fromUtf8(versionId), request.GetRemoteAddress().toString()),
					"CServiceUpdateControllerComp");

		return imtrest::IProtocolEngine::SC_FORBIDDEN;
	}

	if (m_enrollmentControllerCompPtr.IsValid() && m_enrollmentControllerCompPtr->Get(agentId).status != EnrollmentStatus::Approved){
		SendWarningMessage(
					0,
					QStringLiteral("The archive '%1' of the service version '%2' was refused to the agent '%3': the agent is not approved")
								.arg(fileName, QString::fromUtf8(versionId), QString::fromUtf8(agentId)),
					"CServiceUpdateControllerComp");

		return imtrest::IProtocolEngine::SC_FORBIDDEN;
	}

	// Only the archives of the versions added to the server are given.
	imtbase::IObjectCollection::DataPtr versionDataPtr;
	const agentinodata::IServiceVersionInfo* versionInfoPtr = nullptr;
	if (m_serviceVersionCollectionCompPtr->GetObjectData(versionId, versionDataPtr)){
		versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(versionDataPtr.GetPtr());
	}

	if (versionInfoPtr == nullptr){
		return imtrest::IProtocolEngine::SC_NOT_FOUND;
	}

	const QString folderPath = GetCleanPath(versionInfoPtr->GetPath());
	if (folderPath.isEmpty()){
		return imtrest::IProtocolEngine::SC_NOT_FOUND;
	}

	// A file of the folder itself: a link leading out of the folder is not followed.
	const QFileInfo fileInfo(QDir(folderPath), fileName);
	const QString canonicalFolderPath = QFileInfo(folderPath).canonicalFilePath();
	if (!fileInfo.isFile() || canonicalFolderPath.isEmpty() || !IsSamePath(fileInfo.canonicalPath(), canonicalFolderPath)){
		return imtrest::IProtocolEngine::SC_NOT_FOUND;
	}

	// TEST: the archive is downloaded from another server - the agent is redirected to it (302, Location).
	// The agent follows the redirect (QNetworkAccessManager). Remove after the test.
	// static const QByteArray s_testArchiveUrl = QByteArrayLiteral("http://217.160.251.80:7380/Dogma/share/dsl1_VouOJnDB5XAW_miDGbKRUfohrryg8DLUbXHVdHrp-Mk");
	// if (!s_testArchiveUrl.isEmpty()){
	// 	SendInfoMessage(
	// 				0,
	// 				QStringLiteral("TEST: the archive '%1' of the service version '%2' is redirected to '%3' for the agent '%4'")
	// 							.arg(fileName, QString::fromUtf8(versionId), QString::fromLatin1(s_testArchiveUrl), QString::fromUtf8(agentId)),
	// 				"CServiceUpdateControllerComp");

	// 	file.headers.insert(QByteArrayLiteral("Location"), s_testArchiveUrl);
	// 	file.headers.insert(QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("no-store"));

	// 	return imtrest::IProtocolEngine::SC_MOVED_TEMPORARILY;
	// }

	file.filePath = fileInfo.absoluteFilePath();
	file.dataTypeId = QByteArrayLiteral("application/zip");

	return imtrest::IProtocolEngine::SC_OK;
}


// protected methods

// reimplemented (sdl::V1_0::agentino::CUpdatesGqlHandlerCompBase)

sdl::V1_0::agentino::CServiceUpdateListPayload CServiceUpdateControllerComp::OnServiceUpdatesList(
			const sdl::V1_0::agentino::CServiceUpdatesListGqlRequest& serviceUpdatesListRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceUpdateListPayload payload;

	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the service list. Error: Component references 'AgentCollection' or 'ServiceManager' were not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::ServiceUpdatesListRequestArguments arguments = serviceUpdatesListRequest.GetRequestedArguments();

	QString operatingSystemFamily;
	const QString* operatingSystemFamilyPtr = nullptr;
	if (arguments.input.has_value() && arguments.input->operatingSystem.has_value()){
		operatingSystemFamily = *arguments.input->operatingSystem;
		operatingSystemFamilyPtr = &operatingSystemFamily;
	}

	QList<sdl::V1_0::agentino::CServiceUpdateItem> items;

	const QMap<QString, int> serviceAgentCounts = GetServiceAgentCounts(operatingSystemFamilyPtr);
	for (QMap<QString, int>::const_iterator iter = serviceAgentCounts.constBegin(); iter != serviceAgentCounts.constEnd(); ++iter){
		sdl::V1_0::agentino::CServiceUpdateItem item;
		item.id = iter.key().toUtf8();
		item.name = iter.key();
		item.agentCount = iter.value();
		item.latestVersion = QString();
		// item.autoUpdate is not provided yet: left unset, the page shows no icon.

		items << item;
	}

	payload.items.Emplace();
	payload.items->FromList(items);

	return payload;
}


sdl::V1_0::agentino::CAgentOperatingSystemList CServiceUpdateControllerComp::OnAgentOperatingSystemsList(
			const sdl::V1_0::agentino::CAgentOperatingSystemsListGqlRequest& /*agentOperatingSystemsListRequest*/,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CAgentOperatingSystemList payload;

	if (!m_agentCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the operating systems of the agents. Error: Component reference 'AgentCollection' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	bool hasUnknown = false;
	const QStringList families = GetOperatingSystemFamilies(hasUnknown);

	payload.names.Emplace();
	payload.names->FromList(families);
	payload.hasUnknown = hasUnknown;

	return payload;
}


sdl::V1_0::agentino::CServiceVersionFolder CServiceUpdateControllerComp::OnGetServiceVersionFolder(
			const sdl::V1_0::agentino::CGetServiceVersionFolderGqlRequest& getServiceVersionFolderRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFolder payload;

	if (!m_serviceVersionFoldersCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the folder of the service versions. Error: Component reference 'ServiceVersionFolders' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::GetServiceVersionFolderRequestArguments arguments = getServiceVersionFolderRequest.GetRequestedArguments();

	QByteArray serviceTypeId;
	QString operatingSystem;
	if (arguments.input.has_value()){
		if (arguments.input->serviceTypeId.has_value()){
			serviceTypeId = *arguments.input->serviceTypeId;
		}
		if (arguments.input->operatingSystem.has_value()){
			operatingSystem = arguments.input->operatingSystem->trimmed();
		}
	}

	payload.serviceTypeId = serviceTypeId;
	payload.operatingSystem = operatingSystem;
	payload.path = QDir::toNativeSeparators(m_serviceVersionFoldersCompPtr->GetServiceFolder(serviceTypeId, operatingSystem));

	return payload;
}


sdl::V1_0::agentino::CServiceVersionFolder CServiceUpdateControllerComp::OnSetServiceVersionFolder(
			const sdl::V1_0::agentino::CSetServiceVersionFolderGqlRequest& setServiceVersionFolderRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFolder payload;

	if (!m_serviceVersionFoldersCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: Component reference 'ServiceVersionFolders' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::SetServiceVersionFolderRequestArguments arguments = setServiceVersionFolderRequest.GetRequestedArguments();

	QByteArray serviceTypeId;
	QString operatingSystem;
	QString path;
	if (arguments.input.has_value()){
		if (arguments.input->serviceTypeId.has_value()){
			serviceTypeId = *arguments.input->serviceTypeId;
		}
		if (arguments.input->operatingSystem.has_value()){
			operatingSystem = arguments.input->operatingSystem->trimmed();
		}
		if (arguments.input->path.has_value()){
			path = QDir::cleanPath(QDir::fromNativeSeparators(arguments.input->path->trimmed()));
		}
	}

	if (serviceTypeId.isEmpty() || operatingSystem.isEmpty()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: The service type or the operating system is not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	if (path.isEmpty() || !QFileInfo(path).isDir()){
		errorMessage = QStringLiteral("Unable to set the folder of the service versions. Error: The folder '%1' does not exist").arg(QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	// The model is stored by its auto persistence on the change.
	m_serviceVersionFoldersCompPtr->SetServiceFolder(serviceTypeId, operatingSystem, path);

	payload.serviceTypeId = serviceTypeId;
	payload.operatingSystem = operatingSystem;
	payload.path = QDir::toNativeSeparators(path);

	return payload;
}


sdl::V1_0::agentino::CInstallServiceVersionPayload CServiceUpdateControllerComp::OnInstallServiceVersion(
			const sdl::V1_0::agentino::CInstallServiceVersionGqlRequest& installServiceVersionRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CInstallServiceVersionPayload payload;

	if (!m_agentCollectionCompPtr.IsValid() || !m_serviceManagerCompPtr.IsValid() || !m_serviceVersionCollectionCompPtr.IsValid() || !m_asyncApiClientCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to install the service version. Error: Component references 'AgentCollection', 'ServiceManager', 'ServiceVersionCollection' or 'AsyncApiClient' were not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::InstallServiceVersionRequestArguments arguments = installServiceVersionRequest.GetRequestedArguments();

	QByteArray versionId;
	QByteArrayList agentIds;
	if (arguments.input.has_value()){
		if (arguments.input->versionId.has_value()){
			versionId = *arguments.input->versionId;
		}
		if (arguments.input->agentIds.has_value()){
			agentIds = arguments.input->agentIds->ToList();
		}
	}

	imtbase::IObjectCollection::DataPtr versionDataPtr;
	const agentinodata::IServiceVersionInfo* versionInfoPtr = nullptr;
	if (!versionId.isEmpty() && m_serviceVersionCollectionCompPtr->GetObjectData(versionId, versionDataPtr)){
		versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(versionDataPtr.GetPtr());
	}

	if (versionInfoPtr == nullptr){
		errorMessage = QStringLiteral("Unable to install the service version. Error: The version '%1' does not exist").arg(QString::fromUtf8(versionId));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const QString serviceTypeId = QString::fromUtf8(versionInfoPtr->GetServiceTypeId());
	const QString operatingSystem = versionInfoPtr->GetOperatingSystem();
	const QString version = versionInfoPtr->GetVersion();
	const QString path = GetCleanPath(versionInfoPtr->GetPath());

	if (path.isEmpty() || !QFileInfo(path).isDir()){
		errorMessage = QStringLiteral("Unable to install the service version '%1'. Error: The folder '%2' of the version does not exist")
					.arg(version, QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	// The agents download the archives of the version over HTTP.
	const QStringList archiveUrls = GetArchiveUrls(versionId, path);
	if (archiveUrls.isEmpty()){
		errorMessage = QStringLiteral("Unable to install the service version '%1'. Error: The folder '%2' of the version has no archive (*.zip)")
					.arg(version, QDir::toNativeSeparators(path));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	if (agentIds.isEmpty()){
		errorMessage = QStringLiteral("Unable to install the service version '%1'. Error: No agent was chosen").arg(version);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const imtbase::ICollectionInfo::Ids existingAgentIds = m_agentCollectionCompPtr->GetElementIds();

	QList<sdl::V1_0::agentino::CServiceVersionInstallation> installations;
	// The installations to start: indexes in installations
	QList<int> targetIndexes;

	for (const QByteArray& agentId : agentIds){
		sdl::V1_0::agentino::CServiceVersionInstallation agentInstallation;
		agentInstallation.agentId = agentId;
		agentInstallation.started = false;

		if (!existingAgentIds.contains(agentId)){
			agentInstallation.agentName = QString::fromUtf8(agentId);
			agentInstallation.message = QStringLiteral("The agent does not exist");
			installations << agentInstallation;

			continue;
		}

		const QString agentName = m_agentCollectionCompPtr->GetElementInfo(agentId, imtbase::ICollectionInfo::EIT_NAME).toString();
		agentInstallation.agentName = agentName.isEmpty() ? QString::fromUtf8(agentId) : agentName;

		if (m_enrollmentControllerCompPtr.IsValid() && m_enrollmentControllerCompPtr->Get(agentId).status != EnrollmentStatus::Approved){
			agentInstallation.message = QStringLiteral("The agent is not approved");
			installations << agentInstallation;

			continue;
		}

		// A version is built for one operating system family: it is installed on the agents of that family only.
		const QString agentOperatingSystem = GetAgentOperatingSystemFamily(agentId);
		if (!operatingSystem.isEmpty() && agentOperatingSystem != operatingSystem){
			agentInstallation.message = agentOperatingSystem.isEmpty()
						? QStringLiteral("The version is built for %1, the agent did not report its operating system").arg(operatingSystem)
						: QStringLiteral("The version is built for %1, the agent runs on %2").arg(operatingSystem, agentOperatingSystem);
			installations << agentInstallation;

			continue;
		}

		const QList<AgentService> services = GetAgentServicesOfType(agentId, serviceTypeId);
		if (services.isEmpty()){
			agentInstallation.message = QStringLiteral("The agent has no service '%1'").arg(serviceTypeId);
			installations << agentInstallation;

			continue;
		}

		// Every service of the type on the agent is updated.
		for (const AgentService& service : services){
			sdl::V1_0::agentino::CServiceVersionInstallation installation = agentInstallation;
			installation.serviceId = service.id;
			installation.serviceName = service.name;

			targetIndexes << installations.count();
			installations << installation;
		}
	}

	// The installation is tracked: the agents report its result with its ID, the web client asks for its state.
	QByteArray installationId;
	if (!targetIndexes.isEmpty() && m_installationTrackerCompPtr.IsValid()){
		QList<IServiceVersionInstallationTracker::ServiceInstallation> trackedServices;
		for (int index : std::as_const(targetIndexes)){
			const sdl::V1_0::agentino::CServiceVersionInstallation& installation = installations[index];

			IServiceVersionInstallationTracker::ServiceInstallation trackedService;
			trackedService.agentId = *installation.agentId;
			trackedService.agentName = *installation.agentName;
			trackedService.serviceId = *installation.serviceId;
			trackedService.serviceName = *installation.serviceName;

			trackedServices << trackedService;
		}

		installationId = m_installationTrackerCompPtr->CreateInstallation(version, trackedServices);
	}

	for (int index : std::as_const(targetIndexes)){
		sdl::V1_0::agentino::CServiceVersionInstallation& installation = installations[index];
		const QByteArray agentId = *installation.agentId;
		const QByteArray serviceId = *installation.serviceId;
		const QString agentName = *installation.agentName;
		const QString serviceName = *installation.serviceName;

		QString sendError;
		if (SendUpdateServiceVersionRequest(agentId, serviceId, version, archiveUrls, installationId, sendError)){
			installation.started = true;
			installation.state = GetInstallationStateName(IServiceVersionInstallationTracker::IS_RUNNING);

			SendInfoMessage(
						0,
						QStringLiteral("Installation of the version '%1' of the service '%2' started on the agent '%3', archives: %4")
									.arg(version, serviceName, agentName, archiveUrls.join(QStringLiteral(", "))),
						"CServiceUpdateControllerComp");
		}
		else{
			installation.message = sendError;

			if (!installationId.isEmpty()){
				m_installationTrackerCompPtr->FinishServiceInstallation(installationId, agentId, serviceId, false, sendError);
			}

			SendErrorMessage(
						0,
						QStringLiteral("Unable to install the version '%1' of the service '%2' on the agent '%3'. Error: %4")
									.arg(version, serviceName, agentName, sendError),
						"CServiceUpdateControllerComp");
		}
	}

	payload.installationId = installationId;
	payload.version = version;
	payload.path = QDir::toNativeSeparators(path);
	payload.urls.Emplace();
	payload.urls->FromList(archiveUrls);
	payload.installations.Emplace();
	payload.installations->FromList(installations);

	return payload;
}


sdl::V1_0::agentino::CInstallServiceVersionPayload CServiceUpdateControllerComp::OnGetServiceVersionInstallation(
			const sdl::V1_0::agentino::CGetServiceVersionInstallationGqlRequest& getServiceVersionInstallationRequest,
			const ::imtgql::CGqlRequest& /*gqlRequest*/,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CInstallServiceVersionPayload payload;

	if (!m_installationTrackerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to get the installation of the service version. Error: Component reference 'InstallationTracker' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const sdl::V1_0::agentino::GetServiceVersionInstallationRequestArguments arguments = getServiceVersionInstallationRequest.GetRequestedArguments();

	QByteArray installationId;
	if (arguments.input.has_value() && arguments.input->installationId.has_value()){
		installationId = *arguments.input->installationId;
	}

	IServiceVersionInstallationTracker::Installation installation;
	if (installationId.isEmpty() || !m_installationTrackerCompPtr->GetInstallation(installationId, installation)){
		errorMessage = QStringLiteral("Unable to get the installation of the service version. Error: The installation '%1' does not exist")
					.arg(QString::fromUtf8(installationId));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	QList<sdl::V1_0::agentino::CServiceVersionInstallation> installations;
	for (const IServiceVersionInstallationTracker::ServiceInstallation& service : std::as_const(installation.services)){
		sdl::V1_0::agentino::CServiceVersionInstallation item;
		item.agentId = service.agentId;
		item.agentName = service.agentName;
		item.serviceId = service.serviceId;
		item.serviceName = service.serviceName;
		item.started = true;
		item.state = GetInstallationStateName(service.state);
		item.message = service.message;

		installations << item;
	}

	payload.installationId = installation.installationId;
	payload.version = installation.version;
	payload.installations.Emplace();
	payload.installations->FromList(installations);

	return payload;
}


sdl::V1_0::agentino::CServiceVersionInstallationReportResponse CServiceUpdateControllerComp::OnReportServiceVersionInstallation(
			const sdl::V1_0::agentino::CReportServiceVersionInstallationGqlRequest& reportServiceVersionInstallationRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionInstallationReportResponse response;
	response.accepted = false;

	if (!m_installationTrackerCompPtr.IsValid()){
		errorMessage = QStringLiteral("Unable to accept the result of the installation. Error: Component reference 'InstallationTracker' was not set");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return response;
	}

	// The result is reported by the agent the service belongs to.
	const QByteArray agentId = GetRequestAgentId(gqlRequest);
	if (agentId.isEmpty()){
		errorMessage = QStringLiteral("Unable to accept the result of the installation. Error: The request is not sent by an agent (header 'clientid')");
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return response;
	}

	if (m_enrollmentControllerCompPtr.IsValid() && m_enrollmentControllerCompPtr->Get(agentId).status != EnrollmentStatus::Approved){
		errorMessage = QStringLiteral("Unable to accept the result of the installation. Error: The agent '%1' is not approved").arg(QString::fromUtf8(agentId));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return response;
	}

	const sdl::V1_0::agentino::ReportServiceVersionInstallationRequestArguments arguments = reportServiceVersionInstallationRequest.GetRequestedArguments();

	QByteArray installationId;
	QByteArray serviceId;
	bool succeeded = false;
	QString message;
	if (arguments.input.has_value()){
		if (arguments.input->installationId.has_value()){
			installationId = *arguments.input->installationId;
		}
		if (arguments.input->serviceId.has_value()){
			serviceId = *arguments.input->serviceId;
		}
		if (arguments.input->succeeded.has_value()){
			succeeded = *arguments.input->succeeded;
		}
		if (arguments.input->message.has_value()){
			message = *arguments.input->message;
		}
	}

	if (!m_installationTrackerCompPtr->FinishServiceInstallation(installationId, agentId, serviceId, succeeded, message)){
		errorMessage = QStringLiteral("Unable to accept the result of the installation. Error: The installation '%1' has no service '%2' of the agent '%3'")
					.arg(QString::fromUtf8(installationId), QString::fromUtf8(serviceId), QString::fromUtf8(agentId));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return response;
	}

	if (succeeded){
		SendInfoMessage(
					0,
					QStringLiteral("The agent '%1' has installed the version on the service '%2'").arg(QString::fromUtf8(agentId), QString::fromUtf8(serviceId)),
					"CServiceUpdateControllerComp");
	}
	else{
		SendWarningMessage(
					0,
					QStringLiteral("The agent '%1' has not installed the version on the service '%2'. Error: %3")
								.arg(QString::fromUtf8(agentId), QString::fromUtf8(serviceId), message),
					"CServiceUpdateControllerComp");
	}

	response.accepted = true;

	return response;
}


// public static methods

QString CServiceUpdateControllerComp::GetOperatingSystemFamily(const QString& operatingSystem)
{
	if (operatingSystem.trimmed().isEmpty()){
		return QString();
	}

	if (operatingSystem.contains(QStringLiteral("windows"), Qt::CaseInsensitive)){
		return s_windowsFamily;
	}

	static const QStringList macOsKeys = {
		QStringLiteral("macos"), QStringLiteral("mac os"), QStringLiteral("os x"), QStringLiteral("osx")
	};
	for (const QString& key : macOsKeys){
		if (operatingSystem.contains(key, Qt::CaseInsensitive)){
			return s_macOsFamily;
		}
	}

	// QSysInfo::prettyProductName() of a Linux system is PRETTY_NAME of /etc/os-release,
	// which does not always mention "Linux" (e.g. "Ubuntu 24.04 LTS").
	static const QStringList linuxKeys = {
		QStringLiteral("linux"), QStringLiteral("ubuntu"), QStringLiteral("debian"), QStringLiteral("fedora"),
		QStringLiteral("centos"), QStringLiteral("red hat"), QStringLiteral("rhel"), QStringLiteral("suse"),
		QStringLiteral("mint"), QStringLiteral("manjaro"), QStringLiteral("alpine"), QStringLiteral("rocky"),
		QStringLiteral("raspbian"), QStringLiteral("gentoo"), QStringLiteral("kali"), QStringLiteral("elementary"),
		QStringLiteral("pop!_os"), QStringLiteral("zorin"), QStringLiteral("astra")
	};
	for (const QString& key : linuxKeys){
		if (operatingSystem.contains(key, Qt::CaseInsensitive)){
			return s_linuxFamily;
		}
	}

	return s_otherFamily;
}


// private methods

QString CServiceUpdateControllerComp::GetAgentOperatingSystemFamily(const QByteArray& agentId) const
{
	// The agent reports its OS with every announce; the enrollment store keeps the latest value.
	if (!m_enrollmentControllerCompPtr.IsValid()){
		return QString();
	}

	return GetOperatingSystemFamily(m_enrollmentControllerCompPtr->Get(agentId).os);
}


QStringList CServiceUpdateControllerComp::GetOperatingSystemFamilies(bool& hasUnknown) const
{
	hasUnknown = false;

	QSet<QString> families;

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		const QString family = GetAgentOperatingSystemFamily(agentId);
		if (family.isEmpty()){
			hasUnknown = true;
		}
		else{
			families.insert(family);
		}
	}

	QStringList retVal;
	for (const QString& family : {s_windowsFamily, s_linuxFamily, s_macOsFamily, s_otherFamily}){
		if (families.contains(family)){
			retVal << family;
		}
	}

	return retVal;
}


QMap<QString, int> CServiceUpdateControllerComp::GetServiceAgentCounts(const QString* operatingSystemFamilyPtr) const
{
	QMap<QString, int> retVal;

	const imtbase::ICollectionInfo::Ids agentIds = m_agentCollectionCompPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& agentId : agentIds){
		if (operatingSystemFamilyPtr != nullptr && GetAgentOperatingSystemFamily(agentId) != *operatingSystemFamilyPtr){
			continue;
		}

		imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
		if (serviceCollectionPtr == nullptr){
			continue;
		}

		QSet<QString> agentServiceTypes;

		const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
			imtbase::IObjectCollection::DataPtr serviceDataPtr;
			if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
				continue;
			}

			const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
			if (serviceInfoPtr == nullptr){
				continue;
			}

			QString serviceType = serviceInfoPtr->GetServiceTypeId();
			if (serviceType.isEmpty()){
				serviceType = serviceInfoPtr->GetServiceName();
			}

			if (!serviceType.isEmpty()){
				agentServiceTypes.insert(serviceType);
			}
		}

		for (const QString& serviceType : agentServiceTypes){
			retVal[serviceType]++;
		}
	}

	return retVal;
}


QList<CServiceUpdateControllerComp::AgentService> CServiceUpdateControllerComp::GetAgentServicesOfType(
			const QByteArray& agentId,
			const QString& serviceTypeId) const
{
	QList<AgentService> retVal;

	if (serviceTypeId.isEmpty() || !m_serviceManagerCompPtr.IsValid()){
		return retVal;
	}

	imtbase::IObjectCollection* serviceCollectionPtr = m_serviceManagerCompPtr->GetServiceCollection(agentId);
	if (serviceCollectionPtr == nullptr){
		return retVal;
	}

	const imtbase::ICollectionInfo::Ids serviceIds = serviceCollectionPtr->GetElementIds();
	for (const imtbase::ICollectionInfo::Id& serviceId : serviceIds){
		imtbase::IObjectCollection::DataPtr serviceDataPtr;
		if (!serviceCollectionPtr->GetObjectData(serviceId, serviceDataPtr)){
			continue;
		}

		const agentinodata::IServiceInfo* serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
		if (serviceInfoPtr == nullptr){
			continue;
		}

		// The same matching as on the "Update" page (GetServiceAgentCounts).
		QString serviceType = serviceInfoPtr->GetServiceTypeId();
		if (serviceType.isEmpty()){
			serviceType = serviceInfoPtr->GetServiceName();
		}

		if (serviceType == serviceTypeId){
			AgentService service;
			service.id = serviceId;
			service.name = serviceInfoPtr->GetServiceName().isEmpty() ? QString::fromUtf8(serviceId) : serviceInfoPtr->GetServiceName();

			retVal << service;
		}
	}

	return retVal;
}


bool CServiceUpdateControllerComp::SendUpdateServiceVersionRequest(
			const QByteArray& agentId,
			const QByteArray& serviceId,
			const QString& version,
			const QStringList& archiveUrls,
			const QByteArray& installationId,
			QString& errorMessage) const
{
	if (!m_asyncApiClientCompPtr.IsValid()){
		errorMessage = QStringLiteral("Component reference 'AsyncApiClient' was not set");

		return false;
	}

	// Machine-to-agent request: the agent is addressed by 'clientid' only, no user token is attached
	// (as for the GetService requests of the service mirror, see CServiceControllerProxyComp).
	imtgql::CGqlRequest gqlRequest(imtgql::IGqlRequest::RT_MUTATION);

	imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
	imtgql::IGqlContext::Headers headers;
	headers.insert(QByteArrayLiteral("clientid"), agentId);
	gqlContextPtr->SetHeaders(headers);
	gqlRequest.SetGqlContext(gqlContextPtr);

	sdl::V1_0::agentino::UpdateServiceVersionRequestArguments arguments;
	arguments.input.emplace();
	arguments.input->serviceId = serviceId;
	arguments.input->version = version;
	// The agent downloads the archives by these URLs over HTTP (see ResolveFile).
	arguments.input->urls.Emplace();
	arguments.input->urls->FromList(archiveUrls);
	// The agent reports the result of the update with it (ReportServiceVersionInstallation).
	if (!installationId.isEmpty()){
		arguments.input->installationId = installationId;
	}

	if (!sdl::V1_0::agentino::CUpdateServiceVersionGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		errorMessage = QStringLiteral("Unable to create the request 'UpdateServiceVersion'");

		return false;
	}

	gqlRequest.AddSimpleField(QByteArrayLiteral("started"));

	const QString agentIdText = QString::fromUtf8(agentId);
	const QString serviceIdText = QString::fromUtf8(serviceId);

	// The installation tracker is a component of the server: it outlives the handlers of this thread.
	IServiceVersionInstallationTracker* trackerPtr = (!installationId.isEmpty() && m_installationTrackerCompPtr.IsValid())
				? m_installationTrackerCompPtr.GetPtr()
				: nullptr;

	// The agent answers at once: the update goes on in the agent, which reports its result to the server
	// (ReportServiceVersionInstallation) and to its log. The continuation runs on the thread of this component.
	CServiceUpdateControllerComp* self = const_cast<CServiceUpdateControllerComp*>(this);
	m_asyncApiClientCompPtr->SendModelRequest<sdl::V1_0::agentino::CUpdateServiceVersionResponse>(gqlRequest)
				.then(self, [self, trackerPtr, installationId, agentId, serviceId, agentIdText, serviceIdText, version](
							const imtclientgql::IAsyncGqlClient::GqlModelResult<sdl::V1_0::agentino::CUpdateServiceVersionResponse>& result){
					const bool started = result && result.model.started.has_value() && *result.model.started;
					if (!started){
						const QString error = result.errorMessage.isEmpty() ? QStringLiteral("The update was refused") : result.errorMessage;

						// The agent will not report: the installation on the service has ended.
						if (trackerPtr != nullptr){
							trackerPtr->FinishServiceInstallation(installationId, agentId, serviceId, false, error);
						}

						self->SendErrorMessage(
									0,
									QStringLiteral("The agent '%1' did not start the installation of the version '%2' of the service '%3'. Error: %4")
												.arg(agentIdText, version, serviceIdText, error),
									"CServiceUpdateControllerComp");
					}
				});

	return true;
}


QStringList CServiceUpdateControllerComp::GetArchiveUrls(const QByteArray& versionId, const QString& folderPath) const
{
	QStringList retVal;

	// The HTTP address of the server. Without it (or without its host) the agent gets the path of a URL
	// only and resolves it against the address of the server it is connected to.
	QUrl serverUrl;
	if (m_serverConnectionCompPtr.IsValid()){
		if (!m_serverConnectionCompPtr->GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, serverUrl) || serverUrl.host().isEmpty()){
			serverUrl = QUrl();
		}
	}

	const QString commandId = GetFileCommandId();

	// The archives of the folder itself (the subfolders are not given).
	const QFileInfoList entries = QDir(folderPath).entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& entry : entries){
		if (!IsArchiveFileName(entry.fileName())){
			continue;
		}

		QUrl url = serverUrl;
		url.setPath(QLatin1Char('/') + commandId + QLatin1Char('/') + QString::fromUtf8(versionId) + QLatin1Char('/') + entry.fileName());
		url.setQuery(QString());
		url.setFragment(QString());

		// Percent-encoded: a name with spaces or other reserved characters stays one path segment.
		retVal << QString::fromLatin1(url.toEncoded());
	}

	return retVal;
}


QString CServiceUpdateControllerComp::GetFileCommandId() const
{
	QString commandId = m_fileCommandIdAttrPtr.IsValid() ? QString::fromUtf8(*m_fileCommandIdAttrPtr).trimmed() : QString();
	while (commandId.startsWith(QLatin1Char('/'))){
		commandId.remove(0, 1);
	}

	while (commandId.endsWith(QLatin1Char('/'))){
		commandId.chop(1);
	}

	return commandId;
}


// private static methods

QByteArray CServiceUpdateControllerComp::GetRequestAgentId(const ::imtgql::CGqlRequest& gqlRequest)
{
	// The agent puts its ID into the header 'clientid' of its requests (the HTTP layer lowercases the names).
	QByteArray agentId = gqlRequest.GetHeader(QByteArrayLiteral("clientid"));
	if (!agentId.isEmpty()){
		return agentId;
	}

	const imtgql::IGqlContext* contextPtr = gqlRequest.GetRequestContext();
	if (contextPtr != nullptr){
		const imtgql::IGqlContext::Headers headers = contextPtr->GetHeaders();
		for (auto iter = headers.constBegin(); iter != headers.constEnd(); ++iter){
			if (QString::fromUtf8(iter.key()).compare(QStringLiteral("clientid"), Qt::CaseInsensitive) == 0){
				return iter.value();
			}
		}
	}

	return QByteArray();
}


QString CServiceUpdateControllerComp::GetInstallationStateName(IServiceVersionInstallationTracker::InstallationState state)
{
	switch (state){
	case IServiceVersionInstallationTracker::IS_SUCCEEDED:
		return QStringLiteral("Succeeded");

	case IServiceVersionInstallationTracker::IS_FAILED:
		return QStringLiteral("Failed");

	default:
		return QStringLiteral("Running");
	}
}


bool CServiceUpdateControllerComp::IsArchiveFileName(const QString& fileName)
{
	if (fileName.length() <= s_archiveSuffix.length() || !fileName.endsWith(s_archiveSuffix, Qt::CaseInsensitive)){
		return false;
	}

	// A name of a file of the folder itself: no path, no drive.
	return !fileName.contains(QLatin1Char('/')) && !fileName.contains(QLatin1Char('\\')) && !fileName.contains(QLatin1Char(':'));
}


QString CServiceUpdateControllerComp::GetCleanPath(const QString& path)
{
	const QString trimmedPath = path.trimmed();
	if (trimmedPath.isEmpty()){
		return QString();
	}

	return QDir::cleanPath(QDir::fromNativeSeparators(trimmedPath));
}


bool CServiceUpdateControllerComp::IsSamePath(const QString& path1, const QString& path2)
{
#ifdef Q_OS_WIN
	return path1.compare(path2, Qt::CaseInsensitive) == 0;
#else
	return path1 == path2;
#endif
}


} // namespace agentinogql
