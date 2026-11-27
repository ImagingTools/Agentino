// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QMutex>

// ACF includes
#include <ilog/TLoggerCompWrap.h>

// Agentino includes
#include <agentinogql/IServiceVersionInstallationTracker.h>


namespace agentinogql
{


/**
	The installations of the versions of the services, kept in memory (see IServiceVersionInstallationTracker).
	A service whose agent has not reported the result within the timeout is failed; the installations are
	forgotten a day after they were started.
*/
class CServiceVersionInstallationTrackerComp:
			public ilog::CLoggerComponentBase,
			virtual public IServiceVersionInstallationTracker
{
public:
	typedef ilog::CLoggerComponentBase BaseClass;

	I_BEGIN_COMPONENT(CServiceVersionInstallationTrackerComp);
		I_REGISTER_INTERFACE(IServiceVersionInstallationTracker);
		I_ASSIGN(m_timeoutAttrPtr, "Timeout", "Time in seconds an agent may install a version (download, stop, unpack, start) before the installation is failed", false, 3600);
	I_END_COMPONENT;

	// reimplemented (IServiceVersionInstallationTracker)
	virtual QByteArray CreateInstallation(const QString& version, const QList<ServiceInstallation>& services) override;
	virtual bool GetInstallation(const QByteArray& installationId, Installation& installation) const override;
	virtual bool FinishServiceInstallation(
				const QByteArray& installationId,
				const QByteArray& agentId,
				const QByteArray& serviceId,
				bool succeeded,
				const QString& message) override;

private:
	struct InstallationRecord
	{
		Installation installation;
		QDateTime startTime;
	};

	/**
		Fail the running services of the installation that have run out of time (the mutex is locked).
	*/
	void CheckTimeout(InstallationRecord& record) const;

	int GetTimeout() const;

private:
	I_ATTR(int, m_timeoutAttrPtr);

	mutable QMutex m_mutex;
	mutable QMap<QByteArray, InstallationRecord> m_installations;
};


} // namespace agentinogql


