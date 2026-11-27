// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

// ACF includes
#include <istd/IPolymorphic.h>


namespace agentgql
{


/**
	Update of a service of the agent to a version of its service type uploaded to the server
	("Install" of the versions page of the server, UpdateServiceVersion of Services.sdl).
*/
class IServiceVersionInstaller: virtual public istd::IPolymorphic
{
public:
	/**
		Start the update of the service: the archives of the version are downloaded from the server over
		HTTP into the folder of the executable of the service, the service is stopped, the archives are
		unpacked there, the service is started and the archives are removed.
		The method returns at once; the update goes on in the installer, which reports its result to the log.
		\param serviceId	ID of the service in the service collection of the agent.
		\param version		the version, e.g. "1.0.0.513".
		\param archiveUrls	URLs of the archives (*.zip) of the version on the server. A URL without a host
							(path only) is resolved against the HTTP address of the server.
		\param installationId	ID of the installation on the server: the result of the update is reported to the
								server with it (ReportServiceVersionInstallation); empty: not reported.
		\param errorMessage	why the update cannot be started.
		\return true if the update was started.
	*/
	virtual bool StartServiceVersionUpdate(
				const QByteArray& serviceId,
				const QString& version,
				const QStringList& archiveUrls,
				const QByteArray& installationId,
				QString& errorMessage) = 0;
};


} // namespace agentgql


