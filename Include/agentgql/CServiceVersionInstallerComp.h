// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDeadlineTimer>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>

// ACF includes
#include <ilog/TLoggerCompWrap.h>
#include <iprm/ITextParam.h>

// ImtCore includes
#include <imtbase/IObjectCollection.h>
#include <imtclientgql/IGqlClient.h>
#include <imtfile/IFileCompression.h>
#include <imtgql/CGqlRequest.h>

// Agentino includes
#include <agentinodata/IServiceController.h>
#include <agentgql/IServiceVersionInstaller.h>


namespace agentgql
{


/**
	Update of the services of the agent to the versions uploaded to the server ("Install" of the versions
	page of the server, UpdateServiceVersion of Services.sdl, see CServiceControllerComp).

	The server sends the path of the folder of the version on the server. The installer then:
	1. asks the server for the files of the folder (GetServiceVersionFiles of Updates.sdl) and downloads the
	   archives (*.zip) part by part (GetServiceVersionFileChunk) into the folder of the executable of the
	   service (the path of the service, IServiceInfo::GetServicePath);
	2. stops the service and waits until its process has ended;
	3. unpacks the archives there: into a temporary folder first, whose files then replace the files of the
	   service (a file kept open, e.g. a plugin loaded by the agent, is renamed to "<name>.old" first);
	   an archive holding a single folder is unpacked from inside that folder;
	4. starts the service;
	5. removes the archives and sets the version of the service (IServiceInfo::GetServiceVersion), so the
	   server counts the installation of the version.
	The result is reported to the log of the agent. A failed update removes the downloaded files and starts
	the service again if it was running.

	The requests to the server are sent over the WebSocket client of the agent, which must be used on its
	own thread: the updates run on the thread of this component (the main thread), one after another.
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
		I_ASSIGN(m_gqlClientCompPtr, "GqlClient", "WebSocket client connected to the server: the files of the version are requested with it", true, "GqlClient");
		I_ASSIGN(m_clientIdCompPtr, "ClientIdParam", "Agent client-ID (sent as header clientid)", true, "ClientIdParam");
		I_ASSIGN(m_serviceCollectionCompPtr, "ServiceCollection", "Services of the agent", true, "ServiceCollection");
		I_ASSIGN(m_serviceControllerCompPtr, "ServiceController", "Stops and starts the services", true, "ServiceController");
		I_ASSIGN(m_fileCompressionCompPtr, "FileCompression", "Unpacks the archives of the versions (zip)", true, "FileCompression");
		I_ASSIGN(m_stopTimeoutAttrPtr, "StopTimeout", "Time in seconds to wait for a service to stop", false, 60);
		I_ASSIGN(m_chunkSizeAttrPtr, "ChunkSize", "Size in KB of a part of a file requested from the server at once (at most 1024)", false, 512);
	I_END_COMPONENT;

	// reimplemented (IServiceVersionInstaller)
	virtual bool StartServiceVersionUpdate(
				const QByteArray& serviceId,
				const QString& version,
				const QString& versionPath,
				QString& errorMessage) override;

private:
	/**
		An update of a service.
	*/
	struct Update
	{
		QByteArray serviceId;
		QString serviceName;
		QString version;
		// Path of the folder of the version on the server
		QString versionPath;
		// Folder of the executable of the service
		QString serviceFolder;
		// File name of the executable of the service
		QString executableName;
		// The downloaded archives (in the folder of the service)
		QStringList archivePaths;
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
		Download the archives of the version and stop the service.
	*/
	void ExecuteDownload();

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
		Get the names of the files of the folder of the version on the server.
	*/
	bool GetVersionFiles(const QString& versionPath, QMap<QString, qint64>& files, QString& errorMessage) const;

	/**
		Download a file of the folder of the version on the server into the local file.
	*/
	bool DownloadFile(const QString& versionPath, const QString& fileName, qint64 fileSize, const QString& filePath, QString& errorMessage) const;

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
		Create a request to the server, addressed with the client-ID of the agent.
	*/
	imtclientgql::IGqlClient::GqlRequestPtr CreateServerRequest(imtgql::CGqlRequest* gqlRequestPtr) const;

	/**
		Remove the downloaded archives of the current update.
	*/
	void RemoveArchives();

	bool IsServiceStopped(const QByteArray& serviceId) const;

private:
	I_REF(imtclientgql::IGqlClient, m_gqlClientCompPtr);
	I_REF(iprm::ITextParam, m_clientIdCompPtr);
	I_REF(imtbase::IObjectCollection, m_serviceCollectionCompPtr);
	I_REF(agentinodata::IServiceController, m_serviceControllerCompPtr);
	I_REF(imtfile::IFileCompression, m_fileCompressionCompPtr);
	I_ATTR(int, m_stopTimeoutAttrPtr);
	I_ATTR(int, m_chunkSizeAttrPtr);

	// The updates waiting for their turn and the running one (touched on the thread of the component only)
	QList<Update> m_pendingUpdates;
	Update m_currentUpdate;
	bool m_isUpdateRunning = false;

	// The services being updated or waiting for it (StartServiceVersionUpdate is called on the GQL worker threads)
	mutable QMutex m_servicesMutex;
	QSet<QByteArray> m_updatedServices;
};


} // namespace agentgql


