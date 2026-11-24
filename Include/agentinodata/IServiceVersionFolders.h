// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>


namespace agentinodata
{


/**
	Folders on the server the versions of the services are chosen in ("Set service folder" of the
	versions page): one folder per service type and operating system family.
	\ingroup Service
*/
class IServiceVersionFolders: virtual public istd::IChangeable
{
public:
	/**
		Get the folder of the versions of the service type for the operating system family
		("Windows", "Linux", "macOS" or "Other"). Empty if it is not set.
	*/
	virtual QString GetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem) const = 0;

	/**
		Set the folder of the versions of the service type for the operating system family.
		An empty path removes it.
	*/
	virtual void SetServiceFolder(const QByteArray& serviceTypeId, const QString& operatingSystem, const QString& path) = 0;
};


} // namespace agentinodata


