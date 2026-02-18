// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
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
};


} // namespace agentinodata


