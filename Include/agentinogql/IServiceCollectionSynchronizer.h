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
	managed by IServiceManager. When a service is created / changed /
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

		**Non-blocking** (\c TAsyncClientRequestManagerCompWrap / \c IAsyncGqlClient):
		\c true means GetService was dispatched, not that the mirror already holds the row.
		A blocking Wait on the WebSocket completion path deadlocks agent traffic.

		\param agentId		Id of the owning agent.
		\param serviceId	Id of the added / changed service.
		\param errorMessage	Filled when the fetch could not be started.
		\return				\c true when async sync was accepted.
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
		and schedules each remaining/new service via non-blocking SyncServiceInMirror.
		Called on every agent (re)connect. On list-fetch failure the mirror
		must not be modified.

		**Asynchronous**: the ServicesList round-trip goes through the async GQL
		client so the calling (socket / GQL worker) thread is never parked in a
		nested event loop. A \c true return therefore means *"reconcile accepted
		and scheduled"*, not *"mirror is up to date"* — never read the mirror
		expecting this call to have filled it. Overlapping requests for the same
		agent are coalesced: one runs, the next is deferred until it finishes.
		Failures during the deferred pass are reported through the component log.

		\param agentId		Id of the owning agent.
		\param errorMessage	Filled when the reconcile could not be scheduled
							(bad configuration, enrollment gate, agent unknown).
		\return				\c true when the reconcile was scheduled.
	*/
	virtual bool SyncAgentServicesInMirror(
				const QByteArray& agentId,
				QString& errorMessage) = 0;
};


} // namespace agentinogql
