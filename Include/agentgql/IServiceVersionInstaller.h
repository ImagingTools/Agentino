// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>

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
		Start the update of the service: the archive of the version is downloaded from the server into
		the folder of the executable of the service, the service is stopped, the archive is unpacked there,
		the service is started and the archive is removed.
		The method returns at once; the update goes on in the installer, which reports its result to the log.
		\param serviceId	ID of the service in the service collection of the agent.
		\param version		the version, e.g. "1.0.0.513".
		\param versionPath	path of the folder of the version on the server.
		\param errorMessage	why the update cannot be started.
		\return true if the update was started.
	*/
	virtual bool StartServiceVersionUpdate(
				const QByteArray& serviceId,
				const QString& version,
				const QString& versionPath,
				QString& errorMessage) = 0;
};


} // namespace agentgql


