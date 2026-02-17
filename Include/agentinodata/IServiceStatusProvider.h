// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{

/**
	Interface for providing service status.
	\ingroup Service
*/
class IServiceStatusProvider: virtual public istd::IChangeable
{
public:
	struct NotifierStatusInfo
	{
		QByteArray serviceId;
		IServiceStatusInfo::ServiceStatus serviceStatus;
	};

	static const QByteArray CN_STATUS_CHANGED;

	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


Q_DECLARE_METATYPE(agentinodata::IServiceStatusProvider::NotifierStatusInfo)

