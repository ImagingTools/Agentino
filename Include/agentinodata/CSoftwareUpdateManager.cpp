// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include "agentinodata/CSoftwareUpdateManager.h"


// Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDebug>


namespace agentinodata
{


// public methods

CSoftwareUpdateManager::CSoftwareUpdateManager()
{
}


void CSoftwareUpdateManager::SetRepositoryUrl(const QString& repositoryUrl)
{
	m_repositoryUrl = repositoryUrl;
}


QString CSoftwareUpdateManager::GetRepositoryUrl() const
{
	return m_repositoryUrl;
}


// reimplemented (agentinodata::ISoftwareUpdateManager)

ISoftwareUpdateManager::UpdateResult CSoftwareUpdateManager::ApplyUpdate(const QByteArray& updateId, const QByteArray& agentId)
{
	UpdateResult result;

	if (updateId.isEmpty()){
		result.successful = false;
		result.errorMessage = "Update ID is empty";
		return result;
	}

	if (agentId.isEmpty()){
		result.successful = false;
		result.errorMessage = "Agent ID is empty";
		return result;
	}

	// Backup current version before applying
	QString errorMessage;
	if (!BackupCurrentVersion(updateId, agentId, errorMessage)){
		result.successful = false;
		result.status = static_cast<int>(ISoftwareUpdateInfo::US_FAILED);
		result.errorMessage = QString("Failed to create backup: %1").arg(errorMessage);
		return result;
	}

	// Download artifact from file-repository server
	QString targetPath = QDir::tempPath() + "/" + QString::fromUtf8(updateId) + ".artifact";
	QString sourceUrl = m_repositoryUrl + "/artifacts/" + QString::fromUtf8(updateId);

	if (!DownloadArtifact(sourceUrl, targetPath, errorMessage)){
		result.successful = false;
		result.status = static_cast<int>(ISoftwareUpdateInfo::US_FAILED);
		result.errorMessage = QString("Download failed: %1").arg(errorMessage);
		return result;
	}

	// Install the artifact
	if (!InstallArtifact(targetPath, agentId, errorMessage)){
		result.successful = false;
		result.status = static_cast<int>(ISoftwareUpdateInfo::US_FAILED);
		result.errorMessage = QString("Installation failed: %1").arg(errorMessage);

		// Attempt to restore backup on failure
		QString restoreError;
		RestoreBackup(updateId, agentId, restoreError);
		return result;
	}

	// Record the installed update
	InstalledUpdateInfo info;
	info.installedPath = targetPath;
	m_installedUpdates[updateId] = info;

	result.successful = true;
	result.status = static_cast<int>(ISoftwareUpdateInfo::US_INSTALLED);

	// Clean up downloaded artifact
	QFile::remove(targetPath);

	return result;
}


ISoftwareUpdateManager::UpdateResult CSoftwareUpdateManager::RollbackUpdate(const QByteArray& updateId, const QByteArray& agentId)
{
	UpdateResult result;

	if (updateId.isEmpty()){
		result.successful = false;
		result.errorMessage = "Update ID is empty";
		return result;
	}

	if (!m_installedUpdates.contains(updateId)){
		result.successful = false;
		result.errorMessage = "Update was not previously installed or no backup available";
		return result;
	}

	QString errorMessage;
	if (!RestoreBackup(updateId, agentId, errorMessage)){
		result.successful = false;
		result.status = static_cast<int>(ISoftwareUpdateInfo::US_FAILED);
		result.errorMessage = QString("Rollback failed: %1").arg(errorMessage);
		return result;
	}

	m_installedUpdates.remove(updateId);

	result.successful = true;
	result.status = static_cast<int>(ISoftwareUpdateInfo::US_ROLLED_BACK);
	return result;
}


// private methods

bool CSoftwareUpdateManager::DownloadArtifact(const QString& sourceUrl, const QString& targetPath, QString& errorMessage)
{
	Q_UNUSED(sourceUrl);
	Q_UNUSED(targetPath);

	// TODO: Implement actual HTTP download from file-repository server
	// This will use the GraphQL API of the file-repository server
	// to fetch the artifact binary data
	errorMessage = "Download not yet implemented - requires file-repository server connection";
	qDebug() << "CSoftwareUpdateManager::DownloadArtifact - Source:" << sourceUrl << "Target:" << targetPath;

	return false;
}


bool CSoftwareUpdateManager::InstallArtifact(const QString& artifactPath, const QByteArray& agentId, QString& errorMessage)
{
	Q_UNUSED(artifactPath);
	Q_UNUSED(agentId);

	// TODO: Implement artifact installation
	// For service updates: stop service -> replace binaries -> restart service
	// For agent updates: staged update with process restart
	errorMessage = "Installation not yet implemented";
	qDebug() << "CSoftwareUpdateManager::InstallArtifact - Artifact:" << artifactPath << "Agent:" << agentId;

	return false;
}


bool CSoftwareUpdateManager::VerifyChecksum(const QString& filePath, const QString& expectedChecksum) const
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)){
		return false;
	}

	QCryptographicHash hash(QCryptographicHash::Sha256);
	if (!hash.addData(&file)){
		return false;
	}

	QString actualChecksum = hash.result().toHex();
	return (actualChecksum == expectedChecksum);
}


bool CSoftwareUpdateManager::BackupCurrentVersion(const QByteArray& updateId, const QByteArray& agentId, QString& errorMessage)
{
	Q_UNUSED(updateId);
	Q_UNUSED(agentId);

	// TODO: Implement backup of current installation before applying update
	// Should copy current binaries/configs to a backup directory
	errorMessage = "";
	qDebug() << "CSoftwareUpdateManager::BackupCurrentVersion - Update:" << updateId << "Agent:" << agentId;

	return true;
}


bool CSoftwareUpdateManager::RestoreBackup(const QByteArray& updateId, const QByteArray& agentId, QString& errorMessage)
{
	Q_UNUSED(agentId);

	if (!m_installedUpdates.contains(updateId)){
		errorMessage = "No backup found for this update";
		return false;
	}

	// TODO: Implement restore from backup
	// Should copy backed-up binaries/configs back to the installation directory
	qDebug() << "CSoftwareUpdateManager::RestoreBackup - Update:" << updateId << "Agent:" << agentId;

	return true;
}


} // namespace agentinodata


