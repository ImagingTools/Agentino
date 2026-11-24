// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QDateTime>

// ACF includes
#include <iser/IObject.h>


namespace agentinodata
{


/**
	Interface for describing a version of a service type uploaded to the server.
	A version is a folder on the server: the name of the folder is the version.
	\ingroup Service
*/
class IServiceVersionInfo: virtual public iser::IObject
{
public:
	/**
		Get ID of the service type (the software product) the version belongs to.
	*/
	virtual QByteArray GetServiceTypeId() const = 0;

	/**
		Get the operating system family the version is built for: "Windows", "Linux", "macOS" or "Other"
		(the families of the "Update" page, see CServiceUpdateControllerComp). Versions differ by it.
	*/
	virtual QString GetOperatingSystem() const = 0;

	/**
		Get the version, e.g. "1.0.0.513".
	*/
	virtual QString GetVersion() const = 0;

	/**
		Get the path of the folder of the version on the server.
	*/
	virtual QString GetPath() const = 0;

	/**
		Get the time the version was uploaded (added) to the server.
	*/
	virtual QDateTime GetUploadDate() const = 0;
};


} // namespace agentinodata


