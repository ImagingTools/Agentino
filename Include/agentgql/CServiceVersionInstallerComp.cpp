// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceVersionInstallerComp.h>
#include <GeneratedFiles/agentinosdl/SDL/1.0/CPP/Updates.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>

// ImtCore includes
#include <imtclientgql/TClientRequestManagerCompWrap.h>
#include <imtgql/CGqlContext.h>
#include <imtgql/CGqlFieldObject.h>

// Agentino includes
#include <agentinodata/CServiceInfo.h>
#include <agentinodata/IServiceInfo.h>


#undef StartService


namespace agentgql
{


// Temporary folder inside the folder of the service the archives are unpacked into.
static const QString s_unpackFolderName = QStringLiteral(".agentino-update");

// Suffix of a file being downloaded.
static const QString s_partialFileSuffix = QStringLiteral(".part");

// Suffix of a replaced file that could not be removed (kept open).
static const QString s_oldFileSuffix = QStringLiteral(".old");

// Interval of the checks whether a stopped service has ended.
static const int s_stopCheckInterval = 500;


// reimplemented (IServiceVersionInstaller)

bool CServiceVersionInstallerComp::StartServiceVersionUpdate(
			const QByteArray& serviceId,
			const QString& version,
			const QString& versionPath,
			QString& errorMessage)
{
	if (!m_gqlClientCompPtr.IsValid() || !m_serviceCollectionCompPtr.IsValid() || !m_serviceControllerCompPtr.IsValid() || !m_fileCompressionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Component references 'GqlClient', 'ServiceCollection', 'ServiceController' or 'FileCompression' were not set");

		return false;
	}

	if (serviceId.isEmpty() || version.trimmed().isEmpty() || versionPath.trimmed().isEmpty()){
		errorMessage = QStringLiteral("The service, the version or the path of the version is not set");

		return false;
	}

	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	const agentinodata::IServiceInfo* serviceInfoPtr = nullptr;
	if (m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		serviceInfoPtr = dynamic_cast<const agentinodata::IServiceInfo*>(serviceDataPtr.GetPtr());
	}

	if (serviceInfoPtr == nullptr){
		errorMessage = QStringLiteral("The service '%1' does not exist").arg(QString::fromUtf8(serviceId));

		return false;
	}

	// The archive is downloaded into the folder of the executable of the service ("Path" of the service).
	const QString servicePath = QString::fromUtf8(serviceInfoPtr->GetServicePath());
	if (servicePath.isEmpty()){
		errorMessage = QStringLiteral("The service '%1' has no path").arg(serviceInfoPtr->GetServiceName());

		return false;
	}

	const QFileInfo serviceFileInfo(servicePath);
	if (!serviceFileInfo.absoluteDir().exists()){
		errorMessage = QStringLiteral("The folder '%1' of the service '%2' does not exist")
					.arg(QDir::toNativeSeparators(serviceFileInfo.absolutePath()), serviceInfoPtr->GetServiceName());

		return false;
	}

	Update update;
	update.serviceId = serviceId;
	update.serviceName = serviceInfoPtr->GetServiceName().isEmpty() ? QString::fromUtf8(serviceId) : serviceInfoPtr->GetServiceName();
	update.version = version.trimmed();
	update.versionPath = versionPath.trimmed();
	update.serviceFolder = QDir::cleanPath(serviceFileInfo.absolutePath());
	update.executableName = serviceFileInfo.fileName();

	{
		QMutexLocker locker(&m_servicesMutex);
		if (m_updatedServices.contains(serviceId)){
			errorMessage = QStringLiteral("The service '%1' is being updated already").arg(update.serviceName);

			return false;
		}

		m_updatedServices.insert(serviceId);
	}

	// The requests to the server are sent over the WebSocket client on the thread of this component.
	const bool queued = QMetaObject::invokeMethod(
				this,
				[this, update](){
					m_pendingUpdates.append(update);

					StartNextUpdate();
				},
				Qt::QueuedConnection);
	if (!queued){
		QMutexLocker locker(&m_servicesMutex);
		m_updatedServices.remove(serviceId);

		errorMessage = QStringLiteral("Unable to queue the update of the service '%1'").arg(update.serviceName);

		return false;
	}

	SendInfoMessage(
				0,
				QStringLiteral("Update of the service '%1' to the version '%2' (%3) started")
							.arg(update.serviceName, update.version, update.versionPath),
				"CServiceVersionInstallerComp");

	return true;
}


// private methods

void CServiceVersionInstallerComp::StartNextUpdate()
{
	if (m_isUpdateRunning || m_pendingUpdates.isEmpty()){
		return;
	}

	m_isUpdateRunning = true;
	m_currentUpdate = m_pendingUpdates.takeFirst();

	QTimer::singleShot(0, this, [this](){ ExecuteDownload(); });
}


void CServiceVersionInstallerComp::ExecuteDownload()
{
	QString errorMessage;

	// 1. The archive of the version: the files of its folder on the server.
	QMap<QString, qint64> files;
	if (!GetVersionFiles(m_currentUpdate.versionPath, files, errorMessage)){
		FailUpdate(errorMessage, false);

		return;
	}

	QStringList archiveNames;
	for (QMap<QString, qint64>::const_iterator iter = files.constBegin(); iter != files.constEnd(); ++iter){
		if (iter.key().endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)){
			archiveNames << iter.key();
		}
	}

	if (archiveNames.isEmpty()){
		FailUpdate(QStringLiteral("The folder '%1' of the version on the server has no archive (*.zip)").arg(m_currentUpdate.versionPath), false);

		return;
	}

	// 2. Download into the folder of the executable of the service.
	const QDir serviceDir(m_currentUpdate.serviceFolder);
	for (const QString& archiveName : archiveNames){
		const QString archivePath = serviceDir.filePath(archiveName);
		if (!DownloadFile(m_currentUpdate.versionPath, archiveName, files.value(archiveName), archivePath, errorMessage)){
			FailUpdate(errorMessage, false);

			return;
		}

		m_currentUpdate.archivePaths << archivePath;

		SendInfoMessage(
					0,
					QStringLiteral("The archive '%1' of the version '%2' was downloaded into '%3'")
								.arg(archiveName, m_currentUpdate.version, QDir::toNativeSeparators(archivePath)),
					"CServiceVersionInstallerComp");
	}

	// 3. Stop the service.
	const QByteArray& serviceId = m_currentUpdate.serviceId;
	m_currentUpdate.wasRunning = !IsServiceStopped(serviceId);
	if (m_currentUpdate.wasRunning){
		if (!m_serviceControllerCompPtr->StopService(serviceId) && !IsServiceStopped(serviceId)){
			FailUpdate(QStringLiteral("Unable to stop the service"), false);

			return;
		}
	}

	m_currentUpdate.stopTimeout = (m_stopTimeoutAttrPtr.IsValid() && *m_stopTimeoutAttrPtr > 0) ? *m_stopTimeoutAttrPtr : 60;
	m_currentUpdate.stopDeadline = QDeadlineTimer(qint64(m_currentUpdate.stopTimeout) * 1000);

	CheckServiceStopped();
}


void CServiceVersionInstallerComp::CheckServiceStopped()
{
	if (IsServiceStopped(m_currentUpdate.serviceId)){
		ExecuteInstallation();

		return;
	}

	if (m_currentUpdate.stopDeadline.hasExpired()){
		FailUpdate(QStringLiteral("The service did not stop within %1 seconds").arg(m_currentUpdate.stopTimeout), true);

		return;
	}

	QTimer::singleShot(s_stopCheckInterval, this, [this](){ CheckServiceStopped(); });
}


void CServiceVersionInstallerComp::ExecuteInstallation()
{
	QString errorMessage;

	// 4. Unpack the archives into the folder of the service.
	for (const QString& archivePath : std::as_const(m_currentUpdate.archivePaths)){
		if (!UnpackArchive(archivePath, m_currentUpdate.serviceFolder, m_currentUpdate.executableName, errorMessage)){
			FailUpdate(errorMessage, m_currentUpdate.wasRunning);

			return;
		}
	}

	// 5. Start the service.
	const bool isStarted = m_serviceControllerCompPtr->StartService(m_currentUpdate.serviceId);
	if (!isStarted){
		SendErrorMessage(
					0,
					QStringLiteral("The service '%1' was updated to the version '%2', but it cannot be started")
								.arg(m_currentUpdate.serviceName, m_currentUpdate.version),
					"CServiceVersionInstallerComp");
	}

	// 6. Remove the archives.
	RemoveArchives();

	// The version is installed: the server counts the installation by the version of the service.
	if (!SetServiceVersion(m_currentUpdate.serviceId, m_currentUpdate.version)){
		SendWarningMessage(
					0,
					QStringLiteral("Unable to set the version '%1' of the service '%2'").arg(m_currentUpdate.version, m_currentUpdate.serviceName),
					"CServiceVersionInstallerComp");
	}

	if (isStarted){
		SendInfoMessage(
					0,
					QStringLiteral("The service '%1' was updated to the version '%2'").arg(m_currentUpdate.serviceName, m_currentUpdate.version),
					"CServiceVersionInstallerComp");
	}

	FinishUpdate();
}


void CServiceVersionInstallerComp::FailUpdate(const QString& errorMessage, bool restartService)
{
	SendErrorMessage(
				0,
				QStringLiteral("Unable to update the service '%1' to the version '%2'. Error: %3")
							.arg(m_currentUpdate.serviceName, m_currentUpdate.version, errorMessage),
				"CServiceVersionInstallerComp");

	RemoveArchives();

	// The service stopped for the update runs again.
	if (restartService && IsServiceStopped(m_currentUpdate.serviceId)){
		if (!m_serviceControllerCompPtr->StartService(m_currentUpdate.serviceId)){
			SendErrorMessage(
						0,
						QStringLiteral("Unable to start the service '%1' again").arg(m_currentUpdate.serviceName),
						"CServiceVersionInstallerComp");
		}
	}

	FinishUpdate();
}


void CServiceVersionInstallerComp::FinishUpdate()
{
	{
		QMutexLocker locker(&m_servicesMutex);
		m_updatedServices.remove(m_currentUpdate.serviceId);
	}

	m_currentUpdate = Update();
	m_isUpdateRunning = false;

	StartNextUpdate();
}


bool CServiceVersionInstallerComp::GetVersionFiles(const QString& versionPath, QMap<QString, qint64>& files, QString& errorMessage) const
{
	files.clear();

	sdl::V1_0::agentino::GetServiceVersionFilesRequestArguments arguments;
	arguments.input.emplace();
	arguments.input->path = versionPath;

	imtgql::CGqlRequest* gqlRequestPtr = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_QUERY);
	imtclientgql::IGqlClient::GqlRequestPtr requestPtr = CreateServerRequest(gqlRequestPtr);
	if (!sdl::V1_0::agentino::CGetServiceVersionFilesGqlRequest::SetupGqlRequest(*gqlRequestPtr, arguments)){
		errorMessage = QStringLiteral("Unable to create the request 'GetServiceVersionFiles'");

		return false;
	}

	gqlRequestPtr->AddSimpleField(QByteArrayLiteral("path"));

	imtgql::CGqlFieldObject fileFields;
	fileFields.InsertField(QByteArrayLiteral("name"));
	fileFields.InsertField(QByteArrayLiteral("size"));
	gqlRequestPtr->AddField(QByteArrayLiteral("files"), fileFields);

	const imtclientgql::IGqlClient::GqlModelResult<sdl::V1_0::agentino::CServiceVersionFileList> result =
				imtclientgql::CClientRequestModelHelpers::ParseModelResult<sdl::V1_0::agentino::CServiceVersionFileList>(
							m_gqlClientCompPtr->SendRequest(requestPtr),
							sdl::V1_0::agentino::CGetServiceVersionFilesGqlRequest::GetCommandId());
	if (!result){
		errorMessage = QStringLiteral("Unable to get the files of the version '%1' from the server: %2").arg(versionPath, result.errorMessage);

		return false;
	}

	if (result.model.files.has_value()){
		const QList<sdl::V1_0::agentino::CServiceVersionFile> fileList = result.model.files->ToList();
		for (const sdl::V1_0::agentino::CServiceVersionFile& file : fileList){
			if (file.name.has_value() && !file.name->isEmpty()){
				files.insert(*file.name, file.size.has_value() ? qint64(*file.size) : qint64(-1));
			}
		}
	}

	return true;
}


bool CServiceVersionInstallerComp::DownloadFile(
			const QString& versionPath,
			const QString& fileName,
			qint64 fileSize,
			const QString& filePath,
			QString& errorMessage) const
{
	// The file is written under another name until it is complete.
	const QString partialFilePath = filePath + s_partialFileSuffix;

	QFile::remove(partialFilePath);

	QFile file(partialFilePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
		errorMessage = QStringLiteral("Unable to create the file '%1': %2").arg(QDir::toNativeSeparators(partialFilePath), file.errorString());

		return false;
	}

	int chunkSize = (m_chunkSizeAttrPtr.IsValid() && *m_chunkSizeAttrPtr > 0) ? *m_chunkSizeAttrPtr : 512;
	chunkSize = qMin(chunkSize, 1024) * 1024;

	qint64 offset = 0;
	qint64 totalSize = fileSize;

	while (totalSize < 0 || offset < totalSize){
		sdl::V1_0::agentino::GetServiceVersionFileChunkRequestArguments arguments;
		arguments.input.emplace();
		arguments.input->path = versionPath;
		arguments.input->fileName = fileName;
		arguments.input->offset = offset;
		arguments.input->size = chunkSize;

		imtgql::CGqlRequest* gqlRequestPtr = new imtgql::CGqlRequest(imtgql::IGqlRequest::RT_QUERY);
		imtclientgql::IGqlClient::GqlRequestPtr requestPtr = CreateServerRequest(gqlRequestPtr);
		if (!sdl::V1_0::agentino::CGetServiceVersionFileChunkGqlRequest::SetupGqlRequest(*gqlRequestPtr, arguments)){
			errorMessage = QStringLiteral("Unable to create the request 'GetServiceVersionFileChunk'");

			break;
		}

		gqlRequestPtr->AddSimpleField(QByteArrayLiteral("fileName"));
		gqlRequestPtr->AddSimpleField(QByteArrayLiteral("offset"));
		gqlRequestPtr->AddSimpleField(QByteArrayLiteral("content"));
		gqlRequestPtr->AddSimpleField(QByteArrayLiteral("fileSize"));

		// SendRequest waits in a local event loop: the agent keeps working meanwhile.
		const imtclientgql::IGqlClient::GqlModelResult<sdl::V1_0::agentino::CServiceVersionFileChunk> result =
					imtclientgql::CClientRequestModelHelpers::ParseModelResult<sdl::V1_0::agentino::CServiceVersionFileChunk>(
								m_gqlClientCompPtr->SendRequest(requestPtr),
								sdl::V1_0::agentino::CGetServiceVersionFileChunkGqlRequest::GetCommandId());
		if (!result){
			errorMessage = QStringLiteral("Unable to download the file '%1' from the server: %2").arg(fileName, result.errorMessage);

			break;
		}

		if (result.model.fileSize.has_value()){
			totalSize = *result.model.fileSize;
		}

		if (!result.model.offset.has_value() || *result.model.offset != offset){
			errorMessage = QStringLiteral("Unable to download the file '%1' from the server: the server sent another part of it").arg(fileName);

			break;
		}

		const QByteArray data = QByteArray::fromBase64(result.model.content.has_value() ? result.model.content->toLatin1() : QByteArray());
		if (data.isEmpty()){
			// The end of the file
			break;
		}

		if (file.write(data) != data.size()){
			errorMessage = QStringLiteral("Unable to write the file '%1': %2").arg(QDir::toNativeSeparators(partialFilePath), file.errorString());

			break;
		}

		offset += data.size();
	}

	file.close();

	if (errorMessage.isEmpty() && totalSize >= 0 && offset != totalSize){
		errorMessage = QStringLiteral("Unable to download the file '%1' from the server: %2 of %3 bytes received").arg(fileName).arg(offset).arg(totalSize);
	}

	if (!errorMessage.isEmpty()){
		QFile::remove(partialFilePath);

		return false;
	}

	// A file of the same name left from a former update is replaced.
	if (QFileInfo::exists(filePath) && !QFile::remove(filePath)){
		errorMessage = QStringLiteral("Unable to replace the file '%1'").arg(QDir::toNativeSeparators(filePath));
		QFile::remove(partialFilePath);

		return false;
	}

	if (!QFile::rename(partialFilePath, filePath)){
		errorMessage = QStringLiteral("Unable to rename the file '%1' to '%2'").arg(QDir::toNativeSeparators(partialFilePath), QDir::toNativeSeparators(filePath));
		QFile::remove(partialFilePath);

		return false;
	}

	return true;
}


