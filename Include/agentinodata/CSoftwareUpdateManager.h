// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/ISoftwareUpdateManager.h>

// Qt includes
#include <QMap>
#include <QByteArray>
#include <QString>


namespace agentinodata
{


/**
	Default implementation of the update manager.
	Handles communication with the file-repository server to download
	and install updates for agents and services.
	\ingroup Updates
*/
class CSoftwareUpdateManager: virtual public ISoftwareUpdateManager
{
public:
	CSoftwareUpdateManager();

	/**
		Set the base URL of the file-repository server.
	*/
	void SetRepositoryUrl(const QString& repositoryUrl);

	/**
		Get the base URL of the file-repository server.
	*/
	QString GetRepositoryUrl() const;

	// reimplemented (agentinodata::ISoftwareUpdateManager)
	virtual UpdateResult ApplyUpdate(const QByteArray& updateId, const QByteArray& agentId) override;
	virtual UpdateResult RollbackUpdate(const QByteArray& updateId, const QByteArray& agentId) override;

private:
	static bool IsValidId(const QByteArray& id);

	bool DownloadArtifact(const QString& sourceUrl, const QString& targetPath, QString& expectedChecksum, QString& errorMessage);
	bool InstallArtifact(const QString& artifactPath, const QByteArray& agentId, QString& errorMessage);
	bool VerifyChecksum(const QString& filePath, const QString& expectedChecksum) const;
	bool BackupCurrentVersion(const QByteArray& updateId, const QByteArray& agentId, QString& errorMessage);
	bool RestoreBackup(const QByteArray& updateId, const QByteArray& agentId, QString& errorMessage);

private:
	QString m_repositoryUrl;

	struct InstalledUpdateInfo
	{
		QString previousVersion;
		QString backupPath;
		QString installedPath;
	};

	QMap<QByteArray, InstalledUpdateInfo> m_installedUpdates;
};


} // namespace agentinodata


