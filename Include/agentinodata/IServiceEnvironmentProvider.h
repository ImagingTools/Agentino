// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QHash>
#include <QtCore/QString>

// ACF includes
#include <istd/IPolymorphic.h>

// Agentino includes
#include <agentinodata/IServiceInfo.h>


namespace agentinodata
{


/**
	Environment variables a service is started with, beyond the ones of the agent.
*/
class IServiceEnvironmentProvider: virtual public istd::IPolymorphic
{
public:
	typedef QHash<QString, QString> Environment;

	virtual Environment GetServiceEnvironment(const QByteArray& serviceId, IServiceInfo& serviceInfo) const = 0;
};


} // namespace agentinodata


