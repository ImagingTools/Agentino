// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// Qt includes
#include <QtCore/QByteArray>


namespace agentinodata
{


/**
	Fleet-unique address of one connection endpoint published by a service.

	A consumer stores this in its dependant connection link
	(imtservice::CUrlConnectionLinkParam::GetDependantServiceConnectionId), and it is
	what the connection pick-list offers. The wire form is:

	    "<serviceId>|<connectionId>"

	\a serviceId is the producing service, \a connectionId is which of its endpoints
	(either an input-connection element id, or the uuid of one incoming connection of
	that input).

	Why the service id is part of the id: a connection id alone is only a *usage/type*
	marker shared by every service that speaks that protocol — several services across
	several agents publish the same one, so it cannot identify a producer. Prefixing the
	service id makes the reference unambiguous and, because service ids are unique
	fleet-wide (the status collection is flat and keyed by service id alone), it also
	means resolving a link back to its producer is a string split rather than a scan
	over every agent's every service.

	The agent id is deliberately *not* part of it: both the agent and the server must be
	able to build these ids, and an agent does not need to know its own id to describe
	its own services.
*/
namespace ServiceEndpointId
{


inline char Separator()
{
	return '|';
}


inline QByteArray Make(const QByteArray& serviceId, const QByteArray& connectionId)
{
	if (serviceId.isEmpty() || connectionId.isEmpty()){
		return QByteArray();
	}

	return serviceId + Separator() + connectionId;
}


/**
	Split \a endpointId into its parts. Returns false (leaving the outputs untouched)
	when \a endpointId is not in the structured form.
*/
inline bool Parse(const QByteArray& endpointId, QByteArray& serviceId, QByteArray& connectionId)
{
	const int separatorIndex = endpointId.indexOf(Separator());
	if (separatorIndex <= 0 || separatorIndex >= endpointId.size() - 1){
		return false;
	}

	serviceId = endpointId.left(separatorIndex);
	connectionId = endpointId.mid(separatorIndex + 1);

	return true;
}


/** Producing service of \a endpointId, or empty when it is not a structured id. */
inline QByteArray ServiceOf(const QByteArray& endpointId)
{
	QByteArray serviceId;
	QByteArray connectionId;
	if (!Parse(endpointId, serviceId, connectionId)){
		return QByteArray();
	}

	return serviceId;
}


/** Endpoint part of \a endpointId, or empty when it is not a structured id. */
inline QByteArray ConnectionOf(const QByteArray& endpointId)
{
	QByteArray serviceId;
	QByteArray connectionId;
	if (!Parse(endpointId, serviceId, connectionId)){
		return QByteArray();
	}

	return connectionId;
}


} // namespace ServiceEndpointId


} // namespace agentinodata
