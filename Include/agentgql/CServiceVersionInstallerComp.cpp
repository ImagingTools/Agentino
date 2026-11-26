// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentgql/CServiceVersionInstallerComp.h>


// Qt includes
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#if QT_CONFIG(ssl)
#include <QtNetwork/QSslError>
#endif

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

// Suffix of the archives of a version.
static const QString s_archiveSuffix = QStringLiteral(".zip");

// Interval of the checks whether a stopped service has ended.
static const int s_stopCheckInterval = 500;


// reimplemented (IServiceVersionInstaller)

bool CServiceVersionInstallerComp::StartServiceVersionUpdate(
			const QByteArray& serviceId,
			const QString& version,
			const QStringList& archiveUrls,
			QString& errorMessage)
{
	if (!m_serviceCollectionCompPtr.IsValid() || !m_serviceControllerCompPtr.IsValid() || !m_fileCompressionCompPtr.IsValid()){
		errorMessage = QStringLiteral("Component references 'ServiceCollection', 'ServiceController' or 'FileCompression' were not set");

		return false;
	}

	if (serviceId.isEmpty() || version.trimmed().isEmpty()){
		errorMessage = QStringLiteral("The service or the version is not set");

		return false;
	}

	if (archiveUrls.isEmpty()){
		errorMessage = QStringLiteral("The URLs of the archives of the version are not set");

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

	// The archives are downloaded into the folder of the executable of the service ("Path" of the service).
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
	update.serviceFolder = QDir::cleanPath(serviceFileInfo.absolutePath());
	update.executableName = serviceFileInfo.fileName();

	const QDir serviceDir(update.serviceFolder);
	QSet<QString> fileNames;
	QStringList urlTexts;
	for (const QString& urlText : archiveUrls){
		Archive archive;
		if (!ResolveArchiveUrl(urlText, archive.url, archive.fileName, errorMessage)){
			return false;
		}

		// Two archives of the same name would be downloaded into the same file.
		const QString fileNameKey = archive.fileName.toLower();
		if (fileNames.contains(fileNameKey)){
			errorMessage = QStringLiteral("The archive '%1' is given twice").arg(archive.fileName);

			return false;
		}

		fileNames.insert(fileNameKey);

		archive.filePath = serviceDir.filePath(archive.fileName);
		update.archives << archive;

		urlTexts << archive.url.toDisplayString();
	}

	{
		QMutexLocker locker(&m_servicesMutex);
		if (m_updatedServices.contains(serviceId)){
			errorMessage = QStringLiteral("The service '%1' is being updated already").arg(update.serviceName);

			return false;
		}

		m_updatedServices.insert(serviceId);
	}

	// The updates run on the thread of this component.
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
				QStringLiteral("Update of the service '%1' to the version '%2' started, archives: %3")
							.arg(update.serviceName, update.version, urlTexts.join(QStringLiteral(", "))),
				"CServiceVersionInstallerComp");

	return true;
}


// protected methods

// reimplemented (icomp::CComponentBase)

void CServiceVersionInstallerComp::OnComponentDestroyed()
{
	// An unfinished download leaves no file behind.
	AbortDownload();

	BaseClass::OnComponentDestroyed();
}


// private methods

void CServiceVersionInstallerComp::StartNextUpdate()
{
	if (m_isUpdateRunning || m_pendingUpdates.isEmpty()){
		return;
	}

	m_isUpdateRunning = true;
	m_currentUpdate = m_pendingUpdates.takeFirst();
	m_currentUpdate.downloadIndex = 0;

	QTimer::singleShot(0, this, [this](){ DownloadNextArchive(); });
}


void CServiceVersionInstallerComp::DownloadNextArchive()
{
	// 1. Download the archives into the folder of the executable of the service, one after another.
	if (m_currentUpdate.downloadIndex >= m_currentUpdate.archives.count()){
		StopService();

		return;
	}

	const Archive& archive = m_currentUpdate.archives[m_currentUpdate.downloadIndex];

	// The archive is written under another name until it is complete.
	const QString partialFilePath = archive.filePath + s_partialFileSuffix;
	QFile::remove(partialFilePath);

	m_downloadError.clear();
	m_downloadFile.setFileName(partialFilePath);
	if (!m_downloadFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
		FailUpdate(QStringLiteral("Unable to create the file '%1': %2").arg(QDir::toNativeSeparators(partialFilePath), m_downloadFile.errorString()), false);

		return;
	}

	if (m_networkManagerPtr == nullptr){
		m_networkManagerPtr = new QNetworkAccessManager(this);
	}

	QNetworkRequest request(archive.url);

	// The server gives the archives of the versions to the approved agents: the request names the agent.
	if (m_clientIdCompPtr.IsValid()){
		const QByteArray clientId = m_clientIdCompPtr->GetText().toUtf8();
		if (!clientId.isEmpty()){
			request.setRawHeader(QByteArrayLiteral("clientid"), clientId);
		}
	}

	// The archive is compressed already: it is sent as it is, its length is checked.
	request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("identity"));

	const int downloadTimeout = (m_downloadTimeoutAttrPtr.IsValid() && *m_downloadTimeoutAttrPtr > 0) ? *m_downloadTimeoutAttrPtr : 60;
	request.setTransferTimeout(downloadTimeout * 1000);

	QNetworkReply* replyPtr = m_networkManagerPtr->get(request);
	m_downloadReplyPtr = replyPtr;

	connect(replyPtr, &QNetworkReply::readyRead, this, &CServiceVersionInstallerComp::OnDownloadDataReceived);
	connect(replyPtr, &QNetworkReply::finished, this, &CServiceVersionInstallerComp::OnDownloadFinished);

