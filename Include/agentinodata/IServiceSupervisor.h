// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QString>

// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <agentinodata/ServiceRuntimeState.h>


namespace agentinodata
{


/**
	Only writer of runtime status on the agent (Architecture Audit §4.6 / §7.7).
*/
class IServiceSupervisor: virtual public istd::IChangeable
{
public:
	virtual bool Start(const QByteArray& serviceId, QString& errorMessage) = 0;
	virtual bool Stop(const QByteArray& serviceId, QString& errorMessage) = 0;
	virtual ServiceRuntimeState GetState(const QByteArray& serviceId) const = 0;
};


} // namespace agentinodata
