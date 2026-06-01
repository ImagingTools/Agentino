// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QPoint>

// ACF includes
#include <istd/IChangeable.h>

// Agentino includes
#include <agentinodata/agentinodata.h>


namespace agentinodata
{


/**
	Interface for local agent-side topology storage.
	Provides position data for services managed by this agent.
	The agent is the source of truth for its own service layout.
	\ingroup Topology
*/
class ILocalTopologyInfo: virtual public istd::IChangeable
{
public:
	/**
		Get the visual position of a service by its ID.
		Returns a default-constructed QPoint if the service has no stored position.
	*/
	virtual QPoint GetServicePosition(const QByteArray& serviceId) const = 0;

	/**
		Set the visual position of a service.
		Returns true on success.
	*/
	virtual bool SetServicePosition(const QByteArray& serviceId, const QPoint& point) = 0;

	/**
		Return the IDs of all services with a stored position.
	*/
	virtual QByteArrayList GetServiceIds() const = 0;
};


} // namespace agentinodata