#if QT_CONFIG(ssl)
	// The certificate of the server is not checked, as for the WebSocket connection of the agent (CWebSocketClientComp).
	connect(replyPtr, &QNetworkReply::sslErrors, this, [this, replyPtr](const QList<QSslError>& sslErrors){
		for (const QSslError& sslError : sslErrors){
			SendWarningMessage(0, QStringLiteral("SSL error of the download '%1': %2").arg(replyPtr->url().toDisplayString(), sslError.errorString()), "CServiceVersionInstallerComp");
		}

		replyPtr->ignoreSslErrors(sslErrors);
	});
#endif
}


void CServiceVersionInstallerComp::OnDownloadDataReceived()
{
	QNetworkReply* replyPtr = m_downloadReplyPtr.data();
	if (replyPtr == nullptr || !m_downloadError.isEmpty()){
		return;
	}

	// The body of an error response is not the archive.
	const QVariant statusCode = replyPtr->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (statusCode.isValid() && statusCode.toInt() != 200){
		return;
	}

	const QByteArray data = replyPtr->readAll();
	if (!data.isEmpty() && m_downloadFile.write(data) != data.size()){
		m_downloadError = QStringLiteral("Unable to write the file '%1': %2")
					.arg(QDir::toNativeSeparators(m_downloadFile.fileName()), m_downloadFile.errorString());

		// OnDownloadFinished reports the error.
		replyPtr->abort();
	}
}


