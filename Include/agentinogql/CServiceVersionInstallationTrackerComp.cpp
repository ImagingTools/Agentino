// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#include <agentinogql/CServiceVersionInstallationTrackerComp.h>


// Qt includes
#include <QtCore/QMutexLocker>
#include <QtCore/QUuid>


namespace agentinogql
{


// Time in seconds an installation is kept after it was started.
static const qint64 s_keepTime = 24 * 60 * 60;


// reimplemented (IServiceVersionInstallationTracker)

QByteArray CServiceVersionInstallationTrackerComp::CreateInstallation(const QString& version, const QList<ServiceInstallation>& services)
{
	InstallationRecord record;
	record.installation.installationId = QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
	record.installation.version = version;
	record.installation.services = services;
	record.startTime = QDateTime::currentDateTimeUtc();

	for (ServiceInstallation& service : record.installation.services){
		service.state = IS_RUNNING;
		service.message.clear();
	}

	QMutexLocker locker(&m_mutex);

	// The old installations are forgotten.
	for (auto iter = m_installations.begin(); iter != m_installations.end();){
		if (iter->startTime.secsTo(record.startTime) > s_keepTime){
			iter = m_installations.erase(iter);
		}
		else{
			++iter;
		}
	}

	m_installations.insert(record.installation.installationId, record);

	return record.installation.installationId;
}


bool CServiceVersionInstallationTrackerComp::GetInstallation(const QByteArray& installationId, Installation& installation) const
{
	QMutexLocker locker(&m_mutex);

	auto iter = m_installations.find(installationId);
	if (iter == m_installations.end()){
		return false;
	}

	CheckTimeout(*iter);

	installation = iter->installation;

	return true;
}


bool CServiceVersionInstallationTrackerComp::FinishServiceInstallation(
			const QByteArray& installationId,
			const QByteArray& agentId,
			const QByteArray& serviceId,
			bool succeeded,
			const QString& message)
{
	QMutexLocker locker(&m_mutex);

	auto iter = m_installations.find(installationId);
	if (iter == m_installations.end()){
		return false;
	}

	for (ServiceInstallation& service : iter->installation.services){
		if (service.agentId != agentId || service.serviceId != serviceId){
			continue;
		}

		// The first result counts (e.g. a timeout is not overwritten by a late report).
		if (service.state == IS_RUNNING){
			service.state = succeeded ? IS_SUCCEEDED : IS_FAILED;
			service.message = message;
		}

		return true;
	}

	return false;
}


// private methods

void CServiceVersionInstallationTrackerComp::CheckTimeout(InstallationRecord& record) const
{
	const int timeout = GetTimeout();
	if (record.startTime.secsTo(QDateTime::currentDateTimeUtc()) <= timeout){
		return;
	}

	for (ServiceInstallation& service : record.installation.services){
		if (service.state == IS_RUNNING){
			service.state = IS_FAILED;
			service.message = QStringLiteral("The agent did not report the result within %1 minutes").arg(timeout / 60);
		}
	}
}


int CServiceVersionInstallationTrackerComp::GetTimeout() const
{
	return (m_timeoutAttrPtr.IsValid() && *m_timeoutAttrPtr > 0) ? *m_timeoutAttrPtr : 3600;
}


} // namespace agentinogql


