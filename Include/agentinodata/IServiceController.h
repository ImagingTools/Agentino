// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Agentino includes
#include <agentinodata/IServiceStatusProvider.h>


#undef StartService

namespace agentinodata
{


/**
	Interface for controller an service.
	\ingroup Service
*/
class IServiceController: virtual public IServiceStatusProvider
{
public:
	virtual bool StartService(const QByteArray& serviceId) = 0;
	virtual bool StopService(const QByteArray& serviceId) = 0;

	/**
		Stop a running service and start it again once its process has ended, e.g. to give it a new server token.
		A service that does not run is left as it is.
	*/
	virtual bool RestartService(const QByteArray& serviceId) = 0;
};


} // namespace agentinodata


