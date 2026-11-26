// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceUpdateControllerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Services.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>

// ImtCore includes
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinogql
{


// Operating system families shown on the "Update" page, in the order of the panel.
static const QString s_windowsFamily = QStringLiteral("Windows");
static const QString s_linuxFamily = QStringLiteral("Linux");
static const QString s_macOsFamily = QStringLiteral("macOS");
static const QString s_otherFamily = QStringLiteral("Other");

// Largest part of a file of a version given at once (GetServiceVersionFileChunk).
static const int s_maxFileChunkSize = 1024 * 1024;


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


sdl::V1_0::agentino::CServiceVersionFileList CServiceUpdateControllerComp::OnGetServiceVersionFiles(
			const sdl::V1_0::agentino::CGetServiceVersionFilesGqlRequest& getServiceVersionFilesRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFileList payload;

	const sdl::V1_0::agentino::GetServiceVersionFilesRequestArguments arguments = getServiceVersionFilesRequest.GetRequestedArguments();

	QString path;
	if (arguments.input.has_value() && arguments.input->path.has_value()){
		path = *arguments.input->path;
	}

	QString folderPath;
	if (!CheckVersionFileRequest(gqlRequest, path, folderPath, errorMessage)){
		errorMessage = QStringLiteral("Unable to get the files of the service version. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	QList<sdl::V1_0::agentino::CServiceVersionFile> files;

	// The files of the folder itself: the archive of the version (the subfolders are not given).
	const QFileInfoList entries = QDir(folderPath).entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& entry : entries){
		sdl::V1_0::agentino::CServiceVersionFile file;
		file.name = entry.fileName();
		file.size = entry.size();

		files << file;
	}

	payload.path = QDir::toNativeSeparators(folderPath);
	payload.files.Emplace();
	payload.files->FromList(files);

	return payload;
}


sdl::V1_0::agentino::CServiceVersionFileChunk CServiceUpdateControllerComp::OnGetServiceVersionFileChunk(
			const sdl::V1_0::agentino::CGetServiceVersionFileChunkGqlRequest& getServiceVersionFileChunkRequest,
			const ::imtgql::CGqlRequest& gqlRequest,
			QString& errorMessage) const
{
	sdl::V1_0::agentino::CServiceVersionFileChunk payload;

	const sdl::V1_0::agentino::GetServiceVersionFileChunkRequestArguments arguments = getServiceVersionFileChunkRequest.GetRequestedArguments();

	QString path;
	QString fileName;
	qlonglong offset = -1;
	int size = 0;
	if (arguments.input.has_value()){
		if (arguments.input->path.has_value()){
			path = *arguments.input->path;
		}
		if (arguments.input->fileName.has_value()){
			fileName = *arguments.input->fileName;
		}
		if (arguments.input->offset.has_value()){
			offset = *arguments.input->offset;
		}
		if (arguments.input->size.has_value()){
			size = *arguments.input->size;
		}
	}

	QString folderPath;
	if (!CheckVersionFileRequest(gqlRequest, path, folderPath, errorMessage)){
		errorMessage = QStringLiteral("Unable to get the file of the service version. Error: %1").arg(errorMessage);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	// A file of the folder itself only: no path to another folder.
	if (fileName.isEmpty() || fileName.contains(QLatin1Char('/')) || fileName.contains(QLatin1Char('\\')) ||
				fileName == QStringLiteral(".") || fileName == QStringLiteral("..")){
		errorMessage = QStringLiteral("Unable to get the file of the service version. Error: The file name '%1' is invalid").arg(fileName);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const QFileInfo fileInfo(QDir(folderPath), fileName);
	if (!fileInfo.isFile()){
		errorMessage = QStringLiteral("Unable to get the file of the service version. Error: The file '%1' does not exist").arg(QDir::toNativeSeparators(fileInfo.filePath()));
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	if (offset < 0 || offset > fileInfo.size() || size <= 0){
		errorMessage = QStringLiteral("Unable to get the file of the service version. Error: The part (offset %1, size %2) of the file '%3' is invalid")
					.arg(offset).arg(size).arg(fileName);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	QFile file(fileInfo.filePath());
	if (!file.open(QIODevice::ReadOnly) || !file.seek(offset)){
		errorMessage = QStringLiteral("Unable to get the file of the service version. Error: The file '%1' cannot be read: %2")
					.arg(QDir::toNativeSeparators(fileInfo.filePath()), file.errorString());
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const QByteArray data = file.read(qMin(size, s_maxFileChunkSize));

	payload.fileName = fileName;
	payload.offset = offset;
	payload.fileSize = fileInfo.size();
	payload.content = QString::fromLatin1(data.toBase64());

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

	if (agentIds.isEmpty()){
		errorMessage = QStringLiteral("Unable to install the service version '%1'. Error: No agent was chosen").arg(version);
		SendErrorMessage(0, errorMessage, "CServiceUpdateControllerComp");

		return payload;
	}

	const imtbase::ICollectionInfo::Ids existingAgentIds = m_agentCollectionCompPtr->GetElementIds();

	QList<sdl::V1_0::agentino::CServiceVersionInstallation> installations;

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

			QString sendError;
			if (SendUpdateServiceVersionRequest(agentId, service.id, version, path, sendError)){
				installation.started = true;

				SendInfoMessage(
							0,
							QStringLiteral("Installation of the version '%1' (%2) of the service '%3' started on the agent '%4'")
										.arg(version, QDir::toNativeSeparators(path), service.name, *agentInstallation.agentName),
							"CServiceUpdateControllerComp");
			}
			else{
				installation.message = sendError;

				SendErrorMessage(
							0,
							QStringLiteral("Unable to install the version '%1' of the service '%2' on the agent '%3'. Error: %4")
										.arg(version, service.name, *agentInstallation.agentName, sendError),
							"CServiceUpdateControllerComp");
			}

			installations << installation;
		}
	}

	payload.version = version;
	payload.path = QDir::toNativeSeparators(path);
	payload.installations.Emplace();
	payload.installations->FromList(installations);

	return payload;
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
			const QString& path,
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
	// The agent asks the server for the files of this folder (GetServiceVersionFiles).
	arguments.input->path = QDir::toNativeSeparators(path);

	if (!sdl::V1_0::agentino::CUpdateServiceVersionGqlRequest::SetupGqlRequest(gqlRequest, arguments)){
		errorMessage = QStringLiteral("Unable to create the request 'UpdateServiceVersion'");

		return false;
	}

	gqlRequest.AddSimpleField(QByteArrayLiteral("started"));

	const QString agentIdText = QString::fromUtf8(agentId);
	const QString serviceIdText = QString::fromUtf8(serviceId);

	// The agent answers at once: the update goes on in the agent, which reports its result to its log.
	// The continuation runs on the thread of this component.
	CServiceUpdateControllerComp* self = const_cast<CServiceUpdateControllerComp*>(this);
	m_asyncApiClientCompPtr->SendModelRequest<sdl::V1_0::agentino::CUpdateServiceVersionResponse>(gqlRequest)
				.then(self, [self, agentIdText, serviceIdText, version](
							const imtclientgql::IAsyncGqlClient::GqlModelResult<sdl::V1_0::agentino::CUpdateServiceVersionResponse>& result){
					const bool started = result && result.model.started.has_value() && *result.model.started;
					if (!started){
						self->SendErrorMessage(
									0,
									QStringLiteral("The agent '%1' did not start the installation of the version '%2' of the service '%3'. Error: %4")
												.arg(agentIdText, version, serviceIdText,
													 result.errorMessage.isEmpty() ? QStringLiteral("The update was refused") : result.errorMessage),
									"CServiceUpdateControllerComp");
					}
				});

	return true;
}


bool CServiceUpdateControllerComp::CheckVersionFileRequest(
			const ::imtgql::CGqlRequest& gqlRequest,
			const QString& path,
			QString& folderPath,
			QString& errorMessage) const
{
	folderPath.clear();

	if (!m_serviceVersionCollectionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Component reference 'ServiceVersionCollection' was not set");

		return false;
	}

	// The files of the versions are given to the agents installing them.
	const QByteArray agentId = GetRequestAgentId(gqlRequest);
	if (agentId.isEmpty()){
		errorMessage = QStringLiteral("The request is not sent by an agent (header 'clientid')");

		return false;
	}

	if (m_enrollmentControllerCompPtr.IsValid() && m_enrollmentControllerCompPtr->Get(agentId).status != EnrollmentStatus::Approved){
		errorMessage = QStringLiteral("The agent '%1' is not approved").arg(QString::fromUtf8(agentId));

		return false;
	}

	// Only the folders of the versions added to the server are given.
	const QString cleanPath = GetCleanPath(path);
	if (!cleanPath.isEmpty()){
		const imtbase::ICollectionInfo::Ids versionIds = m_serviceVersionCollectionCompPtr->GetElementIds();
		for (const imtbase::ICollectionInfo::Id& versionId : versionIds){
			imtbase::IObjectCollection::DataPtr versionDataPtr;
			if (!m_serviceVersionCollectionCompPtr->GetObjectData(versionId, versionDataPtr)){
				continue;
			}

			const agentinodata::IServiceVersionInfo* versionInfoPtr = dynamic_cast<const agentinodata::IServiceVersionInfo*>(versionDataPtr.GetPtr());
			if (versionInfoPtr != nullptr && IsSamePath(GetCleanPath(versionInfoPtr->GetPath()), cleanPath)){
				folderPath = cleanPath;

				break;
			}
		}
	}

	if (folderPath.isEmpty()){
		errorMessage = QStringLiteral("The folder '%1' is not the folder of a service version").arg(QDir::toNativeSeparators(path));

		return false;
	}

	if (!QFileInfo(folderPath).isDir()){
		errorMessage = QStringLiteral("The folder '%1' of the service version does not exist").arg(QDir::toNativeSeparators(folderPath));

		return false;
	}

	return true;
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