bool CServiceVersionInstallerComp::UnpackArchive(
			const QString& archivePath,
			const QString& serviceFolder,
			const QString& executableName,
			QString& errorMessage) const
{
	// The archive is unpacked into a temporary folder first: a broken archive leaves the service as it is.
	const QString unpackFolder = QDir(serviceFolder).filePath(s_unpackFolderName);

	QDir(unpackFolder).removeRecursively();
	if (!QDir().mkpath(unpackFolder)){
		errorMessage = QStringLiteral("Unable to create the folder '%1'").arg(QDir::toNativeSeparators(unpackFolder));

		return false;
	}

	if (!m_fileCompressionCompPtr->DecompressFolder(archivePath, unpackFolder)){
		QDir(unpackFolder).removeRecursively();

		errorMessage = QStringLiteral("Unable to unpack the archive '%1'").arg(QDir::toNativeSeparators(archivePath));

		return false;
	}

	// An archive holding a single folder with the executable of the service is unpacked from inside it.
	QString sourceFolder = unpackFolder;
	const QFileInfoList entries = QDir(unpackFolder).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
	if (entries.count() == 1 && entries.first().isDir() && !executableName.isEmpty() &&
				QFileInfo::exists(QDir(entries.first().absoluteFilePath()).filePath(executableName))){
		sourceFolder = entries.first().absoluteFilePath();
	}

	QStringList failedFiles;
	const bool isReplaced = ReplaceFiles(sourceFolder, serviceFolder, failedFiles);

	QDir(unpackFolder).removeRecursively();

	if (!isReplaced){
		errorMessage = QStringLiteral("Unable to replace the files of the service: %1").arg(failedFiles.join(QStringLiteral(", ")));

		return false;
	}

	return true;
}


