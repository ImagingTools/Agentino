// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

// ACF includes
#include <istd/IPolymorphic.h>


namespace agentinogql
{


/**
	The installations of the versions of the services started by "Install" of the versions page
	(InstallServiceVersion of Updates.sdl) and their states: running until the agent reports the result
	(ReportServiceVersionInstallation) or the agent does not answer.

	The server handles the requests with a set of handlers per worker thread: the tracker is one component
	of the server shared by them, so all its methods are thread-safe.
*/
class IServiceVersionInstallationTracker: virtual public istd::IPolymorphic
{
public:
	enum InstallationState
	{
		IS_RUNNING,
		IS_SUCCEEDED,
		IS_FAILED
	};

	/**
		The installation of the version on a service of an agent.
	*/
	struct ServiceInstallation
	{
		QByteArray agentId;
		QString agentName;
		QByteArray serviceId;
		QString serviceName;
		InstallationState state = IS_RUNNING;
		// Why the installation failed
		QString message;
	};

	/**
		An installation of a version on the services of the agents.
	*/
	struct Installation
	{
		QByteArray installationId;
		QString version;
		QList<ServiceInstallation> services;
	};

	/**
		Create an installation; all its services are running.
		\return ID of the installation.
	*/
	virtual QByteArray CreateInstallation(const QString& version, const QList<ServiceInstallation>& services) = 0;

	/**
		Get the installation. A service that has been running longer than allowed is failed.
		\return false if the installation does not exist (any more).
	*/
	virtual bool GetInstallation(const QByteArray& installationId, Installation& installation) const = 0;

	/**
		End the installation on a service of an agent. An ended installation is not changed again.
		\return false if the installation has no such service of the agent.
	*/
	virtual bool FinishServiceInstallation(
				const QByteArray& installationId,
				const QByteArray& agentId,
				const QByteArray& serviceId,
				bool succeeded,
				const QString& message) = 0;
};


} // namespace agentinogql


