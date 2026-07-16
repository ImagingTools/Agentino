// SPDX-License-Identifier: LicenseRef-Agentino-Commercial
#pragma once


// ACF includes
#include <istd/IChangeable.h>

// Qt includes
#include <QtCore/QByteArray>
#include <QtCore/QByteArrayList>


namespace agentinogql
{


/**
	Refreshes the server-side mirror of an agent's service collection.

	The server keeps a local mirror of every connected agent's services
	(agentInfo->GetServiceCollection()). When a service is created / changed /
	removed directly on an agent, the agent pushes an
	'OnAgentServicesCollectionChanged' notification up to the server; the
	receiver reconciles the mirror through this interface.

	Full reconciliation on (re)connect is done via SyncAgentServicesInMirror.
*/
class IServiceCollectionSynchronizer: virtual public istd::IChangeable
{
public:
	/**
		Fetches the current data of a single service from the owning agent and
		inserts it into (or updates it in) the server-side mirror.
		\param agentId		Id of the owning agent.
		\param serviceId	Id of the added / changed service.
		\param errorMessage	Filled on failure.
		\return				\c true on success.
	*/
	virtual bool SyncServiceInMirror(
				const QByteArray& agentId,
				const QByteArray& serviceId,
				QString& errorMessage) = 0;

	/**
		Removes services from the server-side mirror.
		\param agentId		Id of the owning agent.
		\param serviceIds	Ids of the removed services.
		\param errorMessage	Filled on failure.
		\return				\c true on success.
	*/
	virtual bool RemoveServicesInMirror(
				const QByteArray& agentId,
				const QByteArrayList& serviceIds,
				QString& errorMessage) = 0;

	/**
		Full reconciliation of the server-side mirror with the agent's
		authoritative ServicesList. Pulls the full list, removes mirror
		entries that no longer exist on the agent (and their status rows),
		and syncs each remaining/new service via SyncServiceInMirror.
		Called on every agent (re)connect. On list-fetch failure the mirror
		must not be modified.
		\param agentId		Id of the owning agent.
		\param errorMessage	Filled on failure (may aggregate partial errors).
		\return				\c true on success.
	*/
	virtual bool SyncAgentServicesInMirror(
				const QByteArray& agentId,
				QString& errorMessage) = 0;
};


} // namespace agentinogql
