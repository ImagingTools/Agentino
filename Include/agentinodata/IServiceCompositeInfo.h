// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>
#include <agentinodata/IServiceStatusInfo.h>


namespace agentinodata
{


class IServiceCompositeInfo: virtual public istd::IChangeable
{
public:
	typedef QByteArray Id;
	typedef QList<Id> Ids;

	enum StateOfRequiredServices
	{
		SORS_UNDEFINED,
		SORS_RUNNING,
		SORS_NOT_RUNNING
	};

	I_DECLARE_ENUM(StateOfRequiredServices, SORS_UNDEFINED, SORS_RUNNING, SORS_NOT_RUNNING);

	virtual QByteArray GetServiceId(const QUrl& url) const = 0;
	virtual QByteArray GetServiceId(const QByteArray& dependantServiceConnectionId) const = 0;
	virtual IServiceStatusInfo::ServiceStatus GetServiceStatus(const QByteArray& serviceId) const = 0;
	virtual StateOfRequiredServices GetStateOfRequiredServices(const QByteArray& serviceId) const = 0;
	virtual Ids GetDependencyServices(const QByteArray& serviceId) const = 0;
	virtual QString GetServiceName(const QByteArray& serviceId) const = 0;
	virtual QString GetServiceAgentName(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata


