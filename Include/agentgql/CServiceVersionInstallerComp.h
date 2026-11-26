// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDeadlineTimer>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtcom/IServerConnectionInterface.h>
#include <imtfile/IFileCompression.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentgql/IServiceVersionInstaller.h>


class QNetworkAccessManager;


namespace agentgql
{


/**
	Update of the services of the agent to the versions uploaded to the server ("Install" of the versions
	page of the server, UpdateServiceVersion of Services.sdl, see CServiceControllerComp).

	The server sends the URLs of the archives (*.zip) of the folder of the version. The installer then:
	1. downloads the archives over HTTP (GET with the header "clientid" of the agent: the server gives
	   them to the approved agents only) into the folder of the executable of the service (the path of
	   the service, IServiceInfo::GetServicePath), one after another; an archive is written as
	   "<name>.part" until it is complete;
	2. stops the service and waits until its process has ended;
	3. unpacks the archives there: into a temporary folder first, whose files then replace the files of the
	   service (a file kept open, e.g. a plugin loaded by the agent, is renamed to "<name>.old" first);
	   an archive holding a single folder is unpacked from inside that folder;
	4. starts the service;
	5. removes the archives and sets the version of the service (IServiceInfo::GetServiceVersion), so the
	   server counts the installation of the version.
	The result is reported to the log of the agent. A failed update removes the downloaded files and starts
	the service again if it was running.

	A URL without a host (path only) is resolved against the HTTP address of the server the agent is
	connected to (ServerConnection); a URL whose host is a loopback or unspecified address (the server
	knows itself as "localhost", for example) gets the host of that address as well.

	The updates run on the thread of this component (the main thread), one after another; the downloads
	are asynchronous.
*/
class CServiceVersionInstallerComp:
			public QObject,
			public ilog::CLoggerComponentBase,
			virtual public IServiceVersionInstaller
{
	Q_OBJECT
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceVersionInstallerComp);
		I_REGISTER_INTERFACE(IServiceVersionInstaller);
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Agent client-ID (sent as header clientid of the downloads)", true, "ClientIdParam");
		I_ASSIGN(m_serverConnectionCompPtr, "ServerConnection", "Addresses of the server: a URL of an archive without a host is resolved against its HTTP address", false, "ServerConnection");
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Services of the agent", true, "ServiceCollection");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Stops and starts the services", true, "ServiceController");
		I_ASSIGN(m_fileCompressionCompPtr, "FileCompression", "Unpacks the archives of the versions (zip)", true, "FileCompression");
		I_ASSIGN(m_stopTimeoutAttrPtr, "StopTimeout", "Time in seconds to wait for a service to stop", false, 60);
		I_ASSIGN(m_downloadTimeoutAttrPtr, "DownloadTimeout", "Time in seconds a download may receive no data before it fails", false, 60);
	I_END_COMPONENT;

	// reimplemented (IServiceVersionInstaller)
	virtual bool StartServiceVersionUpdate(
				const QByteArray& serviceId,
				const QString& version,
				const QStringList& archiveUrls,
				QString& errorMessage) override;

protected:
	// reimplemented (icomp::CComponentBase)
	virtual void OnComponentDestroyed() override;

private:
	/**
		An archive of a version: where it is downloaded from and to.
	*/
	struct Archive
	{
		QUrl url;
		QString fileName;
		// The archive in the folder of the service
		QString filePath;
	};

	/**
		An update of a service.
	*/
	struct Update
	{
		QByteArray serviceId;
		QString serviceName;
		QString version;
		QList<Archive> archives;
		// Folder of the executable of the service
		QString serviceFolder;
		// File name of the executable of the service
		QString executableName;
		// The archives downloaded so far (in the folder of the service)
		QStringList archivePaths;
		// The archive being downloaded (index in archives)
		int downloadIndex = 0;
		// The service was running before the update
		bool wasRunning = false;
		// Time in seconds to wait for the service to stop
		int stopTimeout = 60;
		QDeadlineTimer stopDeadline;
	};

	/**
		Start the next update of the queue unless one is running.
	*/
	void StartNextUpdate();

	/**
		Download the next archive of the current update; when all of them are downloaded, stop the service.
	*/
	void DownloadNextArchive();

	/**
		Write the received data of the download into the file.
	*/
	void OnDownloadDataReceived();

	/**
		End the download of the archive: check it, rename the file and download the next archive.
	*/
	void OnDownloadFinished();

	/**
		Stop the service, then wait for it to stop.
	*/
	void StopService();

	/**
		Wait for the service to stop, then unpack the archives and start the service.
	*/
	void CheckServiceStopped();

	/**
		Unpack the archives, start the service and remove the archives.
	*/
	void ExecuteInstallation();

	/**
		End the current update with an error: the downloaded files are removed, the service is started
		again if it had been stopped by the update.
	*/
	void FailUpdate(const QString& errorMessage, bool restartService);

	/**
		End the current update and start the next one.
	*/
	void FinishUpdate();

	/**
		Get the URL of an archive sent by the server (see the class description) and the name of its file.
	*/
	bool ResolveArchiveUrl(const QString& urlText, QUrl& url, QString& fileName, QString& errorMessage) const;

	/**
		Stop the running download and remove its unfinished file.
	*/
	void AbortDownload();

	/**
		Unpack the archive into the folder of the service.
		\param executableName	file name of the executable of the service: an archive holding a single folder
								with it is unpacked from inside that folder.
	*/
	bool UnpackArchive(const QString& archivePath, const QString& serviceFolder, const QString& executableName, QString& errorMessage) const;

	/**
		Replace the files of the target folder by the files of the source folder (recursively).
	*/
	bool ReplaceFiles(const QString& sourceFolder, const QString& targetFolder, QStringList& failedFiles) const;

	/**
		Replace the file: a file that cannot be removed (kept open) is renamed to "<name>.old" first.
	*/
	bool ReplaceFile(const QString& sourceFilePath, const QString& targetFilePath) const;

	/**
		Set the version of the service in the service collection.
	*/
	bool SetServiceVersion(const QByteArray& serviceId, const QString& version) const;

	/**
		Remove the downloaded archives of the current update.
	*/
	void RemoveArchives();

	bool IsServiceStopped(const QByteArray& serviceId) const;

	/**
		Whether the name is the name of an archive (*.zip) without a path.
	*/
	static bool IsArchiveFileName(const QString& fileName);

	/**
		Whether the host is a loopback or unspecified address ("localhost", "127.0.0.1", "0.0.0.0", "::" ...).
	*/
	static bool IsLocalHost(const QString& host);

private:
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
	I_REF(imtcom::IServerConnectionInterface, m_serverConnectionCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtfile::IFileCompression, m_fileCompressionCompPtr);
	I_ATTR(int, m_stopTimeoutAttrPtr);
	I_ATTR(int, m_downloadTimeoutAttrPtr);

	// The updates waiting for their turn and the running one (touched on the thread of the component only)
	QList<Update> m_pendingUpdates;
	Update m_currentUpdate;
	bool m_isUpdateRunning = false;

	// The running download (on the thread of the component)
	QNetworkAccessManager* m_networkManagerPtr = nullptr;
	QPointer<QNetworkReply> m_downloadReplyPtr;
	QFile m_downloadFile;
	QString m_downloadError;

	// The services being updated or waiting for it (StartServiceVersionUpdate is called on the GQL worker threads)
	mutable QMutex m_servicesMutex;
	QSet<QByteArray> m_updatedServices;
};


} // namespace agentgql