void CServiceVersionInstallerComp::OnDownloadFinished()
{
	QNetworkReply* replyPtr = m_downloadReplyPtr.data();
	if (replyPtr == nullptr){
		return;
	}

	m_downloadReplyPtr.clear();
	replyPtr->deleteLater();

	const int statusCode = replyPtr->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	// The rest of the data not read by OnDownloadDataReceived.
	if (m_downloadError.isEmpty() && replyPtr->error() == QNetworkReply::NoError && statusCode == 200 && replyPtr->bytesAvailable() > 0){
		const QByteArray data = replyPtr->readAll();
		if (m_downloadFile.write(data) != data.size()){
			m_downloadError = QStringLiteral("Unable to write the file '%1': %2")
						.arg(QDir::toNativeSeparators(m_downloadFile.fileName()), m_downloadFile.errorString());
		}
	}

	const QString partialFilePath = m_downloadFile.fileName();
	m_downloadFile.close();

	const qint64 receivedSize = QFileInfo(partialFilePath).size();

	const Archive archive = m_currentUpdate.archives.value(m_currentUpdate.downloadIndex);

	QString errorMessage = m_downloadError;
	if (errorMessage.isEmpty() && replyPtr->error() != QNetworkReply::NoError){
		errorMessage = (replyPtr->error() == QNetworkReply::OperationCanceledError)
					? QStringLiteral("no data was received within %1 seconds").arg((m_downloadTimeoutAttrPtr.IsValid() && *m_downloadTimeoutAttrPtr > 0) ? *m_downloadTimeoutAttrPtr : 60)
					: replyPtr->errorString();
	}

	if (errorMessage.isEmpty() && statusCode != 200){
		errorMessage = QStringLiteral("the server answered %1 %2")
					.arg(statusCode)
					.arg(replyPtr->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
	}

	if (errorMessage.isEmpty()){
		const QVariant contentLength = replyPtr->header(QNetworkRequest::ContentLengthHeader);
		if (contentLength.isValid() && contentLength.toLongLong() != receivedSize){
			errorMessage = QStringLiteral("%1 of %2 bytes received").arg(receivedSize).arg(contentLength.toLongLong());
		}
	}

	if (!errorMessage.isEmpty()){
		QFile::remove(partialFilePath);

		FailUpdate(QStringLiteral("Unable to download the archive '%1': %2").arg(archive.url.toDisplayString(), errorMessage), false);

		return;
	}

	// An archive of the same name left from a former update is replaced.
	if (QFileInfo::exists(archive.filePath) && !QFile::remove(archive.filePath)){
		QFile::remove(partialFilePath);

		FailUpdate(QStringLiteral("Unable to replace the file '%1'").arg(QDir::toNativeSeparators(archive.filePath)), false);

		return;
	}

	if (!QFile::rename(partialFilePath, archive.filePath)){
		QFile::remove(partialFilePath);

		FailUpdate(QStringLiteral("Unable to rename the file '%1' to '%2'")
					.arg(QDir::toNativeSeparators(partialFilePath), QDir::toNativeSeparators(archive.filePath)), false);

		return;
	}

	m_currentUpdate.archivePaths << archive.filePath;

	SendInfoMessage(
				0,
				QStringLiteral("The archive '%1' of the version '%2' was downloaded into '%3' (%4 bytes)")
							.arg(archive.url.toDisplayString(), m_currentUpdate.version, QDir::toNativeSeparators(archive.filePath))
							.arg(receivedSize),
				"CServiceVersionInstallerComp");

	m_currentUpdate.downloadIndex++;

	QTimer::singleShot(0, this, [this](){ DownloadNextArchive(); });
}


void CServiceVersionInstallerComp::StopService()
{
	// 2. Stop the service.
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

	// 3. Unpack the archives into the folder of the service.
	for (const QString& archivePath : std::as_const(m_currentUpdate.archivePaths)){
		if (!UnpackArchive(archivePath, m_currentUpdate.serviceFolder, m_currentUpdate.executableName, errorMessage)){
			FailUpdate(errorMessage, m_currentUpdate.wasRunning);

			return;
		}
	}

	// 4. Start the service.
	const bool isStarted = m_serviceControllerCompPtr->StartService(m_currentUpdate.serviceId);
	if (!isStarted){
		SendErrorMessage(
					0,
					QStringLiteral("The service '%1' was updated to the version '%2', but it cannot be started")
								.arg(m_currentUpdate.serviceName, m_currentUpdate.version),
					"CServiceVersionInstallerComp");
	}

	// 5. Remove the archives.
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

	AbortDownload();
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


bool CServiceVersionInstallerComp::ResolveArchiveUrl(const QString& urlText, QUrl& url, QString& fileName, QString& errorMessage) const
{
	url = QUrl(urlText.trimmed());
	if (urlText.trimmed().isEmpty() || !url.isValid()){
		errorMessage = QStringLiteral("The URL '%1' of an archive is invalid").arg(urlText);

		return false;
	}

	// The HTTP address of the server the agent is connected to.
	QUrl serverUrl;
	if (m_serverConnectionCompPtr.IsValid()){
		if (!m_serverConnectionCompPtr->GetUrl(imtcom::IServerConnectionInterface::PT_HTTP, serverUrl) || serverUrl.host().isEmpty()){
			serverUrl = QUrl();
		}
	}

	if (url.host().isEmpty()){
		// The path of a URL only: the archive is on the server the agent is connected to.
		if (serverUrl.isEmpty()){
			errorMessage = QStringLiteral("The URL '%1' of an archive has no host, and the address of the server is not known").arg(urlText);

			return false;
		}

		QUrl resolvedUrl = serverUrl;
		resolvedUrl.setPath(url.path());
		resolvedUrl.setQuery(url.query());
		url = resolvedUrl;
	}
	else if (IsLocalHost(url.host()) && !serverUrl.isEmpty() && !IsLocalHost(serverUrl.host())){
		// The server knows itself by a local address: it is reached at the address the agent is connected to.
		url.setHost(serverUrl.host());
	}

	const QString scheme = url.scheme().toLower();
	if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")){
		errorMessage = QStringLiteral("The URL '%1' of an archive is not an HTTP URL").arg(urlText);

		return false;
	}

	fileName = url.fileName();
	if (!IsArchiveFileName(fileName)){
		errorMessage = QStringLiteral("The URL '%1' is not the URL of an archive (*.zip)").arg(urlText);

		return false;
	}

	return true;
}


void CServiceVersionInstallerComp::AbortDownload()
{
	QNetworkReply* replyPtr = m_downloadReplyPtr.data();
	m_downloadReplyPtr.clear();

	if (replyPtr != nullptr){
		// No more signals of the download: it is ended here.
		replyPtr->disconnect(this);
		replyPtr->abort();
		replyPtr->deleteLater();
	}

	if (m_downloadFile.isOpen()){
		const QString partialFilePath = m_downloadFile.fileName();
		m_downloadFile.close();

		QFile::remove(partialFilePath);
	}
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


// private static methods

bool CServiceVersionInstallerComp::IsArchiveFileName(const QString& fileName)
{
	if (fileName.length() <= s_archiveSuffix.length() || !fileName.endsWith(s_archiveSuffix, Qt::CaseInsensitive)){
		return false;
	}

	// A name of a file of the folder of the service: no path, no drive.
	return !fileName.contains(QLatin1Char('/')) && !fileName.contains(QLatin1Char('\\')) && !fileName.contains(QLatin1Char(':'));
}


bool CServiceVersionInstallerComp::IsLocalHost(const QString& host)
{
	if (host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0){
		return true;
	}

	const QHostAddress address(host);
	if (address.isNull()){
		return false;
	}

	return address.isLoopback() ||
				address == QHostAddress(QHostAddress::AnyIPv4) ||
				address == QHostAddress(QHostAddress::AnyIPv6);
}


} // namespace agentgql