bool CServiceVersionInstallerComp::ReplaceFiles(const QString& sourceFolder, const QString& targetFolder, QStringList& failedFiles) const
{
	const QDir sourceDir(sourceFolder);
	const QDir targetDir(targetFolder);

	QDirIterator iterator(sourceFolder, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (iterator.hasNext()){
		const QString sourceFilePath = iterator.next();
		const QString relativePath = sourceDir.relativeFilePath(sourceFilePath);
		const QString targetFilePath = targetDir.filePath(relativePath);

		if (!QDir().mkpath(QFileInfo(targetFilePath).absolutePath()) || !ReplaceFile(sourceFilePath, targetFilePath)){
			failedFiles << QDir::toNativeSeparators(relativePath);
		}
	}

	return failedFiles.isEmpty();
}


bool CServiceVersionInstallerComp::ReplaceFile(const QString& sourceFilePath, const QString& targetFilePath) const
{
	if (QFileInfo::exists(targetFilePath)){
		QFile targetFile(targetFilePath);
		targetFile.setPermissions(targetFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);

		if (!targetFile.remove()){
			// A file kept open (e.g. a plugin of the service loaded by the agent) can be renamed but not removed.
			const QString oldFilePath = targetFilePath + s_oldFileSuffix;
			QFile::remove(oldFilePath);

			if (!QFile::rename(targetFilePath, oldFilePath)){
				return false;
			}

			SendWarningMessage(
						0,
						QStringLiteral("The file '%1' is in use: it was renamed to '%2'")
									.arg(QDir::toNativeSeparators(targetFilePath), QDir::toNativeSeparators(oldFilePath)),
						"CServiceVersionInstallerComp");
		}
	}

	if (QFile::rename(sourceFilePath, targetFilePath)){
		return true;
	}

	return QFile::copy(sourceFilePath, targetFilePath);
}


bool CServiceVersionInstallerComp::SetServiceVersion(const QByteArray& serviceId, const QString& version) const
{
	imtbase::IObjectCollection::DataPtr serviceDataPtr;
	if (!m_serviceCollectionCompPtr->GetObjectData(serviceId, serviceDataPtr)){
		return false;
	}

	agentinodata::CServiceInfo* serviceInfoPtr = dynamic_cast<agentinodata::CServiceInfo*>(serviceDataPtr.GetPtr());
	if (serviceInfoPtr == nullptr){
		return false;
	}

	if (serviceInfoPtr->GetServiceVersion() == version){
		return true;
	}

	serviceInfoPtr->SetServiceVersion(version);

	// The change is stored and reported to the server (CAgentServiceCollectionSyncClientComp).
	return m_serviceCollectionCompPtr->SetObjectData(serviceId, *serviceInfoPtr);
}


imtclientgql::IGqlClient::GqlRequestPtr CServiceVersionInstallerComp::CreateServerRequest(imtgql::CGqlRequest* gqlRequestPtr) const
{
	// The server gives the files of the versions to the approved agents: the request names the agent.
	if (m_clientIdCompPtr.IsValid()){
		const QByteArray clientId = m_clientIdCompPtr->GetText().toUtf8();
		if (!clientId.isEmpty()){
			imtgql::CGqlContext* gqlContextPtr = new imtgql::CGqlContext();
			imtgql::IGqlContext::Headers headers;
			headers.insert(QByteArrayLiteral("clientid"), clientId);
			gqlContextPtr->SetHeaders(headers);
			gqlRequestPtr->SetGqlContext(gqlContextPtr);
		}
	}

	return imtclientgql::IGqlClient::GqlRequestPtr(gqlRequestPtr);
}


void CServiceVersionInstallerComp::RemoveArchives()
{
	for (const QString& archivePath : std::as_const(m_currentUpdate.archivePaths)){
		if (QFileInfo::exists(archivePath) && !QFile::remove(archivePath)){
			SendWarningMessage(
						0,
						QStringLiteral("Unable to remove the archive '%1'").arg(QDir::toNativeSeparators(archivePath)),
						"CServiceVersionInstallerComp");
		}
	}

	m_currentUpdate.archivePaths.clear();
}


bool CServiceVersionInstallerComp::IsServiceStopped(const QByteArray& serviceId) const
{
	const agentinodata::IServiceStatusInfo::ServiceStatus status = m_serviceControllerCompPtr->GetServiceStatus(serviceId);

	return status != agentinodata::IServiceStatusInfo::SS_RUNNING &&
				status != agentinodata::IServiceStatusInfo::SS_STARTING &&
				status != agentinodata::IServiceStatusInfo::SS_STOPPING;
}


} // namespace agentgql


